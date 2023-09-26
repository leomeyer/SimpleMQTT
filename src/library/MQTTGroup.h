/////////////////////////////////////////////////////////////////////
// MQTTGroup: topic container class without a value of its own
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// A SimpleMQTT topic that represents a topic in a topic hierarchy.
// A group topic has no value of its own but manages a number of subtopics that can themselves be groups.
class MQTTGroup : public MQTTTopic {
friend class MQTTTopic;
friend class SimpleMQTTClient;

protected:
  typedef struct ListNode {
    MQTTTopic* data;
    ListNode* next;
  } ListNode;

  ListNode nodes{ nullptr, nullptr };
  TopicOrder topicOrder = TopicOrder::UNSPECIFIED;
  String topicPattern;
  String requestPattern;
  String setPattern;

  MQTTGroup(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTTopic(aParent, aTopic, aConfig) {};

  inline String type() const override { return String("+"); };

  String getTopicPattern() override {
    String result;
    if (_parent != nullptr)
      result = _parent->getTopicPattern();
    else
      result = topicPattern;
    if (result == "")
      return DEFAULT_TOPIC_PATTERN;
    return result;
  };

  virtual String getRequestPattern() {
    String result;
    if (_parent != nullptr)
      result = _parent->getRequestPattern();
    else
      result = requestPattern;
    if (result == "")
      return DEFAULT_REQUEST_PATTERN;
    return result;
  };

  virtual String applyRequestPattern(MQTTTopic* value) {
    String result = getRequestPattern();
    result.replace("%s", value->getFullTopic(getTopicOrder()));
    return result;
  };

  virtual String getSetPattern() {
    String result;
    if (_parent != nullptr)
      result = _parent->getSetPattern();
    else
      result = setPattern;
    if (result == "")
      return DEFAULT_SET_PATTERN;
    return result;
  };

  virtual String applySetPattern(MQTTTopic* value) {
    String fullTopic = value->getFullTopic(getTopicOrder());
    // top-level topics are "set" by listening to their topic directly
    if (fullTopic.startsWith("/"))
      return fullTopic;
    String result = "%s/set";
    result.replace("%s", fullTopic);
    return result;
  };

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

    ListNode* node = &nodes;
    while (node->next != nullptr) {
      // cannot add if an existing node already has the same topic
      if (node->data->getFullTopic() == value->getFullTopic()) {
        SIMPLEMQTT_ERROR(PSTR("A topic '%s' has already been added\n"), value->getFullTopic().c_str());
        return false;
      }
      node = node->next;
    }
    node->data = value;
    node->next = SIMPLEMQTT_ALLOCATE(ListNode);
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

  void publish(bool all = false) override {
    SIMPLEMQTT_CHECK_VALID();
    const ListNode* node = &nodes;
    while (node->next != nullptr) {
      MQTTTopic* value = node->data;
      if (all || value->needsPublish())
        value->publish(all);
      node = node->next;
    }
    config &= PUBLISH_CLEARMASK;
  };

  bool needsPublish() const override {
    SIMPLEMQTT_CHECK_VALID(false);
    bool result = ((config >> PUBLISH_BIT) & 1) == 1;
    if (result)
      return true;
    const ListNode* node = &nodes;
    while (node->next != nullptr) {
      MQTTTopic* value = node->data;
      if (value->needsPublish()) {
//        SIMPLEMQTT_DEBUG(PSTR("Needs publish: '%s', config: %s\n"), value->getFullTopic().c_str(), value->getConfigStr());
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

  bool check() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::check())
      return false;
    // recursively check subtopics
    ListNode* node = &nodes;
    while (node->next != nullptr) {
      node->data->check();
      node = node->next;
    }
    return true;
  };

  virtual void addSubscriptions(SimpleMQTTClient* client) override;
  
  virtual bool processPayload(SimpleMQTTClient* client, const char* topic, const char* payload) override;

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTGroup)

  bool isSettable() const override {
    return false;
  };

  // Returns the topic order used by this group.
  // The topic order determines how the full path of a topic is calculated.
  virtual TopicOrder getTopicOrder() {
    if (_parent == nullptr)
      return topicOrder;
    if (topicOrder == TopicOrder::UNSPECIFIED)
      return _parent->getTopicOrder();
    return topicOrder;
  };

  // Sets the topic order used by this group.
  // The topic order determines how the full path of a topic is calculated.
  virtual MQTTGroup& setTopicOrder(TopicOrder order) {
    topicOrder = order;
    return *this;
  };

  // Sets the general topic pattern used by this group.
  // The string %s in the pattern is replaced by a topic's full path in the topic hierarchy.
  virtual MQTTGroup& setTopicPattern(const String& pattern) {
    topicPattern = pattern;
    return *this;
  };

  // Sets the request topic pattern used by this group.
  // The string %s in the pattern is replaced by a topic's full path in the topic hierarchy.
  virtual MQTTGroup& setRequestPattern(const String& pattern) {
    requestPattern = pattern;
    return *this;
  };

  // Sets the set topic pattern used by this group.
  // The string %s in the pattern is replaced by a topic's full path in the topic hierarchy.
  virtual MQTTGroup& setSetPattern(const String& pattern) {
    setPattern = pattern;
    return *this;
  };

  // Causes all subtopics of this group to be published to the broker
  // on the next call of the handle() function.
  void republish() {
    SIMPLEMQTT_CHECK_VALID();
    if (!MQTTTopic::isTopicValid())
      return;
    ListNode* node = &nodes;
    while (node->next != nullptr) {
      node->data->republish();
      node = node->next;
    }
  };

  // helper templates to allow passing macro arguments with commas in brackets
  template<typename T> struct argument_type;
  template<typename T, typename U> struct argument_type<T(U)> { typedef U type; };

#define SIMPLEMQTT_ADD_BODY(typeName, className, ...) \
  if (this == __internal::INVALID_PTR || !topic.isValid()) { \
    topic.release(); \
    return (typename argument_type<void(typeName)>::type)*((typename argument_type<void(className)>::type*)__internal::INVALID_PTR); \
  } \
  typename argument_type<void(className)>::type* result = SIMPLEMQTT_ALLOCATE(typename argument_type<void(className)>::type, __VA_ARGS__); \
  if (result != __internal::INVALID_PTR && addOrFree(result)) { \
    SIMPLEMQTT_DEBUG(PSTR("Added object of class %s for type %s, topic '%s'\n"), #className, #typeName, topic.get()); \
    return *result; \
  } \
  topic.release(); \
  return ((typename argument_type<void(typeName)>::type)*((typename argument_type<void(className)>::type*)__internal::INVALID_PTR))

  // Adds a new group with the specified topic name.
  MQTTGroup& add(__internal::_Topic topic) override {
    SIMPLEMQTT_ADD_BODY(MQTTGroup&, MQTTGroup, this, topic, getConfig());
  };

  #define IS_FUNDAMENTAL  std::is_fundamental_v<T>
  #define NOT_FUNDAMENTAL !std::is_fundamental_v<T>
  #define IS_CONST        std::is_const_v<T>
  #define NOT_CONST       !std::is_const_v<T>
  #define IS_POINTER      std::is_pointer_v<T>
  #define NOT_POINTER     !std::is_pointer_v<T>
  #define IS_ARRAY        std::is_array_v<T>
  #define NOT_ARRAY       !std::is_array_v<T>
  #define NOT_STRING      !std::is_same_v<T, char*> && !std::is_same_v<T, const char*>

  // Adds a new non-const value topic with the specified name and data type.
  template<typename T>
  typename std::enable_if_t<NOT_CONST, typename mqtt_value_type<T>::type> add(__internal::_Topic topic) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<T>::type, MQTTValue<T>, this, topic, getConfig());
  };

