/////////////////////////////////////////////////////////////////////
// StreamClientImpl
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// MQTTClientImpl specialization using Stream

#define SIMPLEMQTT_SIMULATE_PROXY_STREAM  Serial

#ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
  #define MEMORY_STREAM_BUFFER_SIZE 64
  class MemoryStream : public Stream
  {
  private:
    char buffer[MEMORY_STREAM_BUFFER_SIZE + 1];
    Stream* output = nullptr;
    int readPos;
    int length;

  public:
    MemoryStream() : readPos(0), length(0) { 
      setTimeout(1000);
    };

    void setOutput(Stream& out) { output = &out; };

    int available() { return length; };
    
    int read() {
      int val = peek();
      if (val >= 0) {
        readPos = (readPos+1) % MEMORY_STREAM_BUFFER_SIZE;
        length--;
      }
      return val;
    };
    
    int peek() {
      return (available() == 0 ? (-1) : (int)buffer[readPos]);
    };

    void flush() {
      output->flush();
    };
    
    size_t write(uint8_t c) {
      output->write(c);
    };

    void set(const __FlashStringHelper* str) {
      readPos = 0;
      strncpy_P(buffer, (const char*)str, MEMORY_STREAM_BUFFER_SIZE);
      buffer[MEMORY_STREAM_BUFFER_SIZE] = 0;
      length = strlen(buffer);
      buffer[length] = '\n';
      buffer[++length] = '\0';
    };

    void set(const String& str) {
      readPos = 0;
      strncpy(buffer, str.c_str(), MEMORY_STREAM_BUFFER_SIZE);
      buffer[MEMORY_STREAM_BUFFER_SIZE] = 0;
      length = strlen(buffer);
      buffer[length] = '\n';
      buffer[++length] = '\0';
    };
  };
#endif  // SIMPLEMQTT_SIMULATE_PROXY_STREAM


class MQTTStreamClientImpl : public MQTTClientImpl<StreamHandler, Stream> {
private:
  void* operator new(size_t);           // standard new
  void* operator new(size_t, void*);    // placement new
  void* operator new[](size_t);         // array new
  void* operator new[](size_t, void*);  // placement array new

protected:
#ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
  MemoryStream myStream;
#endif
  State state = State::DISCONNECTED;
  bool connected = false;
  uint64_t connectStart = 0;

