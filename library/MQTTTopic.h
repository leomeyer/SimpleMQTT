
class SimpleMQTTClient;
template <typename T> class MQTTArray;

class MQTTTopic {
friend class SimpleMQTTClient;
friend class MQTTGroup;

public:
#define SIMPLEMQTT_DEFINE_BIT(name, bit) \
  static const uint8_t name##_BIT = bit; \
  static const uint8_t name##_CLEARMASK = ((uint8_t) ~(1 << bit)); \
  static const uint8_t name##_SETMASK = (1 << bit);

  // the semicolon is intentional to avoid ugly Arduino IDE re-formatting
  SIMPLEMQTT_DEFINE_BIT(REQUESTABLE, 2);
  SIMPLEMQTT_DEFINE_BIT(SETTABLE, 3);
  SIMPLEMQTT_DEFINE_BIT(AUTO_PUBLISH, 4);
  SIMPLEMQTT_DEFINE_BIT(CHANGED, 5);
  SIMPLEMQTT_DEFINE_BIT(PUBLISH, 6);
  SIMPLEMQTT_DEFINE_BIT(RETAINED, 7);

protected:
  MQTTTopic* parent;
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

  inline uint8_t getConfig() {
    SIMPLEMQTT_CHECK_VALID(0);
    return config;
  };

  String getConfigStr() {
    String result;
    result += (isRetained() ? "R" : "x");
    result += (needsPublish() ? "P" : "x");
    result += (hasBeenChanged(false) ? "C" : "x");
    result += (isAutoPublish() ? "A" : "x");
    result += (isSettable() ? "S" : "x");
    result += (isRequestable() ? "Q" : "x");
    result += getQoS();
    return result;
  }

  bool isTopicValid() {
    SIMPLEMQTT_CHECK_VALID(false);
    return topic.isValid();
  };

  virtual bool checkTopic() {
    SIMPLEMQTT_CHECK_VALID(false);
    return isTopicValid();
  };

  virtual ResultCode requestReceived(const char*) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    republish();
    return ResultCode::OK;
  };

  virtual ResultCode setReceived(const char*) {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    republish();
    return ResultCode::CANNOT_SET;
  };

  MQTTTopic* setChanged(bool changed) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= CHANGED_CLEARMASK;
    config |= (changed ? CHANGED_SETMASK : 0);
    return this;
  };

  void releaseTopic() {
    topic.release();
  }

  virtual void publish(bool all = false);

  virtual void addSubscriptions(SimpleMQTTClient* client);

  virtual bool processPayload(SimpleMQTTClient* client, const char* topic, const char* payload);

