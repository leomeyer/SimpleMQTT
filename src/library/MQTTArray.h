/////////////////////////////////////////////////////////////////////
// MQTTArray for fundamental type arrays of fixed length
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

template<typename T>
class MQTTArray : public MQTTTopic {
friend class MQTTGroup;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTArray<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTArray<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  typename std::remove_const_t<T> array = nullptr;
  size_t length = 0;
  typename mqtt_variable<std::remove_pointer_t<T>>::type helper;  // conversion helper

  MQTTArray(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, T arr, size_t elementCount)
    : MQTTTopic(aParent, aTopic, aConfig),
      array(arr), length(elementCount), helper(aParent, aTopic, aConfig, arr) {
    helper.setSettable(true);
  };

  inline String type() const override {
    String result("");
    if constexpr (std::is_const_v<std::remove_pointer_t<T>>)
      result += "!";
    result += "[";
    result += length;
    result += "]";
    return result; 
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<std::remove_pointer_t<U>>, bool>::type* = nullptr> // only for non-const types
  void _setValue(T const sourceArray) {
    SIMPLEMQTT_CHECK_VALID();
    memcpy(array, sourceArray, sizeof(std::remove_pointer_t<T>) * length);
  };

  template<typename U = T, typename std::enable_if<std::is_const_v<std::remove_pointer_t<U>>, bool>::type* = nullptr> // only for const types
  void _setValue(T) {};

  virtual bool _isEqual(T other) {
    SIMPLEMQTT_CHECK_VALID(false);
    return memcmp(array, other, sizeof(std::remove_pointer_t<T>) * length) == 0;
  };

  ResultCode setReceived(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    if (MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    if (payloadHandler != nullptr)
      return payloadHandler(*this, payload);
    return setFromPayload(payload);
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTArray<T>)

  bool isSettable() const override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::isSettable())
      return false;
    if constexpr (std::is_const_v<T>)
      return false;
    if constexpr (std::is_const_v<std::remove_pointer_t<T>>)
      return false;
    return helper.isSettable();
  };

  size_t size() {
    return length;
  };

  virtual bool set(T sourceArray, bool publish = false) {
    SIMPLEMQTT_CHECK_VALID(false);
    bool changed = !_isEqual(sourceArray);
    _setValue(sourceArray);
    if (MQTTTopic::isAutoPublish() || publish)
      MQTTTopic::republish();
    return changed;
  };

  virtual MQTTArray<T>& setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(*this);
    payloadHandler = handler;
    return *this;
  };

  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
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
      bool changed = set((T)&newValues, true);
      MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
      return ResultCode::OK;
    }
  };

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    String result;
    for (size_t i = 0; i < length; i++) {
      const_cast<MQTTArray<T>*>(this)->helper.setPointer(&array[i]);
      result += helper.getPayload();
      if (i < length - 1)
        result += ",";
    }
    return result;
  };

  virtual typename format_type<std::remove_pointer_t<T>>::type getFormat() {
    SIMPLEMQTT_CHECK_VALID(typename format_type<std::remove_pointer_t<T>>::type{});
    return helper.format;
  };

  virtual MQTTArray<T>& setFormat(typename format_type<std::remove_pointer_t<T>>::type aFormat) {
    SIMPLEMQTT_CHECK_VALID(*this);
    helper.format = aFormat;
    return *this;
  };

  virtual typename mqtt_variable<std::remove_pointer_t<T>>::type& element() {
    return helper;
  };

  // Returns a pointer to the element at index i. Does not perform range checking.
  T get(size_t i) {
    SIMPLEMQTT_CHECK_VALID(T{});
    return &array[i];
  };

  // Returns a pointer to the element at index i. Does not perform range checking.
  inline T operator[](size_t i) {
    return get(i);
  };
};

template<class T>
struct mqtt_array_type { typedef MQTTArray<T>& type; };

template<typename T, size_t N>
class MQTTValueArray : public MQTTArray<T*> {
friend class MQTTGroup;

protected:
  T data[N];

  MQTTValueArray(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTArray<T*>(aParent, aTopic, aConfig, &data[0], N) {};
};

template<class T, size_t N>
struct mqtt_valuearray_type { typedef MQTTValueArray<T, N>& type; };


/////////////////////////////////////////////////////////////////////
// MQTTArray<T> specializations
/////////////////////////////////////////////////////////////////////

// Array topic representing a modifiable char* string of fixed length.
class MQTTCharArray : public MQTTArray<char*> {
  friend class MQTTGroup;

protected:
  MQTTCharArray(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, char* array, size_t len)
    : MQTTArray<char*>(aParent, aTopic, aConfig, array, len){};

  MQTTCharArray& _setValue(char* sourceArray) {
    SIMPLEMQTT_CHECK_VALID(*this);
    strncpy(array, sourceArray, length - 1);
    array[length - 1] = '\0';
    return *this;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTCharArray)

  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    _setValue((char*)payload);
    return ResultCode::OK;
  };

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    return String(array);
  };
};

template<>
struct mqtt_array_type<char*> { typedef MQTTCharArray& type; };

// Array topic representing a const char* string.
class MQTTConstCharArray : public MQTTArray<const char*> {
  friend class MQTTGroup;

protected:
  MQTTConstCharArray(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, const char* array, size_t len)
    : MQTTArray<const char*>(aParent, aTopic, aConfig, array, len) {};

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTConstCharArray)

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    return String(array);
  };
};

template<>
struct mqtt_array_type<const char*> { typedef MQTTConstCharArray& type; };
