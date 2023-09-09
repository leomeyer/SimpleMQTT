/////////////////////////////////////////////////////////////////////
// MQTTReference
// The value is referred to by reference.
// Can be used for fundamental and complex types.
/////////////////////////////////////////////////////////////////////

template<typename T>
class MQTTReference : public MQTTValue<T> {
friend class MQTTGroup;

protected:
  typedef ResultCode (*PayloadHandler)(MQTTReference<T>* object, const char* payload);
  PayloadHandler payloadHandler = [](MQTTReference<T>* object, const char* payload) {
    return object->setFromPayload(payload);
  };
  T& valueRef;

  MQTTReference(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, T& aValue)
    : MQTTValue<T>(aParent, aTopic, aConfig),
      valueRef(aValue) {};

  MQTTReference(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, const char* str)
    : MQTTValue<T>(aParent, aTopic, aConfig), valueRef(MQTTValue<T>::value) {
        setFromPayload(str);
      };

  virtual MQTTReference<T>* _setValue(T newValue) override {
    SIMPLEMQTT_CHECK_VALID(this);
    if constexpr (!std::is_const_v<T>)
      valueRef = newValue;
    return this;
  };

  virtual bool _isEqual(T other) override {
    SIMPLEMQTT_CHECK_VALID(false);
    return valueRef == other;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTReference<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTReference<T>, T)

  virtual bool isSettable() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if constexpr (std::is_const_v<T>)
      return false;
    return MQTTTopic::isSettable();
  };

  virtual MQTTReference<T>* setPayloadHandler(PayloadHandler handler) {
    SIMPLEMQTT_CHECK_VALID(this);
    payloadHandler = handler;
    return this;
  };

  virtual ResultCode setFromPayload(const char* payload) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    return ResultCode::CANNOT_SET;
  };

  virtual T get() override {
    return valueRef;
  };

  virtual String getPayload() override {
    SIMPLEMQTT_CHECK_VALID(String());
    String s = __internal::formatValue(get(), MQTTFormattedTopic<T>::format);
    return s;
  };
};

template<class T>
struct mqtt_reference_type { typedef MQTTReference<T>* type; };

/////////////////////////////////////////////////////////////////////
// MQTTReference<T> specializations
/////////////////////////////////////////////////////////////////////

template<>
ResultCode MQTTReference<String>::setFromPayload(const char* payload) {
  SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
  String newValue = String(payload);
  _set(newValue, true);
  return ResultCode::OK;
};
