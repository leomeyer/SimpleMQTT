/////////////////////////////////////////////////////////////////////
// MQTTWill: Special will topic with a predefined disconnect message.
// Add to SimpleMQTTClient (setWill) before connecting.
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

class MQTTWill : public MQTTReference<String> {
friend class SimpleMQTTClient;

protected:
  String str;
  const char* message = nullptr;

  SIMPLEMQTT_OVERRIDE_SETTERS(MQTTWill)

public:
  MQTTWill(__internal::_Topic aTopic, const char* aMessage, uint8_t qos = 0, bool retained = true)
    : MQTTReference<String>(nullptr, aTopic, (qos & 3) | AUTO_PUBLISH_SETMASK | (retained ? RETAINED_SETMASK : 0), str), message(aMessage) {};

  const char* getMessage() { return message; }

  virtual bool set(const char* str) {
    setFromPayload(str);
    return true;
  };
};

