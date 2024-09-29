/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// helper class for StreamClientImpl and proxy functionality
class StreamHandler {
private:
  template <typename T>
  void print(T data) {
    stream->print(data);
  };

protected:
  Stream* stream = nullptr;
  String message;

  StreamHandler() {};

  virtual void sendChar(char c) {
    print(c);
  };

public:
  StreamHandler(Stream& aStream) : stream(&aStream) {};

  void sendEncoded(uint8_t c) {
    if (c == '\\') {
      sendChar('\\');
      sendChar('\\');
    } else
    if (c < 10) {
      sendChar('\\');
      // maps from '0' to '9'
      sendChar((char)('0' + c));
    } else
    if (c < 32) {
      sendChar('\\');
      // maps ASCII 10+ to 'a'...
      sendChar((char)('a' + (c - 10)));
    } else
      sendChar((char)c);
  };

  void send(const char* str, bool end = false) {
    uint8_t c = str[0];
    size_t pos = 0;
    while (c != '\0') {
      sendEncoded((char)c);
      c = str[++pos];
    }
    if (end) {
      sendChar('\n');
      stream->flush();
    }
  };

  void send(const __FlashStringHelper* fstr, bool end = false) {
    PGM_P p = reinterpret_cast<PGM_P>(fstr);
    uint8_t c = pgm_read_byte(p++);
    size_t pos = 0;
    while (c != '\0') {
      sendEncoded(c);
      c = pgm_read_byte(p++);
    }
    if (end) {
      sendChar('\n');
      stream->flush();
    }
  };

  void send(char c, bool end = false) {
    sendChar(c);
    if (end) {
      sendChar('\n');
      stream->flush();
    }
  };

  void send(int i, bool end = false) {
    String s(i);
    send(s.c_str());
    if (end) {
      sendChar('\n');
      stream->flush();
    }
  };

  void send(bool b, bool end = false) {
    send(b ? '1' : '0', end);
  };

  void skipMessage(uint16_t maxWaitMs = 1000) {
    // advance stream up to the next newline character
    if (stream->available()) {
      unsigned long t = stream->getTimeout();
      stream->setTimeout(1);
      stream->find('\n');
      stream->setTimeout(t);
    }
  };

  bool readByte(uint8_t* b) {
    uint64_t start = millis();
    while (!stream->available() && (millis() - start < stream->getTimeout()));
    if (!stream->available()) {
      SIMPLEMQTT_ERROR(F("Stream: timeout\n"));
      return false;
    }
    int i = stream->read();
    if (i < 0) {
      SIMPLEMQTT_ERROR(F("Stream: no data\n"));
      return false;
    }
    *b = (uint8_t)i;
    return true;
  }

  bool expectChar(uint8_t* c) {
    if (!readByte(c))
      return false;
    // escaped character?
    if (*c == '\\') {
      uint8_t n;
      // unescape from next byte
      if (!expectChar(&n)) {
        SIMPLEMQTT_ERROR(F("Stream: timeout\n"));
        return false;
      }
      if (n == '\\')
        return true;
      if (n >= '0' && n <= '9')
        *c = n - '0';
      else
      if (n >= 'a')
        *c = n - 'a' + 10;
    }
    return true;
  }
/*
  bool expect(const __FlashStringHelper* fstr) {
    PGM_P p = reinterpret_cast<PGM_P>(fstr);
    uint8_t c = pgm_read_byte(p++);
    uint8_t b;
    while (c != 0) {
      if (!expectChar(&b))
        return false;   // timeout or unexpected EOL
      // unexpected character?
      if (b != c) {
        skipMessage();
        break;
      }
      c = pgm_read_byte(p++);
    }
    if (c != 0) {
      SIMPLEMQTT_ERROR(F("Stream: unexpected response\n"));
      return false;
    }
    // next byte must be newline
    if (!readByte(&b) || b != '\n')
      return false;
    return true;
  };
*/
  bool expect(const char c) {
    uint8_t b;
    if (!expectChar(&b))
      return false;   // timeout or unexpected EOL
    // unexpected character?
    if (b != c) {
      skipMessage();
      SIMPLEMQTT_ERROR(F("Stream: unexpected response\n"));
      return false;
    }
    // next byte must be newline
    if (!readByte(&b) || b != '\n')
      return false;
    return true;
  };

  size_t unescape(char* str) {
    // unescape characters in-place
    size_t len = strlen(str);
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
      char c = str[i];
      if (c == '\\' && i < len - 1) {
        i++;
        char n = str[i];
        if (n >= '0' && n <= '9')
          c = n - '0';
        else
        if (n >= 'a')
          c = n - 'a' + 10;
      }
      str[pos++] = c;
    }
    str[pos] = 0;
    return pos;
  };

  bool getMessage(String& msg) {
    while (stream->available()) {
      int i = stream->read();
      // received EOL - message incomplete?
      if (i != '\n')
        message += (char)i;
      else
      // ignore empty messages
      if (message.length() > 0) {
        msg = message;
        message = "";
        return true;
      }
    }
    return false;
  };
};


