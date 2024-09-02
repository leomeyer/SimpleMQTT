/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// MQTTClientImpl specialization using PubSubClient

template <>
MQTTClient::State MQTTClientImpl<PubSubClient>::mqttSetup() {
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
bool MQTTClientImpl<PubSubClient>::_mqttConnect() {
  if (mqttWill != nullptr)
    return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword, getFinalTopic(mqttWill->getFullTopic()).c_str(), mqttWill->getQoS(), mqttWill->isRetained(), mqttWill->getMessage(), cleanSession);
  else
    return PubSubClient::connect(mqttClientName, mqttUser, mqttPassword);
};

template <>
MQTTClient::State MQTTClientImpl<PubSubClient>::mqttState() {
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
MQTTClient::State MQTTClientImpl<PubSubClient>::mqttLoop() {
  // PubSubClient loop
  loop();
  return mqttState();
};

template <>
bool MQTTClientImpl<PubSubClient>::mqttConnected() {
  return PubSubClient::connected();
};

template <>
bool MQTTClientImpl<PubSubClient>::mqttPublish(const String& topic, const char* payload, bool retained, uint8_t qos, bool dup) {
  return PubSubClient::publish(getFinalTopic(topic).c_str(), payload, retained);
};

template <>
bool MQTTClientImpl<PubSubClient>::mqttSubscribe(const String& topic, uint8_t qos) {
  return PubSubClient::subscribe(topic.c_str(), qos);
};

