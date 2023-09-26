/////////////////////////////////////////////////////////////////////
// MQTTTopic: Base class for topic management
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// Base class for SimpleMQTT topics.
class MQTTTopic
#ifdef SIMPLEMQTT_TOPICS_PRINTABLE
  : public Printable
#endif 
{
friend class SimpleMQTTClient;
friend class MQTTGroup;

protected:
  MQTTGroup* _parent;
  __internal::_Topic topic;
  uint8_t config;  // MSB 6 5 4 3 2 1 LSB
                   // ^   ^ ^ ^ ^ ^ ^ ^
                   // |   | | | | | + QoS
                   // |   | | | | + requestable
                   // |   | | | + settable
                   // |   | | + auto-publish
                   // |   | + changed by set message
                   // |   + publish required
                   // + retained

  // prohibit default copy semantics, forcing the user to work with references or pointers
  MQTTTopic(const MQTTTopic&) = delete;
  void operator=(const MQTTTopic&) = delete;

  MQTTTopic(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : _parent(aParent), topic(aTopic), config(aConfig) {
      // topics that start with a slash (top level topics) are by default not requestable and not auto-publishing
      if (name()[0] == '/') {
        setRequestable(false);
        setAutoPublish(false);
      }
    };

  virtual inline String type() const { return String(); };

  inline uint8_t getConfig() {
    SIMPLEMQTT_CHECK_VALID(0);
    return config;
  };

  String getConfigStr() const {
    String result;
    result += (isRetained() ? "R" : "-");
    result += (needsPublish() ? "P" : "-");
    result += (hasBeenChanged(false) ? "C" : "-");
    result += (isAutoPublish() ? "A" : "-");
    result += (isSettable() ? "S" : "-");
    result += (isRequestable() ? "Q" : "-");
    result += getQoS();
    return result;
  }

  bool isTopicValid() {
    SIMPLEMQTT_CHECK_VALID(false);
    return topic.isValid();
  };

  virtual bool check() {
    SIMPLEMQTT_CHECK_VALID(false);
    return isTopicValid();
  };

  virtual String getTopicPattern();

  virtual ResultCode requestReceived(const char*) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    republish();
    return ResultCode::OK;
  };

  virtual ResultCode setReceived(const char*) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    if (isAutoPublish())
      republish();
    return ResultCode::CANNOT_SET;
  };

  MQTTTopic& setChanged(bool changed) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= CHANGED_CLEARMASK;
    config |= (changed ? CHANGED_SETMASK : 0);
    return *this;
  };

  void releaseTopic() {
    topic.release();
  }

  virtual MQTTTopic& get(const String& key, bool autoCreate = false) {
    SIMPLEMQTT_CHECK_VALID(MQTTTopic::INVALID_TOPIC);
    if (key.length() == 0)
      return *this;
    return MQTTTopic::INVALID_TOPIC;
  };

  virtual MQTTTopic* getChange() {
    SIMPLEMQTT_CHECK_VALID(nullptr);
    bool changed = ((config >> CHANGED_BIT) & 1) == 1;
    config &= CHANGED_CLEARMASK;
  	if (changed)
      return this;
    return nullptr;
  };
  
  virtual void publish(bool all = false);

  virtual void addSubscriptions(SimpleMQTTClient* client);

  virtual bool processPayload(SimpleMQTTClient* client, const char* topic, const char* payload);