class MQTTClient : public MQTTGroup {
public:
  enum class State : int8_t {
    CONNECTION_TIMEOUT = -6,
    BAD_CREDENTIALS = -5,
    INVALID_HOST = -4,
    INVALID_TOPIC = -3,
    BAD_CLIENT_ID = -2,
    ERROR = -1,
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2
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

  inline String type() const override {
    return String('$');
  };

  MQTTGroup& parent() override {
    return *this;
  };

  MQTTClient* getClient() {
    return this;
  };

  State mqttConnect() {
#ifdef SIMPLEMQTT_DEBUG_SERIAL
    String debug(F("Client '"));
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

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  size_t printExtras(Print& p, size_t indent) const override {
    size_t n = 0;
    if (mqttWill != nullptr) {
      mqttWill->printTo(p, indent);
    }
    return n;
  };
#endif

  virtual State mqttSetup() = 0;
  virtual State _mqttConnect() = 0;
  virtual State mqttLoop() = 0;
  virtual State mqttState() = 0;

public:

  #ifndef SIMPLEMQTT_OPTIMIZE_MEMORY
  String getStateText(State state) {
      // low RAM
      return String((int)state);
  };
  #else
  virtual String getStateText(State state) {
      switch (state) {
        case State::CONNECTION_TIMEOUT: return "Timeout";
        case State::BAD_CREDENTIALS: return "Bad credentials";
        case State::INVALID_HOST: return "Invalid host";
        case State::INVALID_TOPIC: return "Invalid topic";
        case State::BAD_CLIENT_ID: return "Bad client ID";
        case State::ERROR: return "Error";
        case State::DISCONNECTED: return "Disconnected";
        case State::CONNECTING: return "Connecting";
        case State::CONNECTED: return "Connected";
      };
      return String("Unknown (") + (int)state + ")";
  };
  #endif

  String getFinalTopic(const String& topic) {
    if (topic.charAt(0) == '/')
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
      // the status topic is not settable and auto-publishing
      uint8_t config = getConfig();
      config &= SETTABLE_CLEARMASK;
      config |= AUTO_PUBLISH_SETMASK;
      statusTopic = new MQTTValue<String>(this, aTopic, config);
    }
    return statusTopic;
  };

#ifdef __AVR__
  // low memory
  String getCodeText(int8_t code) {
    return String((int)code);
  };
#else
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
    return String((int)code);
  };
#endif

