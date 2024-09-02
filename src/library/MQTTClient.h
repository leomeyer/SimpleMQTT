/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

class MQTTClient: public MQTTGroup {

public:
  enum class State : int8_t {
    BAD_CREDENTIALS = -5,
    INVALID_HOST = -4,
    INVALID_TOPIC = -3,
    BAD_CLIENT_ID = -2,
    ERROR = -1,
    DISCONNECTED = 0,
    CONNECTING = 1,
    RECONNECTED = 2,
    CONNECTED = 3,
    CONNECTION_LOST = 4,
    CONNECTION_TIMEOUT = 5
  };

protected:
  const char* mqttClientName;
  const char* mqttHost;
  int mqttPort = 1883;
  const char* mqttUser;
  const char* mqttPassword;
  bool cleanSession;
  State prevState = State::DISCONNECTED;
  TopicOrder globalTopicOrder = DEFAULT_TOPIC_ORDER;
  MQTTValue<String>* statusTopic = nullptr;
  MQTTWill* mqttWill = nullptr;
  t_mqttCallback mqttCallback = nullptr;
  uint64_t lastErrorTime = 0;

  MQTTClient(const char* clientName, uint8_t aConfig)
    : MQTTGroup(nullptr, __internal::_Topic(clientName), aConfig), mqttClientName(clientName) {
    MQTTTopic::INVALID_TOPIC._parent = this;
  };

  MQTTClient(const char* clientName, const char* host, int port = 1883, const char* user = nullptr, const char* password = nullptr, bool clean = true, MQTTConfig config = DEFAULT_CONFIG)
    : MQTTGroup(nullptr, __internal::_Topic(clientName), (uint8_t)config),
      mqttClientName(clientName), mqttHost(host), mqttPort(port), mqttUser(user), mqttPassword(password), cleanSession(clean) {
    MQTTTopic::INVALID_TOPIC._parent = this;
  };

  inline String type() const override { return String("$"); };

  MQTTGroup& parent() override {
    return *this;
  };

  MQTTClient* getClient() {
    return this;
  };

  virtual bool mqttConnect() {
#ifdef SIMPLEMQTT_DEBUG_SERIAL    
    String debug(PSTR("Client '"));
    debug += mqttClientName;
    debug += F("' connecting to tcp://");
    if (mqttUser != nullptr) {
      debug += mqttUser;
      debug += F("@");
    }
    debug += mqttHost;
    debug += F(":");
    debug += String(mqttPort);
    if (mqttWill != nullptr) {
      debug += F(" [will topic: '");
      debug += mqttWill->name();
      debug += F("', value: ");
      debug += mqttWill->getMessage();
      debug += F("]\n");
    }
    SIMPLEMQTT_DEBUG(debug.c_str());
#endif
    return _mqttConnect();
  };

  size_t printExtras(Print& p, size_t indent) const override {
    size_t n = 0;
    if (mqttWill != nullptr) {
      mqttWill->printTo(p, indent);
    }
    return n;
  };

  virtual State mqttSetup() = 0;
  virtual bool _mqttConnect() = 0;
  virtual bool mqttConnected() = 0;
  virtual State mqttLoop() = 0;
  virtual State mqttState() = 0;

public:

  virtual String getStateText(State state) {
    switch (state) {
      case State::BAD_CREDENTIALS: return "Bad credentials";
      case State::INVALID_HOST: return "Invalid host";
      case State::INVALID_TOPIC: return "Invalid topic";
      case State::BAD_CLIENT_ID: return "Bad client ID";
      case State::ERROR: return "Error";
      case State::DISCONNECTED: return "Disconnected";
      case State::CONNECTING: return "Connecting";
      case State::RECONNECTED: return "Reconnected";
      case State::CONNECTED: return "Connected";
      case State::CONNECTION_LOST: return "Connection lost";
      case State::CONNECTION_TIMEOUT: return "Timeout";
    };
    return String("Unknown (") + (int)state + ")";
  };

  virtual String getFinalTopic(const String& topic) {
    if (topic.startsWith("/"))
      return topic.substring(1);
    else
      return topic;
  };

  TopicOrder getTopicOrder() override {
    return globalTopicOrder;
  };

