/////////////////////////////////////////////////////////////////////
// MQTTValue: Topic class for values
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// A SimpleMQTT topic that represents a value with a given data type T.
template<typename T>
class MQTTValue : public MQTTFormattedTopic<T> {
friend class MQTTGroup;
friend class MQTTArray<T*>;
friend class SimpleMQTTClient;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTValue<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTValue<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  T _value{};

  MQTTValue(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig) {};

  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  MQTTValue(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, const char* str)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig) {
      setFromPayload(str);
    };

  inline String type() const override { 
    if constexpr (std::is_const_v<T>)
      return String("!#"); 
    return String("#"); 
  };

  virtual void _setValue(T newValue) {
    SIMPLEMQTT_CHECK_VALID();
    if constexpr (!std::is_const_v<T>)
      _value = newValue;
  };

  virtual bool _set(T newValue) {
    SIMPLEMQTT_CHECK_VALID(false);
    if constexpr (std::is_const_v<T>)
      return false;
    bool changed = !this->_isEqual(newValue);
    _setValue(newValue);
    if (MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    return changed;
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
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTValue<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTValue<T>, T)

  bool isSettable() const override {
    SIMPLEMQTT_CHECK_VALID(false);
    if constexpr (std::is_const_v<T>)
      return false;
    return MQTTTopic::isSettable();
  };

  // Returns the current value of this topic.
  T value() const override {
    SIMPLEMQTT_CHECK_VALID(T{});
    return _value;
  };

  // Sets the current value of this topic. Returns whether the value has changed.
  // Does not modify the Changed flag.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline bool set(T newValue) {
    SIMPLEMQTT_CHECK_VALID(false);
    return _set(newValue);
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTValue<T>& operator=(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    _set(newValue);
    return *this;
  };

  // Sets the current value of this topic.
  // Checks whether the new value is different from the current value
  // and sets the Changed flag on the topic if this is the case.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTValue<T>& setTo(T newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    bool changed = _set(newValue);
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return *this;
  };

  // Sets the function that parses an incoming payload for this topic.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  MQTTValue<T>& setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(*this);
    payloadHandler = handler;
    return *this;
  };

  // Attempts to parse the payload and sets the newValue variable.
  // For use by custom implementations of payload handlers.
  bool parseValue(const char* str, T* newValue) {
    if constexpr (std::is_const_v<T>)
      return false;
    return __internal::parseValue(str, newValue, MQTTFormattedTopic<T>::format);
  };

  // Attempts to set this topic's value from the supplied payload string.
  // Returns a ResultCode that indicates success or the reason of failure.
  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
    if constexpr (std::is_const_v<T>)
      return ResultCode::CANNOT_SET;
    T newValue = _value;
    if (!parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    _set(newValue);
    return ResultCode::OK;
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTValue<T>& operator=(const char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTValue<T>& operator=(char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<String, U>::value, bool>::type* = nullptr>
  inline MQTTValue<T>& operator=(const String& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<std::string, U>::value, bool>::type* = nullptr>
  inline MQTTValue<T>& operator=(const std::string& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTValue<T>& operator=(const __FlashStringHelper* payload) { setFromPayload((String() + payload).c_str()); return *this; };
};

template<class T>
struct mqtt_value_type { typedef MQTTValue<T>& type; };
