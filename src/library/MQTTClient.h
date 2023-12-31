/////////////////////////////////////////////////////////////////////
// SimpleMQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// The SimpleMQTT client class that is used to define topics and handle MQTT communication.
class SimpleMQTTClient : public PubSubClient, public MQTTGroup {
friend class MQTTTopic;

private:
  void* operator new(size_t);           // standard new
  void* operator new(size_t, void*);    // placement new
  void* operator new[](size_t);         // array new
  void* operator new[](size_t, void*);  // placement array new

public:
  enum class State : int8_t {
    INVALID_NAME = -3,
    INVALID_TOPIC = -2,
    INVALID_HOST = -1,
    DISCONNECTED = 0,
    CONNECTING = 1,
    RECONNECTED = 2,
    CONNECTED = 3
  };

protected:
  const char* mqttClientName;
  const char* mqttHost;
  int mqttPort = 1883;
  bool cleanSession = true;
  const char* mqttUser;
  const char* mqttPassword;
  MQTTValue<String>* statusTopic = nullptr;
  MQTTWill* mqttWill = nullptr;
  MQTT_CALLBACK_SIGNATURE = nullptr;
  State previousState = State::DISCONNECTED;
  TopicOrder globalTopicOrder = DEFAULT_TOPIC_ORDER;

  inline String type() const override { return String("$"); };

  MQTTGroup& parent() override {
    return *this;
  };

  SimpleMQTTClient* getClient() {
    return this;
  };

  virtual bool connect() {
    if (mqttWill != nullptr)
      return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword, getFinalTopic(mqttWill->getFullTopic()).c_str(), mqttWill->getQoS(), mqttWill->isRetained(), mqttWill->getMessage(), cleanSession);
    else
      return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword);
  };