  MQTTClient& setTopicOrder(TopicOrder order) override {
    globalTopicOrder = order;
    return *this;
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

  MQTTWill* setWill(MQTTWill* will) {
    mqttWill = will;
    if (will == nullptr)
      return nullptr;
    if (!will->isTopicValid())
      return will;
    will->_parent = this;
    return will;
  };

  virtual void setCustomCallback(t_mqttCallback callback) {
    this->mqttCallback = callback;
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
    SIMPLEMQTT_DEBUG(PSTR("Publishing%s topic: '%s' (%s) with payload '%s' (QoS %d)\n"),
                                     (value->isRetained() ? " retained" : ""), fullTopic.c_str(), value->getConfigStr().c_str(), payload.c_str(), value->getQoS());
    return mqttPublish(fullTopic, payload.c_str(), value->isRetained(), value->getQoS());
  };

  bool publish(__internal::_Topic& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) {
    if (!topic.isValid())
      return false;
    String t(topic.get());
    if (t[0] != '/')
      t = String(mqttClientName) + "/" + t;
    return mqttPublish(t, payload, retained, qos, dup);
  };

  State handle(State previousState) {
    // reset error after retry time?
    if ((int)previousState < 0 && millis() - lastErrorTime > SIMPLEMQTT_RETRY_AFTER_ERROR) {
      previousState = State::DISCONNECTED;
    }

    State newState = previousState;
    // disconnected? need to connect
    if (previousState == State::DISCONNECTED && !mqttConnected()) {
      // initial connect attempt?
      if (mqttState() != State::CONNECTION_LOST) {
        // checks
        if (mqttClientName == nullptr || mqttClientName[0] == '\0') {
          SIMPLEMQTT_ERROR(PSTR("Client name not specified or invalid\n"));
          return State::BAD_CLIENT_ID;
        }
        if (mqttHost == nullptr || mqttHost[0] == '\0') {
          SIMPLEMQTT_ERROR(PSTR("Host name not specified\n"));
          return State::INVALID_HOST;
        }
        if (!isTopicValid()) {
          SIMPLEMQTT_ERROR(PSTR("Client topic invalid: '%s'\n"), topic.get());
          return State::INVALID_TOPIC;
        }

        State setup = mqttSetup();
        if (setup != State::DISCONNECTED) 
          // error
          return setup;
      }

      // try to connect
      if (mqttConnect()) {
        // a possible will needs to be republished after connecting
        if (mqttWill != nullptr)
          mqttWill->republish();

        SIMPLEMQTT_DEBUG(PSTR("Connection established\n"));
        // MQTT library processing
        newState = mqttLoop();

        if (newState != State::CONNECTED)
          return State::CONNECTING;
        else
          return State::RECONNECTED;
      } else {
        // connection error
        lastErrorTime = millis();
        newState = mqttState();
        SIMPLEMQTT_ERROR((String("Connect failed: ") + getStateText(newState) + "\n").c_str());
        return newState;
      }
    } else
    if (previousState == State::CONNECTING || previousState == State::CONNECTED || previousState == State::RECONNECTED) {
      // connecting/connected, perform main processing
      if (mqttConnected() && mqttState() != State::CONNECTION_LOST) {
        // MQTT library processing
        newState = mqttLoop();
        // error?
        if ((int)newState < 0)  {
          lastErrorTime = millis();
          SIMPLEMQTT_ERROR((String("Error in MQTT loop: ") + getStateText(newState) + "\n").c_str());
          return newState;
        }

        // initial (re-)connect?
        if (previousState == State::RECONNECTED || (previousState == State::CONNECTING && newState == State::CONNECTED)) {
          // performed once after initial (re-)connect
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
        // publich status topic if necessary
        if (statusTopic != nullptr && statusTopic->needsPublish())
          statusTopic->publish();
      }
    }
    return newState;
  };

  State handle() {
    prevState = handle(prevState);
    return prevState;
  };

  virtual bool payloadReceived(const char* topic, const char* payload, unsigned int length) {
    char* p = (char*)alloca(length + 1);
    memcpy(p, payload, length);
    p[length] = '\0';
    SIMPLEMQTT_DEBUG(PSTR("Received topic: '%s' with payload '%s'\n"), topic, p);

    return processPayload(this, topic, p);
  };

  virtual bool handleMessage(const char* topic, const char* payload, unsigned int length) {
    // functions returns true if they have handled the topic
    // in case of errors they may call setStatus to inform the broker
    if (this->payloadReceived(topic, (const char*)payload, length))
      return true;
    // use custom callback if specified
    if (this->mqttCallback && this->mqttCallback(this, topic, (const char*)payload, length))
      return true;
    // nothing handled this message
    this->setStatus((int8_t)ResultCode::UNKNOWN_TOPIC, String("Unknown topic: ") + topic);
    return false;
  };

  virtual bool mqttSubscribe(const String& topic, uint8_t qos) = 0;
  virtual bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) = 0;
};

// The client implementation.
template <typename T>
class SimpleMQTTClient: public T, public MQTTClient {
friend class MQTTTopic;

private:
  void* operator new(size_t);           // standard new
  void* operator new(size_t, void*);    // placement new
  void* operator new[](size_t);         // array new
  void* operator new[](size_t, void*);  // placement array new

protected:
  //////////////////////////////////////////
  // implemented by template specializations
  virtual State mqttSetup() {
    return State::DISCONNECTED;
  };
  virtual bool _mqttConnect() {
    return false;
  };
  virtual State mqttLoop() {
    return State::DISCONNECTED;
  };
  virtual bool mqttConnected() {
    return false;
  };
  virtual State mqttState() {
    return State::DISCONNECTED;
  };
  virtual bool mqttSubscribe(const String& topic, uint8_t qos) {
    return false;
  };
  //////////////////////////////////////////

public:
  template <typename... Args>
  SimpleMQTTClient(Client& client, const char* clientName, Args... args)
    : T(client, args...), MQTTClient(clientName, (uint8_t)DEFAULT_CONFIG) {};

  SimpleMQTTClient(Client& client, const char* clientName, const char* host, int port = 1883, const char* user = nullptr, const char* password = nullptr, bool clean = true, MQTTConfig config = DEFAULT_CONFIG)
    : T(client), MQTTClient(clientName, host, port, user, password, clean, config) {};

  //////////////////////////////////////////
  // implemented by template specializations
  bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) {
    return false;
  };
  //////////////////////////////////////////
};
