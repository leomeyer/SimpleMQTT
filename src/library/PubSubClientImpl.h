/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// MQTTClientImpl specialization using PubSubClient

#if not __has_include(<functional>) || defined(__STM32F1__)
namespace __ArduinoMQTT {
  // Inner hidden namespace for message callback function (if <functional> is not available).
  // Unfortunately this is necessary because this library does not accept a lambda as callback
  // which means that only one instance of this client can be used at a time. 

  MQTTClientImpl<PubSubClient, Client>* client = nullptr;

  void pubSubClientCallback(char* topic, uint8_t* payload, unsigned int length) {
    auto thisClient = static_cast<MQTTClientImpl<PubSubClient, Client>*>(client);
    thisClient->handleMessage(topic, (const char*)payload, length);
  }

}   // namespace __ArduinoMQTT
#endif

class PubSubClientImpl : public MQTTClientImpl<PubSubClient, Client> {
private:
  void* operator new(size_t);           // standard new
  void* operator new(size_t, void*);    // placement new
  void* operator new[](size_t);         // array new
  void* operator new[](size_t, void*);  // placement array new

public:
  template<typename... Args>
  PubSubClientImpl(Client& client, const char* clientName, Args... args)
    : MQTTClientImpl<PubSubClient, Client>(client, args...) {};

  PubSubClientImpl(Client& client, const char* clientName, const char* host, int port = 1883, const char* user = nullptr, const char* password = nullptr, bool clean = true, MQTTConfig config = DEFAULT_CONFIG)
    : MQTTClientImpl<PubSubClient, Client>(client, clientName, host, port, user, password, clean, config) {};

  bool mqttPublish(const String& topic, const char* payload, bool retained = false, uint8_t qos = 0, bool dup = false) override {
    return PubSubClient::publish(MQTTClient::getFinalTopic(topic).c_str(), payload, retained);
  };

protected:
  MQTTClient::State mqttSetup() override {
    // PubSubClient setup
    PubSubClient::setServer(MQTTClient::mqttHost, MQTTClient::mqttPort);

#if __has_include(<functional>) && not defined(__STM32F1__)
    // use lambda
    PubSubClient::setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
      this->handleMessage(topic, (const char*)payload, length);
    });
#else
    __ArduinoMQTT::client = this;
    PubSubClient::setCallback(__ArduinoMQTT::pubSubClientCallback);
#endif

  #if SIMPLEMQTT_JSON_BUFFERSIZE > 0
    // increase buffer size to allow for larger Json messages
    PubSubClient::setBufferSize(SIMPLEMQTT_JSON_BUFFERSIZE);
  #else
    PubSubClient::setBufferSize(SIMPLEMQTT_BUFFERSIZE);
  #endif
    return MQTTClient::State::DISCONNECTED;
  };

  MQTTClient::State mqttState() override {
    switch (PubSubClient::state()) {
      case MQTT_CONNECTION_TIMEOUT: return MQTTClient::State::CONNECTION_TIMEOUT;
      case MQTT_CONNECTION_LOST: return MQTTClient::State::DISCONNECTED;
      case MQTT_CONNECT_FAILED: return MQTTClient::State::ERROR;
      case MQTT_DISCONNECTED: return MQTTClient::State::DISCONNECTED;
      case MQTT_CONNECTED: return MQTTClient::State::CONNECTED;
      case MQTT_CONNECT_BAD_PROTOCOL: return MQTTClient::State::ERROR;
      case MQTT_CONNECT_BAD_CLIENT_ID: return MQTTClient::State::BAD_CLIENT_ID;
      case MQTT_CONNECT_UNAVAILABLE: return MQTTClient::State::ERROR;
      case MQTT_CONNECT_BAD_CREDENTIALS: return MQTTClient::State::BAD_CREDENTIALS;
      case MQTT_CONNECT_UNAUTHORIZED: return MQTTClient::State::BAD_CREDENTIALS;
    };
    return MQTTClient::State::DISCONNECTED;
  };

  MQTTClient::State _mqttConnect() override {
    SIMPLEMQTT_DEBUG(F("Connecting...\n"));
    bool connected = false;
    if (MQTTClient::mqttWill != nullptr)
      connected = PubSubClient::connect(MQTTClient::mqttClientName, MQTTClient::mqttUser, MQTTClient::mqttPassword,
        MQTTClient::getFinalTopic(MQTTClient::mqttWill->getFullTopic()).c_str(), MQTTClient::mqttWill->getQoS(), MQTTClient::mqttWill->isRetained(), MQTTClient::mqttWill->getMessage(), MQTTClient::cleanSession);
    else
      connected = PubSubClient::connect(MQTTClient::mqttClientName, MQTTClient::mqttUser, MQTTClient::mqttPassword);

    if (!connected) {
      SIMPLEMQTT_DEBUG(F("Connect failed: %n\n"), PubSubClient::state());
      return MQTTClient::State::DISCONNECTED;
    }

    SIMPLEMQTT_DEBUG(F("Connected.\n"));

    return MQTTClient::State::CONNECTING;
  };

  MQTTClient::State mqttLoop() override {
    // PubSubClient loop
    loop();
    return mqttState();
  };

  bool mqttSubscribe(const String& topic, uint8_t qos) override {
    bool result = PubSubClient::subscribe(topic.c_str(), 0 /* qos */);  // QoS > 0 not supported???
    if (result) {
      SIMPLEMQTT_DEBUG(F("Subscribed to %s successfully.\n"), topic.c_str());
    } else {
      SIMPLEMQTT_ERROR(F("Subscribing to %s failed!\n"), topic.c_str());
    }
    return result;
  };
};