public:
  SimpleMQTTClient(Client& client, const char* clientName, const char* host, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, __internal::_Topic(clientName), (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, __internal::_Topic topic, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, __internal::_Topic(clientName), (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port, __internal::_Topic topic, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, __internal::_Topic topic, MQTTConfig config, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, MQTTConfig config, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, __internal::_Topic(clientName), (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port, MQTTConfig config, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, __internal::_Topic(clientName), (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, __internal::_Topic topic, bool clean, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), cleanSession(clean), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port, __internal::_Topic topic, bool clean, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)DEFAULT_CONFIG),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), cleanSession(clean), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, __internal::_Topic topic, MQTTConfig config, bool clean, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), cleanSession(clean), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port, __internal::_Topic topic, MQTTConfig config, bool clean, const char* user = nullptr, const char* password = nullptr)
    : PubSubClient(client), MQTTGroup(nullptr, topic, (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), cleanSession(clean), mqttUser(user), mqttPassword(password) { MQTTTopic::INVALID_TOPIC._parent = this; };

  TopicOrder getTopicOrder() override {
    return globalTopicOrder;
  };

  SimpleMQTTClient& setTopicOrder(TopicOrder order) override {
    globalTopicOrder = order;
    return *this;
  };

  MQTTWill* setWill(MQTTWill* will) {
    mqttWill = will;
    if (will == nullptr)
      return nullptr;
    if (!will->isTopicValid())
      return will;
    will->_parent = this;
    return will;
  };

  virtual String getFinalTopic(const String& topic) {
    if (topic.startsWith("/"))
      return topic.substring(1);
    else
      return topic;
  };

  virtual void setCustomCallback(MQTT_CALLBACK_SIGNATURE) {
    this->callback = callback;
  };

  MQTTValue<String>* setStatusTopic(__internal::_Topic aTopic) {
    if (statusTopic == nullptr && aTopic.isValid()) {
      statusTopic = new MQTTValue<String>(this, aTopic, getConfig());
      statusTopic->setSettable(false).setAutoPublish(true);
    }
    return statusTopic;
  };

  virtual String getCodeText(int8_t code) {
    switch ((ResultCode)code) {
      case ResultCode::OUT_OF_MEMORY: return String() + F("Out of memory");
      case ResultCode::INVALID_VALUE: return String() + F("Invalid value");
      case ResultCode::CANNOT_SET: return String() + F("Cannot set");
      case ResultCode::UNKNOWN_TOPIC: return String() + F("Unnown topic");
      case ResultCode::INVALID_REQUEST: return String() + F("Invalid request");
      case ResultCode::INVALID_PAYLOAD: return String() + F("Invalid payload");
      default:
        if (code < 0) return String() + F("Error ") + (int)code;
        else if (code == 0) return String() + F("OK");
    }
    return String();
  };

  virtual bool setStatus(int8_t code, String topic, String message = {}) {
    String msg = getCodeText(code) + (message == "" ? "" : F(": ") + message);
    if (code < 0) {
      SIMPLEMQTT_ERROR(PSTR("Status Error %d: %s (%s)\n"), (int)code, msg.c_str(), topic.c_str());
    } else {
      SIMPLEMQTT_DEBUG(PSTR("Status Code %d: %s (%s)\n"), (int)code, msg.c_str(), topic.c_str());
    }
    if (statusTopic == nullptr)
      return false;
    // don't overwrite yet unpublished status
    if (statusTopic->needsPublish())
      return false;
    msg.replace("\"", "\\\"");
    msg.replace("\r", "\\r");
    msg.replace("\n", "\\n");
    String json(F("{\"code\":"));
    json.concat(code);
    if (code < 0)
      json.concat(F(",\"error\":\""));
    else
      json.concat(F(",\"message\":\""));
    json.concat(msg);
    if (topic != "") {
      json.concat(F("\",\"topic\":\""));
      json.concat(topic);
    }
    json.concat(F("\"}"));
    statusTopic->set(json);
    return true;
  };

  bool publish(const char* topic, const char* payload, boolean retained) {
    return PubSubClient::publish(getFinalTopic(topic).c_str(), payload, retained);
  };

  bool publish(const char* topic, const char* payload) {
    return PubSubClient::publish(getFinalTopic(topic).c_str(), payload);
  };

  bool publish(MQTTTopic* value) {
    if (value == __internal::INVALID_PTR)
      return false;
    String fullTopic = value->getFullTopic();
    // top-level-topics are only published if their publish-flag is set
    // to avoid infinite publish/set loop
    if (fullTopic.startsWith("/") && !value->needsPublish())
      return false;
    // apply default topic pattern for non-top-level topics
    if (!fullTopic.startsWith("/")) {
      String pattern = value->getTopicPattern();
      if (pattern == "")
        pattern = DEFAULT_TOPIC_PATTERN;
      pattern.replace("%s", fullTopic);
      fullTopic = pattern;
    }
    String payload = value->getPayload();
    SIMPLEMQTT_DEBUG(PSTR("Publishing%s topic: '%s' (%s) with payload '%s'\n"), (value->isRetained() ? " retained" : ""), fullTopic.c_str(), value->getConfigStr().c_str(), payload.c_str());
    return publish(fullTopic.c_str(), payload.c_str(), value->isRetained());
  };

  bool publish(__internal::_Topic& topic, const char* payload, bool retained = false) {
    if (!topic.isValid())
      return false;
    String t(topic.get());
    if (t[0] != '/')
      t = String(mqttClientName) + "/" + t;
    return publish(t.c_str(), payload, retained);
  };

  State handle(State previousState) {
    // initial connect attempt?
    if (previousState == State::DISCONNECTED && !connected() && state() != MQTT_CONNECTION_LOST) {
      // checks
      if (mqttClientName[0] == '\0') {
        SIMPLEMQTT_ERROR(PSTR("SimpleMQTTClient name invalid\n"));
        return State::INVALID_NAME;
      }
      if (mqttHost[0] == '\0') {
        SIMPLEMQTT_ERROR(PSTR("SimpleMQTTClient host name invalid\n"));
        return State::INVALID_HOST;
      }
      if (!isTopicValid()) {
        SIMPLEMQTT_ERROR(PSTR("SimpleMQTTClient topic invalid: '%s'\n"), topic.get());
        return State::INVALID_TOPIC;
      }

      // setup
      setServer(mqttHost, mqttPort);
      setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        if (!this->payloadReceived(topic, payload, length)) {
          if (this->callback)
            this->callback(topic, payload, length);
          else
            this->setStatus((int8_t)ResultCode::UNKNOWN_TOPIC, String("Unknown topic: ") + topic);
        }
      });

#if SIMPLEMQTT_JSON_BUFFERSIZE > 0
      // increase buffer size to allow for larger Json messages
      setBufferSize(SIMPLEMQTT_JSON_BUFFERSIZE);
#endif

      // try to connect
      SIMPLEMQTT_DEBUG(PSTR("SimpleMQTTClient connecting...\n"));
      if (connect()) {
        SIMPLEMQTT_DEBUG(PSTR("SimpleMQTTClient connected\n"));
        loop();
        if (state() != MQTT_CONNECTED)
          return State::CONNECTING;
        else
          return State::RECONNECTED;
      } else {
        SIMPLEMQTT_DEBUG(PSTR("SimpleMQTTClient disconnected\n"));
        return State::DISCONNECTED;
      }
    } else
      // connected, perform main processing
      if (connected() && state() != MQTT_CONNECTION_LOST) {
        // PubSubClient processing
        loop();

        if (previousState == State::RECONNECTED || (previousState == State::CONNECTING && state() == MQTT_CONNECTED)) {
          // performed once after initial connect
          addSubscriptions(this);
          // publish all values
          MQTTGroup::publish(true);
          if (previousState == State::RECONNECTED)
            return State::CONNECTED;
          else
            return State::RECONNECTED;
        }
        if (mqttWill != nullptr && mqttWill->needsPublish())
          mqttWill->publish();

        // recursively check registered topics
        check();
        // publish changed topics
        MQTTGroup::publish();
        if (statusTopic != nullptr && statusTopic->needsPublish())
          statusTopic->publish();
      }

    return state() == MQTT_CONNECTED ? State::CONNECTED : State::DISCONNECTED;
  };

  State handle() {
    previousState = handle(previousState);
    return previousState;
  };

  bool payloadReceived(char* topic, uint8_t* payload, unsigned int length) {
    char* p = (char*)alloca(length + 1);
    memcpy(p, payload, length);
    p[length] = '\0';
    SIMPLEMQTT_DEBUG(PSTR("Received topic: '%s' with payload '%s'\n"), topic, p);

    return processPayload(this, topic, p);
  };

protected:
  using PubSubClient::PubSubClient;
  using PubSubClient::setServer;
  using PubSubClient::setCallback;
  using PubSubClient::setClient;
};
