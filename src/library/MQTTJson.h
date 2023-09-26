/////////////////////////////////////////////////////////////////////
// Support for JSON topics
// Requires the define SIMPLEMQTT_JSON_BUFFERSIZE to be set.
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

class MQTTJsonTopic : public MQTTTopic {
friend class MQTTGroup;
friend class SimpleMQTTClient;

protected:
  StaticJsonDocument<SIMPLEMQTT_JSON_BUFFERSIZE> jdoc;
  JsonDocument* filter;

  MQTTJsonTopic(MQTTGroup* aParent, __internal::_Topic aTopic, uint8_t aConfig, JsonDocument* aFilter)
    : MQTTTopic(aParent, aTopic, aConfig), filter(aFilter) {};

  static ResultCode _deserialize(JsonDocument& newDoc, const char* payload, JsonDocument* filter = nullptr) {
    DeserializationError err = (filter == nullptr 
      ? deserializeJson(newDoc, payload) : deserializeJson(newDoc, payload, DeserializationOption::Filter(*filter)));
    switch (err.code()) {
      case DeserializationError::EmptyInput: return ResultCode::INVALID_PAYLOAD;
      case DeserializationError::IncompleteInput: return ResultCode::OUT_OF_MEMORY;
      case DeserializationError::InvalidInput: return ResultCode::INVALID_PAYLOAD;
      case DeserializationError::NoMemory: return ResultCode::OUT_OF_MEMORY;
      case DeserializationError::TooDeep: return ResultCode::INVALID_VALUE;
      case DeserializationError::Ok: return ResultCode::OK;
      default: return ResultCode::CANNOT_SET;
    }
  };

  virtual bool _set(JsonDocument& newDoc) {
    SIMPLEMQTT_CHECK_VALID(false);
    bool changed = jdoc != newDoc;
    jdoc = newDoc;
    if (MQTTTopic::isAutoPublish())
      MQTTTopic::republish();
    MQTTTopic::setChanged(MQTTTopic::hasBeenChanged(false) || changed);
    return changed;
  };

  ResultCode setReceived(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    return setFromPayload(payload);
  };

public:
  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTJsonTopic)

  virtual JsonDocument& doc() {
    return jdoc;
  };

  // JsonDocument delegate functions >>>>>
  auto operator[](size_t index) { return jdoc[index]; };
  auto operator[](const char* key) { return jdoc[key]; };
  auto operator[](char* key) { return jdoc[key]; };
  auto operator[](const String& key) { return jdoc[key]; };
  auto operator[](const std::string& key) { return jdoc[key]; };
  auto operator[](const __FlashStringHelper* key) { return jdoc[key]; };
  auto operator[](std::string_view key) { return jdoc[key]; };
  auto add() {
    return jdoc.add();
  };
  template <typename TValue>
  auto add(const TValue& value) {
    return jdoc.add(value);
  };
  template <typename TChar>
  auto add(TChar* value) {
    return jdoc.add(value);
  };
  template <typename T>
  auto to() {
    return jdoc.to<T>();
  };
  // <<<<< JsonDocument delegate functions

  String getPayload() const override {
    SIMPLEMQTT_CHECK_VALID(String());
    String json;
    serializeJson(jdoc, json);
    return json;
  };

  ResultCode setFromPayload(const char* payload) override {
    SIMPLEMQTT_CHECK_VALID(ResultCode::OUT_OF_MEMORY);
    SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD;

    DynamicJsonDocument newDoc(SIMPLEMQTT_JSON_BUFFERSIZE);
    ResultCode code = _deserialize(newDoc, payload, filter);
    if (code == ResultCode::OK)
      _set(newDoc);
    return code;
  };
};