  State mqttSetup() override {
    skipMessage(10);
    state = State::DISCONNECTED;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("-> "));
    #endif
    send('I', true);
    // proxy must confirm
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    myStream.set(F("K"));
    #endif
    if (!expect('K'))
      return State::ERROR;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("<- K\n"));
    #endif
    return State::DISCONNECTED;
  };

  State _mqttConnect() override {
    state = State::DISCONNECTED;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("-> "));
    #endif
    send('C');
    send('\t');
    send(mqttClientName);
    send('\t');
    send(mqttHost);
    send('\t');
    send(mqttPort);
    send('\t');
    send(mqttUser);
    send('\t');
    send(mqttPassword);
    send('\t');
    send(mqttWill ? getFinalTopic(mqttWill->getFullTopic()).c_str() : "");
    send('\t');
    send(mqttWill ? mqttWill->getQoS() : 0);
    send('\t');
    send(mqttWill ? mqttWill->isRetained() : false);
    send('\t');
    send(mqttWill ? mqttWill->getMessage() : "");
    send('\t');
    send(cleanSession, true);
    // proxy must confirm
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    myStream.set(F("K"));
    #endif
    if (!expect('K'))
      state = State::ERROR;
    else {
      #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
        SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("<- K\n"));
      #endif
      connectStart = millis();
      state = State::CONNECTING;
    }
    return state;
  };

  State mqttLoop() override {
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM    
      if (SIMPLEMQTT_SIMULATE_PROXY_STREAM.available()) {
        String input;
        while (SIMPLEMQTT_SIMULATE_PROXY_STREAM.available()) {
          char c = SIMPLEMQTT_SIMULATE_PROXY_STREAM.read();
          input += c;
          delay(10);
        }
        myStream.set(input);
      }
    #endif

    // process incoming message
    String message;
    if (getMessage(message)) {
      #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
        SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("<- "));
        SIMPLEMQTT_SIMULATE_PROXY_STREAM.println(message.c_str());
      #endif
      // single-part message?
      if (message.length() == 1) {
        if (message.charAt(0) == 'D') {
          state = State::DISCONNECTED;
        } else
        if (state == State::CONNECTING) {
          // connection confirmation?
          if (message.charAt(0) == 'O') {
            state = State::CONNECTED;
          }
        }
      } else
      // length > 1; multi-part message?
      if (message.charAt(1) == '\t') {
        // error?
        if (message.charAt(1) == 'E') {
          state = State::ERROR;
          // parse error code
          size_t l = message.length() - 2;  // E\t
          int i = (l <= 0 ? -1 : atoi(&message.c_str()[2]));  // invalid ==> 0
          // valid error code?
          if (i < 0)
            state = (State)i;
        }
        else
        if (state == State::CONNECTED) {
          // message from broker?
          if (message.charAt(0) == 'M') {
            String msg = message.substring(2);  // M\t
            if (msg.length() > 0) {
              int tabpos = msg.indexOf('\t');
              if (tabpos > 0) {
                String topic = msg.substring(0, tabpos);
                String payload = msg.substring(tabpos + 1);
                char t[topic.length() + 1];
                strncpy(t, topic.c_str(), topic.length() + 1);
                // topic is required
                if (unescape(t) > 0) {
                  char p[payload.length() + 1];
                  strncpy(p, payload.c_str(), payload.length() + 1);
                  size_t pl = unescape(p);
                  handleMessage(t, p, pl);
                }
              }
            }
          } // "M"
        } // connected
      } // multi-part message
    } else
    // no message to process
    if (state == State::CONNECTING) {
      // connect timeout?
      if (connectStart > 0 && (millis() - connectStart > 10000)) {
        state = State::CONNECTION_TIMEOUT;
      }
    }

    return state;
  };

  State mqttState() override {
    return state;
  };

  bool mqttSubscribe(const String& topic, uint8_t qos) override {
    if (state != State::CONNECTED)
      return false;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("-> "));
    #endif
    send('S');
    send('\t');
    send(topic.c_str());
    send('\t');
    send(qos, true);
    // proxy must confirm
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    myStream.set(F("K"));
    #endif
    if (!expect('K'))
      return false;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("<- K\n"));
    #endif
    return true;
  };

#ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
  void sendChar(char c) override {
    StreamHandler::sendChar(c);
    SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(c);
  };
#endif
public:
  MQTTStreamClientImpl(Stream& aStream, const char* clientName)
#ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    : MQTTClientImpl<StreamHandler, Stream>(myStream, clientName) { myStream.setOutput(aStream); };
#else
    : MQTTClientImpl<StreamHandler, Stream>(aStream, clientName) { };
#endif

  MQTTStreamClientImpl(Stream& aStream, const char* clientName, const char* host, int port = 1883, const char* user = nullptr, const char* password = nullptr, bool clean = true, MQTTConfig config = DEFAULT_CONFIG)
#ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    : MQTTClientImpl<StreamHandler, Stream>(myStream, clientName, host, port, user, password, clean, config) { myStream.setOutput(aStream); };
#else
    : MQTTClientImpl<StreamHandler, Stream>(aStream, clientName, host, port, user, password, clean, config) { };
#endif

  bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) override {
    if (state != State::CONNECTED)
      return false;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("-> "));
    #endif
    send('P');
    send('\t');
    send(topic.c_str());
    send('\t');
    send(payload);
    send('\t');
    send(retained);
    send('\t');
    send(qos);
    send('\t');
    send(dup, true);
    // proxy must confirm
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
    myStream.set(F("K"));
    #endif
    if (!expect('K'))
      return false;
    #ifdef SIMPLEMQTT_SIMULATE_PROXY_STREAM
      SIMPLEMQTT_SIMULATE_PROXY_STREAM.print(F("<- K\n"));
    #endif
    return true;
  };
};
