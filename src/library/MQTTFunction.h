/////////////////////////////////////////////////////////////////////
// MQTTFunction
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// A SimpleMQTT topic that publishes the value of a function when requested.
template<typename T>
class MQTTGetFunction : public MQTTFormattedTopic<T> {
friend class MQTTGroup;

protected:
  typedef T (*GetFunction)(void);
  GetFunction getFunction;

  MQTTGetFunction(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, GetFunction aFunction)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig &= AUTO_PUBLISH_CLEARMASK),
      getFunction(aFunction) {};

  inline String type() const override { 
    return String("(<)"); 
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTGetFunction<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTGetFunction<T>, T)

  bool isSettable() const override {
    return false;
  };

  // Returns the current value of this topic.
  T value() const override {
    return getFunction();
  };
};

template<class T>
struct mqtt_getfunction_type { typedef MQTTGetFunction<T>& type; };

// set function

// A SimpleMQTT topic that calls a function when set.
template<typename T>
class MQTTSetFunction : public MQTTFormattedTopic<T> {
friend class MQTTGroup;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTSetFunction<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTSetFunction<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  typedef void (*SetFunction)(T);
  SetFunction setFunction;

  MQTTSetFunction(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, SetFunction aFunction)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig &= AUTO_PUBLISH_CLEARMASK),
      setFunction(aFunction) {};

  inline String type() const override { 
    return String("(>)"); 
  };

  // Always returns the default value of the data type.
  T value() const override {
    return T{};
  };

  void _set(T newValue) {
    setFunction(newValue);
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
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTSetFunction<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTSetFunction<T>, T)

  bool isSettable() const override {
    return true;
  };

  bool isRequestable() const override {
    return false;
  };

  // Calls the set function of this topic.
  inline void set(T newValue) {
    SIMPLEMQTT_CHECK_VALID();
    _set(newValue);
  };

  inline MQTTSetFunction<T>& operator=(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    _set(newValue);
    return *this;
  };

  // Sets the function that parses an incoming payload for this topic.
  MQTTSetFunction<T>& setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(*this);
    payloadHandler = handler;
    return *this;
  };

  // Attempts to parse the payload and sets the newValue variable.
  // For use by custom implementations of payload handlers.
  bool parseValue(const char* str, T* newValue) {
    return __internal::parseValue(str, newValue, MQTTFormattedTopic<T>::format);
  };

  // Attempts to set this topic's value from the supplied payload string.
  // Returns a ResultCode that indicates success or the reason of failure.
  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
    T newValue;
    if (!parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    _set(newValue);
    return ResultCode::OK;
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<const char*, U>::value, bool>::type* = nullptr>
  inline MQTTSetFunction<T>& operator=(const char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<char*, U>::value, bool>::type* = nullptr>
  inline MQTTSetFunction<T>& operator=(char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<String, U>::value, bool>::type* = nullptr>
  inline MQTTSetFunction<T>& operator=(const String& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<std::string, U>::value, bool>::type* = nullptr>
  inline MQTTSetFunction<T>& operator=(const std::string& payload) { setFromPayload(payload.c_str()); return *this; };
  inline MQTTSetFunction<T>& operator=(const __FlashStringHelper* payload) { setFromPayload((String() + payload).c_str()); return *this; };
};

template<class T>
struct mqtt_setfunction_type { typedef MQTTSetFunction<T>& type; };

// get/set function topic

// set function

// A SimpleMQTT topic that has both a Get and a Set function.
template<typename T>
class MQTTGetSetFunction : public MQTTFormattedTopic<T> {
friend class MQTTGroup;

protected:
  typedef T (*GetFunction)(void);
  GetFunction getFunction;

  typedef ResultCode (*PayloadHandler)(MQTTGetSetFunction<T>& object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTGetSetFunction<T>& object, const char* payload) {
    return object.setFromPayload(payload);
  };
  typedef void (*SetFunction)(T);
  SetFunction setFunction;

  MQTTGetSetFunction(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, GetFunction aGetFunction, SetFunction aSetFunction)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig),
      getFunction(aGetFunction), setFunction(aSetFunction) {};

  inline String type() const override { 
    return String("(<>)"); 
  };

  void _set(T newValue) {
    setFunction(newValue);
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
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTGetSetFunction<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTGetSetFunction<T>, T)

  // Returns the current value of this topic.
  T value() const override {
    return getFunction();
  };

  // Calls the set function of this topic.
  inline void set(T newValue) {
    SIMPLEMQTT_CHECK_VALID();
    _set(newValue);
  };

  inline MQTTGetSetFunction<T>& operator=(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    _set(newValue);
    return *this;
  };

  // Sets the function that parses an incoming payload for this topic.
  MQTTGetSetFunction<T>& setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(*this);
    payloadHandler = handler;
    return *this;
  };

  // Attempts to parse the payload and sets the newValue variable.
  // For use by custom implementations of payload handlers.
  bool parseValue(const char* str, T* newValue) {
    return __internal::parseValue(str, newValue, MQTTFormattedTopic<T>::format);
  };

  // Attempts to set this topic's value from the supplied payload string.
  // Returns a ResultCode that indicates success or the reason of failure.
  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
    T newValue;
    if (!parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    _set(newValue);
    return ResultCode::OK;
  };

  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<const char*, U>::value, bool>::type* = nullptr>
  inline MQTTGetSetFunction<T>& operator=(const char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<char*, U>::value, bool>::type* = nullptr>
  inline MQTTGetSetFunction<T>& operator=(char* payload) { setFromPayload(payload); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<String, U>::value, bool>::type* = nullptr>
  inline MQTTGetSetFunction<T>& operator=(const String& payload) { setFromPayload(payload.c_str()); return *this; };
  template<typename U = T, typename std::enable_if<!std::is_const_v<U> && !std::is_same<std::string, U>::value, bool>::type* = nullptr>
  inline MQTTGetSetFunction<T>& operator=(const std::string& payload) { setFromPayload(payload.c_str()); return *this; };
  inline MQTTGetSetFunction<T>& operator=(const __FlashStringHelper* payload) { setFromPayload((String() + payload).c_str()); return *this; };
};

template<class T>
struct mqtt_getsetfunction_type { typedef MQTTGetSetFunction<T>& type; };
