/////////////////////////////////////////////////////////////////////
// MQTTArray for fundamental type arrays of fixed length
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// An array of the underlying data type with a specified constant length.
// Is settable if the underlying data type is not const.
// Access to the array's elements is provided via get() and set() methods.
// Alternatively the index operator [] can be used. If the index is out of bounds
// the getter returns the default value while the setter does nothing.
// Elements are formatted using the format specified with setFormat().
// The separator character for the string representation of the array can be
// set or retrieved using setSeparator() or getSeparator() respectively.
template<typename T>
class MQTTArray : public MQTTTopic {
friend class MQTTGroup;

protected:
  typedef typename std::remove_pointer_t<T> E;
  typedef ResultCode (*PayloadHandler)(MQTTArray<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTArray<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  typename std::remove_const_t<T> array = nullptr;
  size_t length = 0;
  char separator = ',';
  typename mqtt_variable<E>::type helper;  // conversion helper

  template<typename E>
  class ElementProxy {
    MQTTArray<T>* parent;
    size_t index;

  public:
    ElementProxy(MQTTArray<T>* aParent, size_t anIndex) : parent(aParent), index(anIndex) {}; 

    // Returns the current value of this topic.
    E value() const {
      if (index >= parent->length)
        return E{};
      return parent->array[index];
    };

    inline operator E() const {
      return value();
    };

    template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
    inline ElementProxy<E>& operator=(const E& newValue) {
      parent->set(index, newValue);
      return *this;
    };

    inline String getPayload() const {
      return parent->getPayload(index);
    }

    template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
    ResultCode setFromPayload(const char* payload) {
/*      
      Serial.print("ElementProxy.setFromPayload(");
      Serial.print(index);
      Serial.print(", ");
      Serial.print(payload);
      Serial.println(")");
*/
      return parent->setFromPayload(index, payload);
    };

    template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
    inline ElementProxy<E>& operator=(const char* payload) { setFromPayload(payload); return *this; };
    template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
    inline ElementProxy<E>& operator=(char* payload) { setFromPayload(payload); return *this; };
    template<typename U = E, typename std::enable_if<!std::is_const_v<U> && !std::is_same<String, U>::value, bool>::type* = nullptr>
    inline ElementProxy<E>& operator=(const String& payload) { setFromPayload(payload.c_str()); return *this; };
    template<typename U = E, typename std::enable_if<!std::is_const_v<U> && !std::is_same<std::string, U>::value, bool>::type* = nullptr>
    inline ElementProxy<E>& operator=(const std::string& payload) { setFromPayload(payload.c_str()); return *this; };
    template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
    inline ElementProxy<E>& operator=(const __FlashStringHelper* payload) { setFromPayload((String() + payload).c_str()); return *this; };
  };

  MQTTArray(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, T arr, size_t elementCount)
    : MQTTTopic(aParent, aTopic, aConfig),
      array(arr), length(elementCount), helper(aParent, aTopic, aConfig, arr) {
    helper.setSettable(true);
  };

  inline String type() const override {
    String result("");
    if constexpr (std::is_const_v<E>)
      result += "!";
    result += "[";
    result += length;
    result += "]";
    return result; 
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<std::remove_pointer_t<U>>, bool>::type* = nullptr> // only for non-const types
  void _setValue(T const sourceArray) {
    SIMPLEMQTT_CHECK_VALID();
    memcpy(array, sourceArray, sizeof(E) * length);
  };

  template<typename U = T, typename std::enable_if<std::is_const_v<std::remove_pointer_t<U>>, bool>::type* = nullptr> // only for const types
  void _setValue(T) {};

  virtual bool _isEqual(T other) {
    SIMPLEMQTT_CHECK_VALID(false);
    return memcmp(array, other, sizeof(E) * length) == 0;
  };

  ResultCode setReceived(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    if (MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    if (payloadHandler != nullptr)
      return payloadHandler(*this, payload);
    return setFromPayload(payload);
  };

  // Copies the values from the source array into this array.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  bool set(T sourceArray, bool publish = false) {
    SIMPLEMQTT_CHECK_VALID(false);
    bool changed = !_isEqual(sourceArray);
    _setValue(sourceArray);
    if (changed && (MQTTTopic::isAutoPublish() || publish))
      MQTTTopic::republish();
    return changed;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTArray<T>)

  bool isSettable() const override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::isSettable())
      return false;
    if constexpr (std::is_const_v<E>)
      return false;
    return true;
  };

  // Returns the length of the array as specified during initialization.
  size_t size() const {
    return length;
  };

  inline char getSeparator() const {
    return separator;
  }

  inline void setSeparator(const char s) {
    if (s != '\0')
      separator = s;
  }

  String getPayload(size_t index) const {
    SIMPLEMQTT_CHECK_VALID(String());
    String result;
    if (index <= length) {
      const_cast<MQTTArray<T>*>(this)->helper.setPointer(&array[index]);
      result += helper.getPayload();
    }
    return result;
  };

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    String result;
    for (size_t i = 0; i < length; i++) {
      const_cast<MQTTArray<T>*>(this)->helper.setPointer(&array[i]);
      result += helper.getPayload();
      if (i < length - 1)
        result += separator;
    }
    return result;
  };

  // Sets the value of the element at the given index. Returns whether the value has changed
  // and auto-publishes the array if necessary.
  template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  bool set(size_t index, E newValue) {
    SIMPLEMQTT_CHECK_VALID(false);
    if (index >= length)
      return false;
    helper.setPointer(&array[index]);
    helper.hasBeenChanged();  // clear flag
    helper.setTo(newValue);
    bool changed = helper.hasBeenChanged();
    if (changed && MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    return changed;
  };

  // Sets the value of the element at the given index from the specified payload.
  // Auto-publishes the array if necessary.
  template<typename U = E, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  ResultCode setFromPayload(size_t index, const char* payload) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    if (index >= length)
      return ResultCode::CANNOT_SET;
    helper.setPointer(&array[index]);
    helper.hasBeenChanged();  // clear flag
    helper.setFromPayload(payload);
    bool changed = helper.hasBeenChanged();
    if (changed && MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    return ResultCode::OK;
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
      std::remove_const_t<E> newValues[length];
      // copy current values
      memcpy(newValues, array, sizeof(E) * length);
      // iterate through values
      // expected format: "<v1>,<v2>,..."
      size_t i = 0;
      const char* s = payload;
      while (*s != '\0' && i < length) {
        // modifies payload temporarily in-place
        char* e = (char*)s;
        while (*e != separator && *e != '\0') {
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
          *e = separator;
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

  virtual typename format_type<E>::type getFormat() {
    SIMPLEMQTT_CHECK_VALID(typename format_type<E>::type{});
    return helper.format;
  };

  virtual MQTTArray<T>& setFormat(typename format_type<E>::type aFormat) {
    SIMPLEMQTT_CHECK_VALID(*this);
    helper.format = aFormat;
    return *this;
  };

  virtual typename mqtt_variable<E>::type& element() {
    return helper;
  };

  ElementProxy<E> get(size_t i) {
    return ElementProxy<E>(this, i);
  };

  inline ElementProxy<E> operator[](size_t i) {
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
