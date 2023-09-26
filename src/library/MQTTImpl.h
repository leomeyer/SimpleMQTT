/////////////////////////////////////////////////////////////////////
// Specific class function implementations and external definitions
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

// define safe memory area for invalid topics
Topic_P(invalidTopic, "-");
MQTTTopic MQTTTopic::INVALID_TOPIC(nullptr, invalidTopic, 0);
const void* __internal::INVALID_PTR = (void*)&MQTTTopic::INVALID_TOPIC;

SimpleMQTTClient* MQTTTopic::getClient() {
  SIMPLEMQTT_CHECK_VALID(nullptr);
  return _parent->getClient();
}

void MQTTTopic::publish(bool) {
  SIMPLEMQTT_CHECK_VALID();
  getClient()->publish(this);
  config &= PUBLISH_CLEARMASK;
}

String MQTTTopic::getTopicPattern() {
  if (_parent != nullptr)
    return _parent->getTopicPattern();
  else
    return DEFAULT_TOPIC_PATTERN;
}

String MQTTTopic::getFullTopic(TopicOrder order) {
  SIMPLEMQTT_CHECK_VALID(String());
  const char* myName = name();
  if (myName[0] == '/' || _parent == nullptr)
    return String(myName);
  else if (_parent != nullptr) {
    switch (order) {
      case TopicOrder::BOTTOM_UP: return String(myName) + "/" + _parent->getFullTopic(order);
      default: return _parent->getFullTopic(order) + "/" + myName;
    }
  } else
    return String(myName);
}

String MQTTTopic::getFullTopic() {
  if (_parent == nullptr)
    return getFullTopic(TopicOrder::UNSPECIFIED);
  else
    return getFullTopic(_parent->getTopicOrder());
}

String MQTTTopic::getRequestTopic() {
  SIMPLEMQTT_CHECK_VALID(String());
  return parent().applyRequestPattern(this);
}

String MQTTTopic::getSetTopic() {
  SIMPLEMQTT_CHECK_VALID(String());
  return parent().applySetPattern(this);
}

void MQTTTopic::addSubscriptions(SimpleMQTTClient* client) {
  SIMPLEMQTT_CHECK_VALID();
  SIMPLEMQTT_DEBUG(PSTR("Adding subscriptions for '%s', config: %s\n"), getFullTopic().c_str(), getConfigStr().c_str());
  if (isTopicValid()) {
    if (isRequestable()) {
      String request_topic = client->getFinalTopic(getRequestTopic());
      const char* topic = request_topic.c_str();
      SIMPLEMQTT_DEBUG(PSTR("Subscribing to request with topic '%s'\n"), topic);
      client->subscribe(topic);
    }
    if (isSettable()) {
      String set_topic = client->getFinalTopic(getSetTopic());
      const char* topic = set_topic.c_str();
      SIMPLEMQTT_DEBUG(PSTR("Subscribing to set with topic '%s'\n"), topic);
      client->subscribe(topic);
    }
  } else
    SIMPLEMQTT_DEBUG(PSTR("Not valid, skipping: '%s'\n"), getFullTopic().c_str());
}

bool MQTTTopic::processPayload(SimpleMQTTClient* client, const char* topic, const char* payload) {
  SIMPLEMQTT_CHECK_VALID(false);
  if (isRequestable()) {
    // request topic received?
    if (client->getFinalTopic(getRequestTopic()) == topic) {
      SIMPLEMQTT_DEBUG(PSTR("Request for topic '%s' with payload '%s'\n"), topic, payload);
      switch (ResultCode code = requestReceived(payload)) {
        case ResultCode::OK:
          client->setStatus((int8_t)code, String(topic));
          break;
        default:
          client->setStatus((int8_t)code, String(topic), String(payload));
      }
      SIMPLEMQTT_DEBUG(PSTR("After request:%s"), " ");
      #ifdef SIMPLEMQTT_DEBUG_SERIAL
      printTo(SIMPLEMQTT_DEBUG_SERIAL);
      #endif
      return true;
    }
  }
  if (isSettable()) {
    // set topic received?
    if (client->getFinalTopic(getSetTopic()) == topic) {
      SIMPLEMQTT_DEBUG(PSTR("Set for topic '%s' with payload '%s'\n"), topic, payload);
      switch (ResultCode code = setReceived(payload)) {
        case ResultCode::OK:
          client->setStatus((int8_t)code, String(topic));
          break;
        default:
          client->setStatus((int8_t)code, String(topic), String(payload));
      }
      SIMPLEMQTT_DEBUG(PSTR("After set:%s"), " ");
      #ifdef SIMPLEMQTT_DEBUG_SERIAL
      printTo(SIMPLEMQTT_DEBUG_SERIAL);
      #endif
      return true;
    }
  }
  return false;
}

void MQTTGroup::addSubscriptions(SimpleMQTTClient* client) {
  SIMPLEMQTT_CHECK_VALID();
  MQTTTopic::addSubscriptions(client);
  ListNode* node = &nodes;
  while (node->next != nullptr) {
    MQTTTopic* value = node->data;
    value->addSubscriptions(client);
    node = node->next;
  }
}

bool MQTTGroup::processPayload(SimpleMQTTClient* client, const char* topic, const char* payload) {
  SIMPLEMQTT_CHECK_VALID(false);
  if (MQTTTopic::processPayload(client, topic, payload))
    return true;
  ListNode* node = &nodes;
  while (node->next != nullptr) {
    MQTTTopic* value = node->data;
    if (value->isTopicValid()) {
      if (value->processPayload(client, topic, payload))
        return true;
    }
    node = node->next;
  }
  return false;
}
