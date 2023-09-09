/////////////////////////////////////////////////////////////////////
// MQTTArray for arithmetic type arrays of known lengths or char*s
/////////////////////////////////////////////////////////////////////

template<typename T>
class MQTTArray : public MQTTFormattedTopic<std::remove_pointer_t<T>> {
  friend class MQTTGroup;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTArray<T>* object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTArray<T>* object, const char* payload) {
    return object->setFromPayload(payload);
  };
  T array = nullptr;
  size_t length = 0;
  typename mqtt_variable<std::remove_pointer_t<T>>::type helper;  // conversion helper

  MQTTArray(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, T arr, size_t elementCount)
    : MQTTFormattedTopic<std::remove_pointer_t<T>>(aParent, aTopic, aConfig),
      array(arr), length(elementCount), helper(aParent, aTopic, aConfig, arr) {
    helper.setSettable(true);
  };

  virtual MQTTArray<T>* _setValue(T const sourceArray) {
    SIMPLEMQTT_CHECK_VALID(this);
    if constexpr (!std::is_const_v<std::remove_pointer_t<T>>)
      memcpy(array, sourceArray, sizeof(std::remove_pointer_t<T>) * length);
    return this;
  };

  virtual bool _isEqual(T other) {
    SIMPLEMQTT_CHECK_VALID(false);
    return memcmp(array, other, sizeof(std::remove_pointer_t<T>) * length) == 0;
  };

  virtual ResultCode setReceived(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    MQTTTopic::republish();
    if (payloadHandler != nullptr)
      return payloadHandler(this, payload);
    return setFromPayload(payload);
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTArray<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTArray<T>, std::remove_pointer_t<T>)

  bool isSettable() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTFormattedTopic<std::remove_pointer_t<T>>::isSettable())
      return false;
    if constexpr (std::is_const_v<T>)
      return false;
    if constexpr (std::is_const_v<std::remove_pointer_t<T>>)
      return false;
    return helper.isSettable();
  };

  virtual bool set(T sourceArray, bool publish = false) {
    SIMPLEMQTT_CHECK_VALID(false);
    bool changed = !_isEqual(sourceArray);
    _setValue(sourceArray);
    if (MQTTTopic::isAutoPublish() || publish)
      MQTTTopic::republish();
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return changed;
  };

  virtual MQTTArray<T>* setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(this);
    payloadHandler = handler;
    return this;
  };

  virtual ResultCode setFromPayload(const char* payload) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    if (!isSettable())
      return ResultCode::CANNOT_SET;
    if (payload[0] == '\0')
      return ResultCode::OK;
    if constexpr (std::is_const_v<std::remove_pointer<T>>)
      return ResultCode::CANNOT_SET;
    else {
      // target array
      std::remove_const_t<std::remove_pointer_t<T>> newValues[length];
      // copy current values
      memcpy(newValues, array, sizeof(std::remove_pointer_t<T>) * length);
      // iterate through values
      // expected format: "<v1>,<v2>,..."
      size_t i = 0;
      const char* s = payload;
      while (*s != '\0' && i < length) {
        // modifies payload temporarily in-place
        char* e = (char*)s;
        while (*e != ',' && *e != '\0') {
          e++;
        }
        // empty value? skip
        if (e == s) {
          s++;
          i++;
          continue;
        }
        bool atEnd = *e == '\0';
        // mark end for value parser
        *e = '\0';
        helper.setPointer(&newValues[i]);
        // helper handles conversion
        ResultCode code = helper.setReceived(s);
        if (code != ResultCode::OK)
          return code;
        if (!atEnd) {
          *e = ',';
          s = e + 1;
        } else
          break;
        i++;
      }
      set((T)&newValues, true);
      return ResultCode::OK;
    }
  };

  virtual String getPayload() {
    SIMPLEMQTT_CHECK_VALID(String());
    String result;
    for (size_t i = 0; i < length; i++) {
      helper.setPointer(&array[i]);
      result += helper.getPayload();
      if (i < length - 1)
        result += ",";
    }
    return result;
  };

  virtual typename format_type<std::remove_pointer_t<T>>::type getElementFormat() {
    SIMPLEMQTT_CHECK_VALID(typename format_type<std::remove_pointer_t<T>>::type{});
    return helper.format;
  };

  virtual MQTTArray<T>* setElementFormat(typename format_type<std::remove_pointer_t<T>>::type aFormat) {
    SIMPLEMQTT_CHECK_VALID(this);
    helper.format = aFormat;
    return this;
  };

  virtual typename mqtt_variable<std::remove_pointer_t<T>>::type* element() {
    return &helper;
  };
};

template<class T>
struct mqtt_array_type { typedef MQTTArray<T>* type; };


/////////////////////////////////////////////////////////////////////
// MQTTArray<T> specializations
/////////////////////////////////////////////////////////////////////

// char*

class MQTTCharArray : public MQTTArray<char*> {
  friend class MQTTGroup;

protected:
  MQTTCharArray(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, char* array, size_t len)
    : MQTTArray<char*>(aParent, aTopic, aConfig, array, len){};

  MQTTCharArray* _setValue(char* sourceArray) override {
    SIMPLEMQTT_CHECK_VALID(this);
    strncpy(array, sourceArray, length - 1);
    array[length - 1] = '\0';
    return this;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTCharArray)

  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    _setValue((char*)payload);
    return ResultCode::OK;
  };

  String getPayload() {
    SIMPLEMQTT_CHECK_VALID(String());
    return String(array);
  };
};

template<>
struct mqtt_array_type<char*> { typedef MQTTCharArray* type; };

// const char*

class MQTTConstCharArray : public MQTTArray<const char*> {
  friend class MQTTGroup;

protected:
  MQTTConstCharArray(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, const char* array, size_t len)
    : MQTTArray<const char*>(aParent, aTopic, aConfig, array, len){};

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTConstCharArray)

  String getPayload() {
    SIMPLEMQTT_CHECK_VALID(String());
    return String(array);
  };
};

template<>
struct mqtt_array_type<const char*> { typedef MQTTConstCharArray* type; };
