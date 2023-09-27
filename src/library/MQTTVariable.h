/////////////////////////////////////////////////////////////////////
// MQTTVariable: Topic class for variables referenced via pointer
// Use for fundamental data types only.
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// A SimpleMQTT topic using a pointer to a variable of fundamental data type T.
template<typename T>
class MQTTVariable : public MQTTValue<T> {
friend class MQTTGroup;
friend class MQTTArray<T*>;

protected:
  T* valuePtr;

  MQTTVariable(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, T* valPtr)
    : MQTTValue<T>(aParent, aTopic, aConfig),
      valuePtr(valPtr) { 
        if constexpr (!std::is_const_v<T>)
          MQTTValue<T>::_value = *valPtr;
      };

  inline String type() const override { 
    if constexpr (std::is_const_v<T>)
      return String("!*"); 
    return String("*"); 
  };

  // for use in MQTTArray
  void setPointer(T* newPtr) {
    valuePtr = newPtr;
    if constexpr (!std::is_const_v<T>)
      MQTTValue<T>::_value = *newPtr;
  };

  void _setValue(T newValue) override {
    SIMPLEMQTT_CHECK_VALID();
    if constexpr (!std::is_const_v<T>)
      *valuePtr = newValue;
  };

  bool check() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTValue<T>::check())
      return false;
    if constexpr (!std::is_const_v<T>) {
      // detect underlying value change
      if (!this->_isEqual(MQTTValue<T>::_value)) {
        MQTTValue<T>::_value = *valuePtr;
        if (MQTTValue<T>::isAutoPublish())
          MQTTValue<T>::republish();
      }
    }
    return true;
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTVariable<T>)
  SIMPLEMQTT_FORMAT_SETTER(MQTTVariable<T>, T)

  T* pointer() const {
    return valuePtr;
  }

  T value() const override {
    SIMPLEMQTT_CHECK_VALID(T{});
    return *valuePtr;
  };

  // Sets the current value of this topic.
  // Checks whether the new value is different from the current value
  // and sets the Changed flag on the topic if this is the case.
  template<typename U = T, typename std::enable_if<!std::is_const_v<U>, bool>::type* = nullptr> // only for non-const types
  inline MQTTVariable<T>& setTo(const T& newValue) {
    SIMPLEMQTT_CHECK_VALID(*this);
    bool changed = _set(newValue);
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return *this;
  };
};

template<class T>
struct mqtt_variable { typedef MQTTVariable<T> type; };

template<class T>
struct mqtt_variable_type { typedef MQTTVariable<T>& type; };
