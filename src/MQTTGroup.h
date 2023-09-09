/////////////////////////////////////////////////////////////////////
// MQTTGroup
/////////////////////////////////////////////////////////////////////

class MQTTGroup : public MQTTTopic {
friend class SimpleMQTTClient;

protected:
  MQTTTopic::ListNode nodes{ nullptr, nullptr };

  MQTTGroup(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTTopic(aParent, aTopic, aConfig) {};

  bool addNode(MQTTTopic* value) {
    if (value == __internal::INVALID_PTR) {
      SIMPLEMQTT_ERROR(PSTR("Attempting to add an invalid element, ignoring\n"));
      return true;
    }
    if (this == __internal::INVALID_PTR) {
      SIMPLEMQTT_ERROR(PSTR("MQTTGroup is invalid, cannot add element\n"));
      return false;
    } 
    if (!isTopicValid()) {
      SIMPLEMQTT_ERROR(PSTR("MQTTGroup topic '%s' is invalid, cannot add element\n"), topic.get());
      return false;
    }
    if (!value->isTopicValid()) {
      SIMPLEMQTT_ERROR(PSTR("Element topic '%s' is invalid, cannot add\n"), value->topic.get());
      return false;
    }

    MQTTTopic::ListNode* node = &nodes;
    while (node->next != nullptr) {
      // cannot add if an existing node already has the same topic
      if (node->data->getFullTopic() == value->getFullTopic()) {
        SIMPLEMQTT_ERROR(PSTR("A topic '%s' has already been added\n"), value->getFullTopic().c_str());
        return false;
      }
      node = node->next;
    }
    node->data = value;
    node->next = SIMPLEMQTT_ALLOCATE(MQTTTopic::ListNode);
    if (node->next == __internal::INVALID_PTR) {
        SIMPLEMQTT_ERROR(PSTR("Not enough memory for internal list, element '%s'\n"), value->getFullTopic().c_str());
      node->data = nullptr;
      node->next = nullptr;
      return false;
    }
    if (value->isAutoPublish())
      value->republish();
    return true;
  };

  virtual void publish(bool all = false) {
    SIMPLEMQTT_CHECK_VALID();
    MQTTTopic::ListNode* node = &nodes;
    while (node->next != nullptr) {
      MQTTTopic* value = node->data;
      if (all || value->needsPublish())
        value->publish(all);
      node = node->next;
    }
    config &= PUBLISH_CLEARMASK;
  };

  virtual bool needsPublish() {
    bool result = ((config >> PUBLISH_BIT) & 1) == 1;
    if (result)
      return true;
    MQTTTopic::ListNode* node = &nodes;
    while (node->next != nullptr) {
      MQTTTopic* value = node->data;
      if (value->needsPublish()) {
        // SIMPLEMQTT_DEBUG(PSTR("Needs publish: '%s', config: %s\n"), value->getFullTopic().c_str(), value->getConfigStr());
        return true;
      }
      node = node->next;
    }
    return false;
  };

  template<typename T>
  bool addOrFree(T* node) {
    if (addNode(node))
      return true;
    SIMPLEMQTT_DEALLOCATE(node);
    return false;
  };

  virtual void addSubscriptions(SimpleMQTTClient* client) override;
  
  virtual bool processPayload(SimpleMQTTClient* client, const char* topic, const char* payload) override;

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTGroup)

  // a group is not settable (for now)
  inline bool isSettable() {
    return false;
  };

  virtual bool setFromPayload(const char*) {
    return false;
  };

  String getPayload() {
    return String();
  };

  void republish() {
    SIMPLEMQTT_CHECK_VALID();
    if (!MQTTTopic::isTopicValid())
      return;
    MQTTTopic::ListNode* node = &nodes;
    while (node->next != nullptr) {
      node->data->republish();
      node = node->next;
    }
  };

  bool checkTopic() {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::checkTopic())
      return false;
    // recursively check subtopics
    MQTTTopic::ListNode* node = &nodes;
    while (node->next != nullptr) {
      node->data->checkTopic();
      node = node->next;
    }
    return true;
  };

