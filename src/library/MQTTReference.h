/////////////////////////////////////////////////////////////////////
// MQTTReference: Topic class for reference variables
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// A SimpleMQTT topic whose underlying variable is a reference of data type T.
template<typename T>
class MQTTReference : public MQTTValue<T> {
friend class MQTTGroup;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTReference<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTReference<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  T& valueRef;

  MQTTReference(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, T& aValue)
    : MQTTValue<T>(aParent, aTopic, aConfig),
      valueRef(aValue) { 
        if constexpr (!std::is_const_v<T>)
          MQTTValue<T>::_value = aValue;
      };

  inline String type() const override { 
    if constexpr (std::is_const_v<T>)
      return String("!&"); 
    return String("&"); 
  };

  void _setValue(T newValue) override {
    SIMPLEMQTT_CHECK_VALID();
    if constexpr (!std::is_const_v<T>)
      valueRef = newValue;
  };

  bool _isEqual(T other) override {
    SIMPLEMQTT_CHECK_VALID(false);
    return valueRef == other;
  };

  bool check() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTValue<T>::check())
      return false;
    if constexpr (!std::is_const_v<T>) {
      // detect underlying value change
      if (!_isEqual(MQTTValue<T>::_value)) {
        MQTTValue<T>::_value = valueRef;
        if (MQTTValue<T>::isAutoPublish())
          MQTTValue<T>::republish();
      }
    }
    return true;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTReference<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTReference<T>, T)

  // Sets the function that parses an incoming payload for this topic.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  MQTTReference<T>& setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(*this);
    payloadHandler = handler;
    return *this;
  };

  // Returns the current value of this topic.
  T value() const override {
    return valueRef;
  };

  inline operator T&() const {
	  return &valueRef;
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTReference<T>& operator=(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    MQTTValue<T>::set(newValue);
    return *this;
  };

  // Sets the current value of this topic.
  // Checks whether the new value is different from the current value
  // and sets the Changed flag on the topic if this is the case.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTReference<T>& setTo(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    bool changed = _set(newValue);
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return *this;
  };

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    String s = __internal::formatValue(value(), MQTTValue<T>::format);
    return s;
  };

  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    return ResultCode::CANNOT_SET;
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTReference<T>& operator=(const char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTReference<T>& operator=(char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<String, U>::value, bool>::type* = nullptr>
  inline MQTTReference<T>& operator=(const String& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<std::string, U>::value, bool>::type* = nullptr>
  inline MQTTReference<T>& operator=(const std::string& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTReference<T>& operator=(const __FlashStringHelper* payload) { setFromPayload((String() + payload).c_str()); return *this; };
};

template<class T>
struct mqtt_reference_type { typedef MQTTReference<T>& type; };

/////////////////////////////////////////////////////////////////////
// MQTTReference<T> specializations
/////////////////////////////////////////////////////////////////////

template<>
ResultCode MQTTReference<String>::setFromPayload(const char* payload) {
  SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
  SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
  String newValue = String(payload);
  _set(newValue);
  return ResultCode::OK;
};
