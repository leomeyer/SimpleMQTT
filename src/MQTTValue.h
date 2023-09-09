/////////////////////////////////////////////////////////////////////
// MQTTValue
// The value is managed by this class.
/////////////////////////////////////////////////////////////////////

template<typename T>
class MQTTValue : public MQTTFormattedTopic<T> {
friend class MQTTGroup;
friend class MQTTArray<T*>;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTValue<T>* object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTValue<T>* object, const char* payload) {
    return object->setFromPayload(payload);
  };
  T value{};

  MQTTValue(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig) {};

  MQTTValue(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, const char* str)
    : MQTTFormattedTopic<T>(aParent, aTopic, aConfig) {
      setFromPayload(str);
    };

  virtual MQTTValue<T>* _setValue(T newValue) {
    SIMPLEMQTT_CHECK_VALID(this);
    if constexpr (!std::is_const_v<T>)
      value = newValue;
    return this;
  };

  virtual bool _isEqual(T other) {
    SIMPLEMQTT_CHECK_VALID(false);
    return value == other;
  };

  virtual bool _set(T const& newValue, bool publish) {
    SIMPLEMQTT_CHECK_VALID(false);
    if constexpr (std::is_const_v<T>)
      return false;
    bool changed = !_isEqual(newValue);
    _setValue(newValue);
    if (MQTTTopic::isAutoPublish() || publish)
      MQTTTopic::republish();
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return changed;
  };

  ResultCode setReceived(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    MQTTTopic::republish();
    if (payloadHandler != nullptr)
      return payloadHandler(this, payload);
    return setFromPayload(payload);
  };

  virtual ResultCode setFromPayload(const char* payload) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG(PSTR("setFromPayload: %s\n"), payload);
    if constexpr (std::is_const_v<T>)
      return ResultCode::CANNOT_SET;
    T newValue{};
    if (!parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    set(newValue, true);
    return ResultCode::OK;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTValue<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTValue<T>, T)

  virtual bool isSettable() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if constexpr (std::is_const_v<T>)
      return false;
    return MQTTTopic::isSettable();
  };

  virtual T get() {
    SIMPLEMQTT_CHECK_VALID(T{});
    return value;
  };

  virtual bool set(T const& newValue, bool publish = false) {
    SIMPLEMQTT_CHECK_VALID(false);
    return _set(newValue, publish);
  };

  virtual MQTTValue<T>* setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(this);
    payloadHandler = handler;
    return this;
  };

  virtual bool parseValue(const char* str, T* newValue) {
//    SIMPLEMQTT_DEBUG(PSTR("parseValue: %s\n"), str);
    if constexpr (std::is_const_v<T>)
      return false;
    return __internal::parseValue(str, newValue, MQTTFormattedTopic<T>::format);
  };

  virtual String getPayload() {
    SIMPLEMQTT_CHECK_VALID(String());
    String s = __internal::formatValue(get(), MQTTFormattedTopic<T>::format);
    return s;
  };
};

template <>
bool MQTTValue<const char*>::parseValue(const char* str, const char** newValue) {
    return false;
};

template<class T>
struct mqtt_value_type { typedef MQTTValue<T>* type; };

