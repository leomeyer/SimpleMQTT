/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////


// SimpleMQTTClient specialization using PubSubClient

template <>
MQTTClient::State SimpleMQTTClient<PubSubClient>::mqttSetup() {
  // PubSubClient setup
  PubSubClient::setServer(mqttHost, mqttPort);
  PubSubClient::setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
    this->handleMessage(topic, (const char*)payload, length);
  });

#if SIMPLEMQTT_JSON_BUFFERSIZE > 0
  // increase buffer size to allow for larger Json messages
  PubSubClient::setBufferSize(SIMPLEMQTT_JSON_BUFFERSIZE);
#endif

  return State::DISCONNECTED;
};

template <>
bool SimpleMQTTClient<PubSubClient>::_mqttConnect() {
  if (mqttWill != nullptr)
    return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword, getFinalTopic(mqttWill->getFullTopic()).c_str(), mqttWill->getQoS(), mqttWill->isRetained(), mqttWill->getMessage(), cleanSession);
  else
    return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword);
};

template <>
MQTTClient::State SimpleMQTTClient<PubSubClient>::mqttState() {
  switch (PubSubClient::state()) {
    case MQTT_CONNECTION_TIMEOUT: return State::CONNECTION_TIMEOUT;
    case MQTT_CONNECTION_LOST: return State::CONNECTION_LOST;
    case MQTT_CONNECT_FAILED: return State::ERROR;
    case MQTT_DISCONNECTED: return State::DISCONNECTED;
    case MQTT_CONNECTED: return State::CONNECTED;
    case MQTT_CONNECT_BAD_PROTOCOL: return State::ERROR;
    case MQTT_CONNECT_BAD_CLIENT_ID: return State::BAD_CLIENT_ID;
    case MQTT_CONNECT_UNAVAILABLE: return State::ERROR;
    case MQTT_CONNECT_BAD_CREDENTIALS: return State::BAD_CREDENTIALS;
    case MQTT_CONNECT_UNAUTHORIZED: return State::BAD_CREDENTIALS;
  };
  return State::DISCONNECTED;
};

template <>
MQTTClient::State SimpleMQTTClient<PubSubClient>::mqttLoop() {
  // PubSubClient loop
  loop();
  return mqttState();
};

template <>
bool SimpleMQTTClient<PubSubClient>::mqttConnected() {
  return PubSubClient::connected();
};

template <>
bool SimpleMQTTClient<PubSubClient>::mqttPublish(const String& topic, const char* payload, bool retained, uint8_t qos, bool dup) {
  return PubSubClient::publish(getFinalTopic(topic).c_str(), payload, retained);
};

template <>
bool SimpleMQTTClient<PubSubClient>::mqttSubscribe(const String& topic, uint8_t qos) {
  return PubSubClient::subscribe(topic.c_str(), qos);
};