  // Adds a new non-const fundamental data type value topic with the specified name and initial value.
  template<typename T>
  typename std::enable_if_t<IS_FUNDAMENTAL && NOT_CONST, typename mqtt_value_type<T>::type> add(__internal::_Topic topic, T initialValue) {
    typename mqtt_value_type<T>::type result = add<T>(topic);
    if (result.name()[0] != '\0')
      result._set(initialValue);
    return result;
  };

  // Adds a new non-const non-fundamental data type value topic with the specified name and initial value.
  template<typename T, typename V = T>
  typename std::enable_if_t<NOT_FUNDAMENTAL && NOT_CONST && NOT_POINTER && NOT_ARRAY, typename mqtt_value_type<T>::type> add(__internal::_Topic topic, V initialValue) {
    typename mqtt_value_type<T>::type result = add<T>(topic);
    if (result.name()[0] != '\0')
      result = initialValue;
    return result;
  };

  // Adds a new fundamental data type variable topic with the specified name pointing to the given variable.
  // The types char* and const char* do not apply. These add string topics instead. The variable may be const.
  template<typename T>
  typename std::enable_if_t<IS_POINTER && NOT_ARRAY && NOT_STRING, typename mqtt_variable_type<std::remove_pointer_t<T>>::type> add(__internal::_Topic topic, T val) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_variable_type<std::remove_pointer_t<T>>::type, MQTTVariable<std::remove_pointer_t<T>>, this, topic, getConfig(), val);
  };

  // Adds a new fundamental data type value array topic with the specified name and the given element count.
  // The data type may not be const.
  template<typename T, size_t N>
  typename std::enable_if_t<IS_FUNDAMENTAL && NOT_CONST && (N > 0), typename mqtt_valuearray_type<T, N>::type> add(__internal::_Topic topic) {
    SIMPLEMQTT_ADD_BODY((typename mqtt_valuearray_type<T, N>::type), (MQTTValueArray<T, N>), this, topic, getConfig());
  };
  
  // Adds a new fundamental data type array topic with the specified name and the given element count.
  // The data type may be const.
  template<typename T>
  typename std::enable_if_t<std::is_fundamental_v<std::remove_pointer_t<T>>, typename mqtt_array_type<T>::type> add(__internal::_Topic topic, T array, size_t elementCount) {
    if (elementCount == 0)
      return (typename mqtt_array_type<T>::type)__internal::INVALID_PTR;
    SIMPLEMQTT_ADD_BODY(typename mqtt_array_type<T>::type, MQTTArray<T>, this, topic, getConfig(), array, elementCount);
  };

  // Adds a new fundamental data type array topic with the specified name.
  // The types char* and const char* do not apply. These add string topics instead. The data type may be const.
  template<typename T, size_t N>
  typename std::enable_if_t<IS_FUNDAMENTAL && NOT_STRING && (N > 1), typename mqtt_array_type<std::add_pointer_t<T>>::type> add(__internal::_Topic topic, T (&array)[N]) {
    return add(topic, array, N);
  };

  // Adds a new reference data type topic with the specified name.
  // The types char* and const char* do not apply. These add string topics instead. The data type may be const.
  template<typename T, size_t N>
  typename std::enable_if_t<std::is_reference_v<T&> && (N == 1) && NOT_STRING, typename mqtt_reference_type<T>::type> add(__internal::_Topic topic, T (&val)[N]) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_reference_type<T>::type, MQTTReference<T>, this, topic, getConfig(), val);
  };

  // Adds a new reference data type topic with the specified name. The data type may be const.
  template<typename T>
  typename std::enable_if_t<std::is_lvalue_reference_v<T&> && NOT_ARRAY && NOT_FUNDAMENTAL, typename mqtt_reference_type<T>::type> add(__internal::_Topic topic, T& val) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_reference_type<T>::type, MQTTReference<T>, this, topic, getConfig(), val);
  };

  // Adds a new value data type topic with the specified name. The value is copied into the topic's value. The data type may not be const.
  template<typename T, typename V>
  typename std::enable_if_t<!std::is_same_v<T, V> && std::is_rvalue_reference_v<V&> && NOT_ARRAY && NOT_FUNDAMENTAL, typename mqtt_value_type<T>::type> add(__internal::_Topic topic, V& val) {
    SIMPLEMQTT_ADD_BODY(typename mqtt_value_type<T>::type, MQTTValue<T>, this, topic, getConfig(), val);
  };

  // Adds a new string topic from the given string. The topic value is settable and has a fixed length.
  MQTTCharArray& add(__internal::_Topic topic, char* s);

  // Adds a new string topic from the given string. The topic value is not settable.
  MQTTConstCharArray& add(__internal::_Topic topic, const char* s);

  #if SIMPLEMQTT_JSON_BUFFERSIZE > 0
  // Adds a new Json topic with an optional filter specifying the JSON nodes of interest.
  // The filter document's content can be changed if necessary.
  MQTTJsonTopic& addJsonTopic(__internal::_Topic topic, JsonDocument* filter = nullptr) {
    SIMPLEMQTT_ADD_BODY(MQTTJsonTopic&, MQTTJsonTopic, this, topic, getConfig(), filter);
  };
  #endif

  // Returns the number of child topics in this group.
  const size_t size() const {
    SIMPLEMQTT_CHECK_VALID(0);
    const ListNode* node = &nodes;
  	size_t c = 0;
    while (node->next != nullptr) {
      c++;
      node = node->next;
    }
	  return c;
  };
  
  // Returns a subtopic by name or path
  MQTTTopic& get(const String& key, bool autoCreate = false) override {
    SIMPLEMQTT_CHECK_VALID(MQTTTopic::INVALID_TOPIC);
    if (key.length() == 0)
      return *this;
    String part{key};
    String rest;
    int i = key.indexOf("/", 1);
    if (i > 0) {
      part = part.substring(0, i);
      rest = key.substring(i + 1);
    }
    const ListNode* node = &nodes;
    while (node->next != nullptr) {
	  if (part.equals(node->data->name()))
	    return node->data->get(rest, autoCreate);
      node = node->next;
    }
    // not found; create?
    if (autoCreate) {
      MQTTTopic& result = add(part);
      return result.get(rest);
    }
	  return MQTTTopic::INVALID_TOPIC;
  };

  // bracket operators use implicit auto-create of missing group topics
  inline MQTTTopic& operator[](const char* key) { return get(String() + key, true); };
  inline MQTTTopic& operator[](char* key) { return get(String() + key, true); };
  inline MQTTTopic& operator[](const String& key) { return get(key, true); };
  inline MQTTTopic& operator[](const __FlashStringHelper* key) { return get(String() + key, true); };
  
  // Returns the subtopic at position i. May return nullptr if the index is out of bounds.
  MQTTTopic* get(size_t i) {
    SIMPLEMQTT_CHECK_VALID(nullptr);
    const ListNode* node = &nodes;
	  size_t c = 0;
    while (node->next != nullptr) {
      if (i == c++)
		    return node->data;
      node = node->next;
    }
	  return nullptr;
  };

  // Returns the subtopic at position i. May return nullptr if the index is out of bounds.
  inline MQTTTopic* operator[](size_t i) {
	  return get(i);
  };
  
  bool hasBeenChanged(bool b) const override {
    SIMPLEMQTT_CHECK_VALID(false);
    const ListNode* node = &nodes;
    while (node->next != nullptr) {
	  if (node->data->hasBeenChanged(b))
        return true;
      node = node->next;
    }
    return false;
  };

  // Returns the first subtopic that has been changed (depth-first) or nullptr if there are no changed subtopics.
  MQTTTopic* getChange() override {
    SIMPLEMQTT_CHECK_VALID(nullptr);
    ListNode* node = &nodes;
    while (node->next != nullptr) {
	  if (node->data->hasBeenChanged(false))
        return node->data->getChange();
      node = node->next;
    }
  	return nullptr;
  };

  // Prints information about this topic group to the specified Print object.
  size_t printTo(Print& p, size_t indent) const override {
    size_t n = 0;
    for (size_t i = 0; i < indent; i++)
      n += p.print(" ");
    if (name()[0] == '\0') {
      p.print(F("INVALID"));
    } else {
      n += p.print(type().c_str());
      n += p.print(name());
      n += p.print(" (");
      n += p.print(getConfigStr());
      n += p.println("): {");
      const ListNode* node = &nodes;
      while (node->next != nullptr) {
        n += node->data->printTo(p, indent + 2);
        node = node->next;
      }
      for (size_t i = 0; i < indent; i++)
        n += p.print(" ");
      n += p.print("} (");
      n += p.print(name());
      n += p.println(")");
    }
    return n;
  };

  // Prints information about this topic group to the specified Print object.
  // Implements the necessary method of the Printable interface.
  inline size_t printTo(Print& p) const override {
    return printTo(p, 0);
  };
};

// add function template specializations

// Adds a new string topic referring to the given string variable. The topic value is settable and has a fixed length.
template<>
MQTTCharArray& MQTTGroup::add<char*>(__internal::_Topic topic, char* array, size_t length) {
  SIMPLEMQTT_ADD_BODY(MQTTCharArray&, MQTTCharArray, this, topic, getConfig(), array, length);
}

// Adds a new string topic from the given string. The topic value is not settable.
template<>
MQTTConstCharArray& MQTTGroup::add<const char*>(__internal::_Topic topic, const char* array, size_t len) {
  SIMPLEMQTT_ADD_BODY(MQTTConstCharArray&, MQTTConstCharArray, this, topic, getConfig(), array, len);
}

// convenience function
MQTTCharArray& MQTTGroup::add(__internal::_Topic topic, char* s) {
  return add<char*>(topic, s, strlen(s));
}

// convenience function
MQTTConstCharArray& MQTTGroup::add(__internal::_Topic topic, const char* s) {
  return add<const char*>(topic, s, strlen(s));
}