public:
  MQTTTopic(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : parent(aParent), topic(aTopic), config(aConfig) {};

  typedef struct ListNode {
    MQTTTopic* data;
    ListNode* next;
  } ListNode;

  virtual SimpleMQTTClient* getClient() {
    SIMPLEMQTT_CHECK_VALID(nullptr);
    return parent->getClient();
  };

  inline const char* getTopic() {
    SIMPLEMQTT_CHECK_VALID(__internal::EMPTY);
    return topic.get();
  }

  virtual MQTTTopic* setQoS(uint8_t qos) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= ~0b11;
    config |= qos & ~0b11;
    return this;
  };

  virtual uint8_t getQoS() {
    SIMPLEMQTT_CHECK_VALID(0);
    return config & 0b11;
  };

  virtual MQTTTopic* setRetained(bool retained) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= RETAINED_CLEARMASK;
    config |= (retained ? RETAINED_SETMASK : 0);
    return this;
  };

  virtual bool isRetained() {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> RETAINED_BIT) & 1) == 1;
  };

  virtual MQTTTopic* setAutoPublish(bool autoPublish) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= AUTO_PUBLISH_CLEARMASK;
    config |= (autoPublish ? AUTO_PUBLISH_SETMASK : 0);
    return this;
  };

  virtual bool isAutoPublish() {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> AUTO_PUBLISH_BIT) & 1) == 1;
  };

  virtual MQTTTopic* setRequestable(bool requestable) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= REQUESTABLE_CLEARMASK;
    config |= (requestable ? REQUESTABLE_SETMASK : 0);
    return this;
  };

  virtual bool isRequestable() {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> REQUESTABLE_BIT) & 1) == 1;
  };

  virtual String getRequestTopic();

  virtual MQTTTopic* setSettable(bool settable) {
    SIMPLEMQTT_CHECK_VALID(this);
    config &= SETTABLE_CLEARMASK;
    config |= (settable ? SETTABLE_SETMASK : 0);
    return this;
  };

  virtual bool isSettable() {
    SIMPLEMQTT_CHECK_VALID(false);
    return ((config >> SETTABLE_BIT) & 1) == 1;
  };

  virtual String getSetTopic();

  virtual String getPayload() {
    return String();
  };

  virtual bool needsPublish() {
    SIMPLEMQTT_CHECK_VALID(false);
    bool result = ((config >> PUBLISH_BIT) & 1) == 1;
    return result;
  };

  virtual void republish() {
    SIMPLEMQTT_CHECK_VALID();
    // set flag to re-publish
    config |= PUBLISH_SETMASK;
  };

  virtual String getFullTopic() {
    SIMPLEMQTT_CHECK_VALID(String());
    if (topic.get()[0] == '/' || parent == nullptr)
      return String(topic.get());
    else if (parent != nullptr)
      return parent->getFullTopic() + "/" + topic.get();
    else
      return String(topic.get());
  };

  virtual bool hasBeenChanged(bool clear = true) {
    SIMPLEMQTT_CHECK_VALID(false);
    bool result = ((config >> CHANGED_BIT) & 1) == 1;
    if (clear)
      config &= CHANGED_CLEARMASK;
    return result;
  };
};


template<typename T>
class MQTTFormattedTopic : public MQTTTopic {
protected:
  typename format_type<T>::type format = getDefaultFormat<typename format_type<T>::type>();

  MQTTFormattedTopic(MQTTTopic* aParent, __internal::_Topic aTopic, uint8_t aConfig)
    : MQTTTopic(aParent, aTopic, aConfig) {};

public:
  virtual bool isSettable() override {
    SIMPLEMQTT_CHECK_VALID(false);
    if (!MQTTTopic::isSettable())
      return false;
    return !std::is_const_v<T>;
  };

  virtual typename format_type<T>::type getFormat() {
    SIMPLEMQTT_CHECK_VALID(typename format_type<T>::type{});
    return format;
  };

  virtual MQTTFormattedTopic<T>* setFormat(typename format_type<T>::type aFormat) {
    SIMPLEMQTT_CHECK_VALID(this);
    format = aFormat;
    return this;
  };
};


#define SIMPLEMQTT_OVERRIDE_SETTERS(TYPE) \
  inline TYPE* setQoS(uint8_t qos) { \
    MQTTTopic::setQoS(qos); \
    return this; \
  }; \
  inline TYPE* setRetained(bool retained) { \
    MQTTTopic::setRetained(retained); \
    return this; \
  }; \
  inline TYPE* setAutoPublish(bool autoPublish) { \
    MQTTTopic::setAutoPublish(autoPublish); \
    return this; \
  }; \
  inline TYPE* setRequestable(bool requestable) { \
    MQTTTopic::setRequestable(requestable); \
    return this; \
  }; \
  inline TYPE* setSettable(bool settable) { \
    MQTTTopic::setSettable(settable); \
    return this; \
  };

#define SIMPLEMQTT_FORMAT_SETTER(TYPE, T) \
  inline TYPE* setFormat(typename format_type<T>::type aFormat) { \
    SIMPLEMQTT_CHECK_VALID(this); \
    MQTTFormattedTopic<T>::format = aFormat; \
    return this; \
  };

