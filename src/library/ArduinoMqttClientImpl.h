/////////////////////////////////////////////////////////////////////
// MQTTClient
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// MQTTClientImpl specialization using MqttClient from the ArduinoMqttClient library

namespace __ArduinoMQTT {
  // Inner hidden namespace for message callback function.
  // Unfortunately this is necessary because this library does not accept a lambda as callback
  // which means that only one instance of this client can be used at a time. 

  MQTTClientImpl<MqttClient>* client = nullptr;

  void arduinoMqttClientOnMessage(int length) {
    auto thisClient = static_cast<MQTTClientImpl<MqttClient>*>(client);
    String topic = client->messageTopic();
    String payload = "";
    while (client->available())
      payload += (char)client->read();
    thisClient->handleMessage(topic.c_str(), payload.c_str(), length);
  }

}   // namespace __ArduinoMQTT

template <>
MQTTClient::State MQTTClientImpl<MqttClient>::mqttSetup() {
  // MqttClient setup
  MqttClient::setId(mqttClientName);
  MqttClient::setUsernamePassword(mqttUser, mqttPassword);
  MqttClient::setCleanSession(cleanSession);
  __ArduinoMQTT::client = this;
  MqttClient::onMessage(__ArduinoMQTT::arduinoMqttClientOnMessage);

#if SIMPLEMQTT_JSON_BUFFERSIZE > 0
  // increase payload buffer size to allow for larger Json messages
  MqttClient::setTxPayloadSize(SIMPLEMQTT_JSON_BUFFERSIZE);
#endif

  return State::DISCONNECTED;
};

template <>
bool MQTTClientImpl<MqttClient>::_mqttConnect() {
  if (!connect(mqttHost, mqttPort)) {
    // print out the error message:
    Serial.print("MQTT connection failed. Error no: ");
    Serial.println(connectError());
    return false;
  }

  if (mqttWill != nullptr) {
    beginWill(mqttWill->name(), mqttWill->getPayload().length(), mqttWill->isRetained(), mqttWill->getQoS());
    print(mqttWill->getPayload());
    endWill();
  }

  return true;
};

template <>
MQTTClient::State MQTTClientImpl<MqttClient>::mqttState() {
  switch (MqttClient::connectError()) {
    case MQTT_CONNECTION_REFUSED: return State::ERROR;
    case MQTT_CONNECTION_TIMEOUT: return State::CONNECTION_TIMEOUT;
    case MQTT_SUCCESS: return State::CONNECTED;
    case MQTT_UNACCEPTABLE_PROTOCOL_VERSION: return State::ERROR;
    case MQTT_IDENTIFIER_REJECTED: return State::BAD_CLIENT_ID;
    case MQTT_SERVER_UNAVAILABLE: return State::ERROR;
    case MQTT_BAD_USER_NAME_OR_PASSWORD: return State::BAD_CREDENTIALS;
    case MQTT_NOT_AUTHORIZED: return State::BAD_CREDENTIALS;
  };
  return State::DISCONNECTED;
};

template <>
MQTTClient::State MQTTClientImpl<MqttClient>::mqttLoop() {
  // MqttClient loop
  poll();
  return mqttState();
};

template <>
bool MQTTClientImpl<MqttClient>::mqttConnected() {
  return MqttClient::connected();
};

template <>
bool MQTTClientImpl<MqttClient>::mqttPublish(const String& topic, const char* payload, bool retained, uint8_t qos, bool dup) {
  if (!MqttClient::connected())
    return false;

  if (!beginMessage(getFinalTopic(topic), strlen(payload), retained, qos, dup))
    return false;
  print(payload);
  if (!endMessage())
    return false;  

  return true;
};

template <>
bool MQTTClientImpl<MqttClient>::mqttSubscribe(const String& topic, uint8_t qos) {
  return MqttClient::subscribe(topic, qos);
};