public:
  static MQTTTopic INVALID_TOPIC;

  // Returns the group topic that this topic belongs to.
  virtual MQTTGroup& parent() {
    return *_parent;
  };

  // Returns the client used by this topic. May return nullptr.
  virtual SimpleMQTTClient* getClient();

  virtual MQTTGroup& add(__internal::_Topic topic) {
    return (MQTTGroup&)INVALID_TOPIC;
  };

  // Returns a pointer to the name of this topic.
  // Important: Do not store this pointer for longer as the memory area might be used for other purposes.
  // It is better to always use name() if you need the name of a topic.
  inline const char* name() const {
    SIMPLEMQTT_CHECK_VALID(__internal::EMPTY);
    return topic.get();
  };

  inline operator const char*() const {
	  return name();
  };

  // Sets the Quality of Service for this topic. A value between 0 and 2.
  // Only has an effect before the first call of the handle() function.
  virtual MQTTTopic& setQoS(uint8_t qos) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= ~0b11;
    config |= (qos > 2 ? 2 : qos);
    return *this;
  };

  // Returns the Quality of Service for this topic.
  virtual uint8_t getQoS() const {
    SIMPLEMQTT_CHECK_VALID(0);
    return config & 0b11;
  };

  // Sets the Retained flag for this topic.
  // Only has an effect before the first call of the handle() function.
  virtual MQTTTopic& setRetained(bool retained) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= RETAINED_CLEARMASK;
    config |= (retained ? RETAINED_SETMASK : 0);
    return *this;
  };

  // Returns the value of the Retained flag for this topic.
  virtual bool isRetained() const {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> RETAINED_BIT) & 1) == 1;
  };

  // Sets whether this topic is auto-publishing.
  virtual MQTTTopic& setAutoPublish(bool autoPublish) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= AUTO_PUBLISH_CLEARMASK;
    config |= (autoPublish ? AUTO_PUBLISH_SETMASK : 0);
    return *this;
  };

  // Returns whether this topic is auto-publishing.
  virtual bool isAutoPublish() const {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> AUTO_PUBLISH_BIT) & 1) == 1;
  };

  // Sets whether this topic is requestable.
  // Requestable topics will generate a subscription to their individual request topic.
  // Only has an effect before the first call of the handle() function.
  virtual MQTTTopic& setRequestable(bool requestable) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= REQUESTABLE_CLEARMASK;
    config |= (requestable ? REQUESTABLE_SETMASK : 0);
    return *this;
  };

  // Returns whether this topic is requestable.
  virtual bool isRequestable() const {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> REQUESTABLE_BIT) & 1) == 1;
  };

  // Returns the request topic for this topic.
  // Is only used if the topic is requestable.
  virtual String getRequestTopic();

  // Sets whether this topic is settable.
  // Settable topics will generate a subscription to their individual set topic.
  // Only has an effect before the first call of the handle() function.
  virtual MQTTTopic& setSettable(bool settable) {
    SIMPLEMQTT_CHECK_VALID(*this);
    config &= SETTABLE_CLEARMASK;
    config |= (settable ? SETTABLE_SETMASK : 0);
    return *this;
  };

  // Returns whether this topic is settable.
  virtual bool isSettable() const {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> SETTABLE_BIT) & 1) == 1;
  };

  // Returns the set topic for this topic.
  // Is only used if the topic is settable.
  virtual String getSetTopic();

  // Attempts to set this topic's value from the supplied payload string.
  // Returns a ResultCode that indicates success or the reason of failure.
  virtual ResultCode setFromPayload(const char* payload) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;
    return ResultCode::CANNOT_SET;
  };

  // Returns the current value of this topic as a String.
  virtual String getPayload() const {
    return String();
  };

  // Sets a flag that indicates that this topic should be published to the broker
  // on the next call of the handle() function.
  virtual void republish() {
    SIMPLEMQTT_CHECK_VALID();
    // set flag to re-publish
    config |= PUBLISH_SETMASK;
  };

  // Returns whether this topic should be published to the broker
  // on the next call of the handle() function.
  virtual bool needsPublish() const {
    SIMPLEMQTT_CHECK_VALID(false);
    bool result = ((config >> PUBLISH_BIT) & 1) == 1;
    return result;
  };

  // Returns the full name of this topic as it should appear in the topic hierarchy
  // if the given TopicOrder is being used.
  virtual String getFullTopic(TopicOrder order); // see MQTTImpl.h

  // Returns the full name of this topic as it should appear in the topic hierarchy
  // using the current TopicOrder specified for the topic's group.
  virtual String getFullTopic(); // see MQTTImpl.h

  // Returns whether this topic has been changed since the last call to hasBeenChanged().
  // It makes no difference whether the topic has been changed via MQTT or by calling
  // the set() method or the assignment operator.
  // Clears the flag indicating that the topic has been changed.
  virtual bool hasBeenChanged() {
    SIMPLEMQTT_CHECK_VALID(false);
    bool result = ((config >> CHANGED_BIT) & 1) == 1;
    config &= CHANGED_CLEARMASK;
    return result;
  };
  
  // Returns whether this topic has been changed.
  // It makes no difference whether the topic has been changed via MQTT or by calling
  // the set() method or the assignment operator.
  // Does not clears the flag indicating that the topic has been changed.
  virtual bool hasBeenChanged(bool) const {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> CHANGED_BIT) & 1) == 1;
  };
  
  // Prints information about this topic to the specified Print object.
  virtual size_t printTo(Print& p, size_t indent) const {
    size_t n = 0;
    for (size_t i = 0; i < indent; i++)
      n += p.print(" ");
    if (name()[0] == '\0') {
      p.print(F("INVALID\n"));
    } else {
      n += p.print(type().c_str());
      n += p.print(name());
      n += p.print(" (");
      n += p.print(getConfigStr());
      n += p.print("): ");
      n += p.println(getPayload());
    }
    return n;
  };

  // Prints information about this topic to the specified Print object.
  // Implements the necessary method of the Printable interface.
  virtual inline size_t printTo(Print& p) const {
    return printTo(p, 0);
  };
};


