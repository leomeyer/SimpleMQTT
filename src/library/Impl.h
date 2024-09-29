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

MQTTClient* MQTTTopic::getClient() {
  SIMPLEMQTT_CHECK_VALID(nullptr);
  return _parent->getClient();
}

void MQTTTopic::publish(bool) {
  SIMPLEMQTT_CHECK_VALID();
  getClient()->publish(this);
  config &= PUBLISH_CLEARMASK;
}

#ifndef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
String MQTTTopic::getTopicPattern() {
  if (_parent != nullptr)
    return _parent->getTopicPattern();
  else
    return DEFAULT_TOPIC_PATTERN;
}
#endif

String MQTTTopic::getFullTopic(TopicOrder order) {
  SIMPLEMQTT_CHECK_VALID(String());
  String myName(name());  // copy from common memory area
  if (myName.charAt(0) == '/' || _parent == nullptr)
    return myName;
  else if (_parent != nullptr) {
    switch (order) {
      case TopicOrder::BOTTOM_UP: return myName + '/' + _parent->getFullTopic(order);
      default: return _parent->getFullTopic(order) + '/' + myName;
    }
  } else
    return myName;
}

String MQTTTopic::getFullTopic() {
  if (_parent == nullptr)
    return getFullTopic(TopicOrder::UNSPECIFIED);
  else
    return getFullTopic(_parent->getTopicOrder());
}

String MQTTTopic::getRequestTopic() {
  SIMPLEMQTT_CHECK_VALID(String());
#ifdef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
  return getFullTopic() + F("/get");
#else
  return parent().applyRequestPattern(this);
#endif
}

String MQTTTopic::getSetTopic() {
  SIMPLEMQTT_CHECK_VALID(String());
#ifdef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
  return getFullTopic() + F("/set");
#else
  return parent().applySetPattern(this);
#endif
}

void MQTTTopic::addSubscriptions(MQTTClient* client) {
  SIMPLEMQTT_CHECK_VALID();
  SIMPLEMQTT_DEBUG(F("Preparing subscriptions for '%s', config: %s\n"), getFullTopic().c_str(), getConfigStr().c_str());
  if (isTopicValid()) {
    if (isRequestable()) {
      String request_topic = client->getFinalTopic(getRequestTopic());
      SIMPLEMQTT_DEBUG(F("  Subscribing request with topic '%s'\n"), request_topic.c_str());
      client->mqttSubscribe(request_topic, getQoS());
    }
    if (isSettable()) {
      String set_topic = client->getFinalTopic(getSetTopic());
      SIMPLEMQTT_DEBUG(F("  Subscribing set with topic '%s'\n"), set_topic.c_str());
      client->mqttSubscribe(set_topic, getQoS());
    }
  } else
    SIMPLEMQTT_DEBUG(F("Invalid topic, skipping: '%s'\n"), getFullTopic().c_str());
}

bool MQTTTopic::processPayload(MQTTClient* client, const char* topic, const char* payload) {
  SIMPLEMQTT_CHECK_VALID(false);
  if (isRequestable()) {
    // request topic received?
    if (client->getFinalTopic(getRequestTopic()) == topic) {
      SIMPLEMQTT_DEBUG(F("Request for topic '%s' with payload '%s'\n"), topic, payload);
      switch (ResultCode code = requestReceived(payload)) {
        case ResultCode::OK:
          client->setStatus((int8_t)code, String(topic));
          break;
        default:
          client->setStatus((int8_t)code, String(topic), String(payload));
      }
      SIMPLEMQTT_DEBUG(F("After request:%s"), " ");
      #ifdef SIMPLEMQTT_DEBUG_SERIAL
      printTo(SIMPLEMQTT_DEBUG_SERIAL);
      #endif
      return true;
    }
  }
  if (isSettable()) {
    // set topic received?
    if (client->getFinalTopic(getSetTopic()) == topic) {
      SIMPLEMQTT_DEBUG(F("Set for topic '%s' with payload '%s'\n"), topic, payload);
      switch (ResultCode code = setReceived(payload)) {
        case ResultCode::OK:
          client->setStatus((int8_t)code, String(topic));
          break;
        default:
          client->setStatus((int8_t)code, String(topic), String(payload));
      }
      SIMPLEMQTT_DEBUG(F("After set:%s"), " ");
      #ifdef SIMPLEMQTT_DEBUG_SERIAL
      printTo(SIMPLEMQTT_DEBUG_SERIAL);
      #endif
      return true;
    }
  }
  return false;
}

void MQTTGroup::addSubscriptions(MQTTClient* client) {
  SIMPLEMQTT_CHECK_VALID();
  MQTTTopic::addSubscriptions(client);
  ListNode* node = &nodes;
  while (node->next != nullptr) {
    MQTTTopic* value = node->data;
    value->addSubscriptions(client);
    node = node->next;
  }
}

bool MQTTGroup::processPayload(MQTTClient* client, const char* topic, const char* payload) {
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
