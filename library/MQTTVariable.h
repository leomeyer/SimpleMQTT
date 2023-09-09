/////////////////////////////////////////////////////////////////////
// MQTTVariable
// The value is an external variable referred to via a pointer.
// Use for elementary data types only.
/////////////////////////////////////////////////////////////////////

template<typename T>
class MQTTVariable : public MQTTValue<T> {
friend class MQTTGroup;
friend class MQTTArray<T>;

protected:
  T* valuePtr;

public:
  MQTTVariable(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig, T* valPtr)
    : MQTTValue<T>(aParent, aTopic, aConfig),
      valuePtr(valPtr) { 
        if constexpr (!std::is_const_v<T>)
          MQTTValue<T>::value = *valPtr;
      };

  // for use in MQTTArray
  void setPointer(T* newPtr) {
    valuePtr = newPtr;
    if constexpr (!std::is_const_v<T>)
      MQTTValue<T>::value = *newPtr;
  }

protected:
  virtual MQTTVariable<T>* _setValue(T newValue) override {
    SIMPLEMQTT_CHECK_VALID(this);
    MQTTValue<T>::_setValue(newValue);
    if constexpr (!std::is_const_v<T>)
      *valuePtr = newValue;
    return this;
  };

  virtual bool _isEqual(T other) override {
    SIMPLEMQTT_CHECK_VALID(false);
    return *valuePtr == other;
  };

  virtual bool checkTopic() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTValue<T>::checkTopic())
      return false;
    if constexpr (!std::is_const_v<T>) {
      // detect underlying value change
      if (!_isEqual(MQTTValue<T>::value)) {
        MQTTValue<T>::value = *valuePtr;
        if (MQTTValue<T>::isAutoPublish())
          MQTTValue<T>::republish();
      }
    }
    return true;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTVariable<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTVariable<T>, T)

  virtual T get() override {
    SIMPLEMQTT_CHECK_VALID(T{});
    return *valuePtr;
  };
};

template<class T>
struct mqtt_variable { typedef MQTTVariable<T> type; };

template<class T>
struct mqtt_variable_type { typedef MQTTVariable<T>* type; };