  bool setStatus(int8_t code, String topic, String message = {}) {
    String msg = getCodeText(code);
    if (message.length() > 0) {
      msg += F(": ");
      msg += message;
    }
    if (code < 0) {
      SIMPLEMQTT_ERROR(F("Status Error %d: %s (%s)\n"), (int)code, msg.c_str(), topic.c_str());
    } else {
      SIMPLEMQTT_DEBUG(F("Status Code %d: %s (%s)\n"), (int)code, msg.c_str(), topic.c_str());
    }
    if (statusTopic == nullptr)
      return false;
    // don't overwrite yet unpublished status
    if (statusTopic->needsPublish())
      return false;
      /*
    msg.replace(F("\""), F("\\\""));
    msg.replace(F("\r"), F("\\r"));
    msg.replace(F("\n"), F("\\n"));
    */
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

  void setCustomCallback(t_mqttCallback callback) {
    this->mqttCallback = callback;
  };

  bool publish(MQTTTopic* value) {
    if (value == __internal::INVALID_PTR)
      return false;
    String fullTopic = value->getFullTopic();
    // top-level-topics are only published if their publish-flag is set
    // to avoid infinite publish/set loop
    if (fullTopic.charAt(0) == '/' && !value->needsPublish())
      return false;
#ifndef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
    // apply default topic pattern for non-top-level topics
    if (!fullTopic.charAt(0) == '/') {
      String pattern = value->getTopicPattern();
      if (pattern == "")
        pattern = DEFAULT_TOPIC_PATTERN;
      int patpos = pattern.indexOf(F("%s"));
      if (patpos >= 0)
        fullTopic = pattern.substring(0, patpos) + fullTopic + pattern.substring(patpos + 2);
      else
        fullTopic = pattern;
    }
#endif
    String payload = value->getPayload();
    SIMPLEMQTT_DEBUG(F("Publishing%s topic: '%s' (%s) with payload '%s' (QoS %d)\n"),
                     (value->isRetained() ? " retained" : ""), fullTopic.c_str(), value->getConfigStr().c_str(), payload.c_str(), value->getQoS());
    return mqttPublish(fullTopic, payload.c_str(), value->isRetained(), value->getQoS());
  };

  bool publish(__internal::_Topic& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) {
    if (!topic.isValid())
      return false;
    String t(topic.get());
    if (t[0] != '/')
      t = String(mqttClientName) + '/' + t;
    return mqttPublish(t, payload, retained, qos, dup);
  };

  State handle(State previousState) {
    // reset error after retry time?
    if ((int)previousState < 0 && millis() - lastErrorTime > SIMPLEMQTT_RETRY_AFTER_ERROR) {
      previousState = State::DISCONNECTED;
    }

    State newState = previousState;
    // disconnected? need to connect
    if (previousState == State::DISCONNECTED) {
#ifndef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
      // initialize default patterns
      if (DEFAULT_TOPIC_PATTERN.length() == 0)
        DEFAULT_TOPIC_PATTERN = F("%s");
      if (DEFAULT_REQUEST_PATTERN.length() == 0)
        DEFAULT_REQUEST_PATTERN = F("%s/get");
      if (DEFAULT_SET_PATTERN.length() == 0)
        DEFAULT_SET_PATTERN = F("%s/set");
#endif        
      // checks
      if (mqttClientName == nullptr || mqttClientName[0] == '\0') {
        SIMPLEMQTT_ERROR(F("Client name not specified or invalid\n"));
        return State::BAD_CLIENT_ID;
      }
      if (mqttHost == nullptr || mqttHost[0] == '\0') {
        SIMPLEMQTT_ERROR(F("Host name not specified\n"));
        return State::INVALID_HOST;
      }
      if (!isTopicValid()) {
        SIMPLEMQTT_ERROR(F("Client topic invalid: '%s'\n"), topic.get());
        return State::INVALID_TOPIC;
      }

      // initialize client
      newState = mqttSetup();

      // still disconnected/no error?
      if (newState == State::DISCONNECTED) {
        // try to connect
        newState = mqttConnect();
      }
    } 
    
    if (newState == State::CONNECTING) {
      // wait for connection to be established
      newState = mqttLoop();

      // now connected?
      if (newState == State::CONNECTED) {
        SIMPLEMQTT_DEBUG(F("Connection established\n"));
        // perform once after initial (re-)connect
        addSubscriptions(this);
        // a possible will needs to be republished after connecting
        if (mqttWill != nullptr)
          mqttWill->republish();
      }
    }
    
    if (newState == State::CONNECTED) {
      // connected, perform main processing
      newState = mqttLoop();

      // still connected?	
      if (newState == State::CONNECTED) {
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

    // new error?
    if ((newState != previousState) && ((int)newState < 0)) {
      // connection error
      lastErrorTime = millis();
      SIMPLEMQTT_ERROR((String(F("MQTTClient error: ")) + getStateText(newState) + '\n').c_str());
    }

    return newState;
  };

  State handle() {
    prevState = handle(prevState);
    return prevState;
  };

  bool payloadReceived(const char* topic, const char* payload, unsigned int length) {
    char* p = (char*)alloca(length + 1);
    memcpy(p, payload, length);
    p[length] = '\0';
    SIMPLEMQTT_DEBUG(F("Received topic: '%s' with payload '%s'\n"), topic, p);

    return processPayload(this, topic, p);
  };

  bool handleMessage(const char* topic, const char* payload, unsigned int length) {
    // functions returns true if they have handled the topic
    // in case of errors they may call setStatus to inform the broker
    if (this->payloadReceived(topic, (const char*)payload, length))
      return true;
    // use custom callback if specified
    if (this->mqttCallback && this->mqttCallback(this, topic, (const char*)payload, length))
      return true;
    // nothing handled this message
    this->setStatus((int8_t)ResultCode::UNKNOWN_TOPIC, String(F("Unknown topic: ")) + topic);
    return false;
  };

  virtual bool mqttSubscribe(const String& topic, uint8_t qos) = 0;
  virtual bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) = 0;
};

// The client implementation.
template<typename T, typename C>
class MQTTClientImpl : public T, public MQTTClient {
  friend class MQTTTopic;

private:
  void* operator new(size_t);           // standard new
  void* operator new(size_t, void*);    // placement new
  void* operator new[](size_t);         // array new
  void* operator new[](size_t, void*);  // placement array new

protected:
  State mqttSetup() override {
    return State::DISCONNECTED;
  };
  State _mqttConnect() override {
    return State::DISCONNECTED;
  };
  State mqttLoop() override {
    return State::DISCONNECTED;
  };
  State mqttState() override {
    return State::DISCONNECTED;
  };
  bool mqttSubscribe(const String& topic, uint8_t qos) {
    return false;
  };

public:
  template<typename... Args>
  MQTTClientImpl(C& client, const char* clientName, Args... args)
    : T(client, args...), MQTTClient(clientName, (uint8_t)DEFAULT_CONFIG) {};

  MQTTClientImpl(C& client, const char* clientName, const char* host, int port = 1883, const char* user = nullptr, const char* password = nullptr, bool clean = true, MQTTConfig config = DEFAULT_CONFIG)
    : T(client), MQTTClient(clientName, host, port, user, password, clean, config) {};

  bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) {
    return false;
  };
};