#define SIMPLEMQTT_ADD_BODY(typeName, className, ...) \
  if (this == __internal::INVALID_PTR || !topic.isValid()) { \
    topic.release(); \
    return (typeName)__internal::INVALID_PTR; \
  } \
  typeName result = SIMPLEMQTT_ALLOCATE(className, __VA_ARGS__); \
  if (result != __internal::INVALID_PTR && addOrFree(result)) { \
    SIMPLEMQTT_DEBUG(PSTR("Added object of class %s for type %s, topic '%s'\n"), #className, #typeName, topic.get()); \
    return result; \
  } \
  topic.release(); \
  return (typeName)__internal::INVALID_PTR;

  // group, by topic
  MQTTGroup* add(__internal::_Topic topic) {
    SIMPLEMQTT_ADD_BODY(MQTTGroup*, MQTTGroup, this, topic, getConfig())
  };

  // necessary template definition for non-fundamental type values
  template<typename T>
  typename std::enable_if<!std::is_fundamental<T>::value, typename mqtt_value_type<T>::type>::type add(__internal::_Topic topic) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<T>::type, MQTTValue<T>, this, topic, getConfig())
  };

  // simple value, fundamental type  
  template<typename T>
  typename std::enable_if<std::is_fundamental<T>::value, typename mqtt_value_type<T>::type>::type add(__internal::_Topic topic) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<T>::type, MQTTValue<T>, this, topic, getConfig())
  };

  // simple value, fundamental type, initialized
  template<typename T>
  typename std::enable_if<std::is_fundamental<T>::value, typename mqtt_value_type<T>::type>::type add(__internal::_Topic topic, T initialValue) {
    typename mqtt_value_type<T>::type result = add<T>(topic);
    if (result != __internal::INVALID_PTR) 
      result->_setValue(initialValue);
    return result;
  };

  // variable, referenced by pointer
  template<typename T>
  typename std::enable_if<std::is_fundamental<T>::value && !std::is_same_v<T, const char>, typename mqtt_variable_type<T>::type>::type add(__internal::_Topic topic, T* val) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_variable_type<T>::type, MQTTVariable<T>, this, topic, getConfig(), val)
  };

  // array, referenced by pointer
  template<typename T>
  typename std::enable_if<std::is_pointer<T>::value && std::is_fundamental<std::remove_pointer_t<T>>::value, typename mqtt_array_type<T>::type>::type add(__internal::_Topic topic, T array, size_t elementCount) {
    if (elementCount == 0)
      return (typename mqtt_array_type<T>::type)__internal::INVALID_PTR;
    SIMPLEMQTT_ADD_BODY(typename mqtt_array_type<T>::type, MQTTArray<T>, this, topic, getConfig(), array, elementCount)
  };

  // reference variable
  template<typename T>
  typename std::enable_if<std::is_reference<T&>::value && !std::is_same_v<T, const char*>, typename mqtt_reference_type<T>::type>::type add(__internal::_Topic topic, T& val) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_reference_type<T>::type, MQTTReference<T>, this, topic, getConfig(), val)
  };

/*
  // reference, initialized by string
  template<typename T>
  typename std::enable_if<std::is_reference<T&>::value && !std::is_same_v<T, const char*>, typename mqtt_reference_type<T>::type>::type add(__internal::_Topic topic, const char* str) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_reference_type<T>::type, MQTTReference<T>, this, topic, getConfig(), str)
  };
*/
  // String from char[] constant
  template<size_t N>
  typename mqtt_value_type<String>::type add(__internal::_Topic topic, const char (&str)[N]) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<String>::type, MQTTValue<String>, this, topic, getConfig(), str)
  };
};

// add function specializations

// char arrays
template<>
MQTTCharArray* MQTTGroup::add<char*>(__internal::_Topic topic, char* array, size_t len) {
  SIMPLEMQTT_ADD_BODY(MQTTCharArray*, MQTTCharArray, this, topic, getConfig(), array, len)
}

template<>
MQTTConstCharArray* MQTTGroup::add<const char*>(__internal::_Topic topic, const char* array, size_t len) {
  SIMPLEMQTT_ADD_BODY(MQTTConstCharArray*, MQTTConstCharArray, this, topic, getConfig(), array, len)
}

template<>
typename mqtt_value_type<String>::type MQTTGroup::add<String>(__internal::_Topic topic) {
  SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<String>::type, MQTTValue<String>, this, topic, getConfig())
}