// Base class for SimpleMQTT topics that can have a certain input and output format.
// The type of format depends on the underlying data type used by the topic.
template<typename T>
class MQTTFormattedTopic : public MQTTTopic {
protected:
  typename format_type<T>::type format = getDefaultFormat<typename format_type<T>::type>();

  MQTTFormattedTopic(MQTTGroup* a_parent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTTopic(a_parent, aTopic, aConfig) {};

public:
  bool isSettable() const override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::isSettable())
      return false;
    return !std::is_const_v<T>;
  };

  // Returns the current format specified for this topic.
  virtual typename format_type<T>::type getFormat() {
    SIMPLEMQTT_CHECK_VALID(typename format_type<T>::type{});
    return format;
  };

  // Sets the current format used by this topic.
// The type of format depends on the underlying data type used by the topic.
  virtual MQTTFormattedTopic<T>& setFormat(typename format_type<T>::type aFormat) {
    SIMPLEMQTT_CHECK_VALID(*this);
    format = aFormat;
    return *this;
  };
};

// Co-variant return type setters for the specified type.
#define SIMPLEMQTT_OVERRIDE_SETTERS(TYPE) \
  inline TYPE& setQoS(uint8_t qos) override { \
    MQTTTopic::setQoS(qos); \
    return *this; \
  }; \
  inline TYPE& setRetained(bool retained) override { \
    MQTTTopic::setRetained(retained); \
    return *this; \
  }; \
  inline TYPE& setAutoPublish(bool autoPublish) override { \
    MQTTTopic::setAutoPublish(autoPublish); \
    return *this; \
  }; \
  inline TYPE& setRequestable(bool requestable) override { \
    MQTTTopic::setRequestable(requestable); \
    return *this; \
  }; \
  inline TYPE& setSettable(bool settable) override { \
    MQTTTopic::setSettable(settable); \
    return *this; \
  };

// Co-variant return type setFormat() function for the specified type.
#define SIMPLEMQTT_FORMAT_SETTER(TYPE, T) \
  inline TYPE& setFormat(typename format_type<T>::type aFormat) override { \
    SIMPLEMQTT_CHECK_VALID(*this); \
    MQTTFormattedTopic<T>::format = aFormat; \
    return *this; \
  };
