/////////////////////////////////////////////////////////////////////
// SimpleMQTT library master include
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

#if (__cplusplus < 201703L)
  #error This library requires a C++ standard of at least C++17!
#endif

#include "PubSubClient.h"  // https://github.com/knolleary/pubsubclient

#define SIMPLEMQTT_JSON_BUFFERSIZE    2048

#if SIMPLEMQTT_JSON_BUFFERSIZE > 0
  #include <ArduinoJson.h>
#endif

#ifndef SIMPLEMQTT_MAX_STATIC_RAM
  #if defined(ESP8266) || defined(ESP32)
    #define SIMPLEMQTT_MAX_STATIC_RAM 4096
  #endif
#endif

#define SIMPLEMQTT_PAYLOAD_HANDLER [](auto& object, const char* payload)

// Static memory buffer size for topic strings to be copied from PROGMEM.
#ifndef SIMPLEMQTT_MAX_TOPIC_LENGTH
  #define SIMPLEMQTT_MAX_TOPIC_LENGTH 32
#endif

// Buffer size for conversion of values on the stack. Does not consume static memory.
#ifndef SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER
  #define SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER 100
#endif

// #define SIMPLEMQTT_STATIC_MEMORY_SIZE  2048

#if SIMPLEMQTT_STATIC_MEMORY_SIZE > 0 && SIMPLEMQTT_STATIC_MEMORY_SIZE < 64
  #error Please reserve at least 64 bytes for static memory!
#endif
#if SIMPLEMQTT_STATIC_MEMORY_SIZE > 0 && SIMPLEMQTT_STATIC_MEMORY_SIZE > SIMPLEMQTT_MAX_STATIC_RAM
  #error Static memory too large!
#endif

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  #ifndef SIMPLEMQTT_ERROR_SERIAL
    #define SIMPLEMQTT_ERROR_SERIAL SIMPLEMQTT_DEBUG_SERIAL
  #endif
#endif

#ifndef SIMPLEMQTT_TIMESTAMP
  #define SIMPLEMQTT_TIMESTAMP     PSTR("%d ms: "), millis()
#endif

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  #define SIMPLEMQTT_DEBUG(...)    { SIMPLEMQTT_DEBUG_SERIAL.printf_P(SIMPLEMQTT_TIMESTAMP); SIMPLEMQTT_DEBUG_SERIAL.printf_P(__VA_ARGS__); }
#else
  #define SIMPLEMQTT_DEBUG(...)    {}
#endif
#ifdef SIMPLEMQTT_ERROR_SERIAL
  #define SIMPLEMQTT_ERROR(...)    { SIMPLEMQTT_ERROR_SERIAL.printf_P(SIMPLEMQTT_TIMESTAMP); SIMPLEMQTT_ERROR_SERIAL.printf_P(__VA_ARGS__); }
#else
  #define SIMPLEMQTT_ERROR(...)    {}
#endif

#ifndef SIMPLEMQTT_DEBUG_MEMORY
  #define SIMPLEMQTT_DEBUG_MEMORY false
#endif

#define SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD   SIMPLEMQTT_DEBUG(PSTR("%s.setFromPayload: %s\n"), MQTTTopic::getFullTopic().c_str(), payload);

namespace SimpleMQTT {

  enum class ResultCode : int8_t {
    OUT_OF_MEMORY = -127,
    INVALID_VALUE = -5,
    CANNOT_SET = -4,
    UNKNOWN_TOPIC = -3,
    INVALID_REQUEST = -2,
    INVALID_PAYLOAD = -1,
    OK = 0
  };

#define SIMPLEMQTT_DEFINE_BIT(name, bit) \
  static const uint8_t name##_BIT = bit; \
  static const uint8_t name##_CLEARMASK = ((uint8_t) ~(1 << bit)); \
  static const uint8_t name##_SETMASK = (1 << bit);

  // the semicolon is intentional to avoid ugly Arduino IDE re-formatting
  SIMPLEMQTT_DEFINE_BIT(REQUESTABLE, 2);
  SIMPLEMQTT_DEFINE_BIT(SETTABLE, 3);
  SIMPLEMQTT_DEFINE_BIT(AUTO_PUBLISH, 4);
  SIMPLEMQTT_DEFINE_BIT(CHANGED, 5);
  SIMPLEMQTT_DEFINE_BIT(PUBLISH, 6);
  SIMPLEMQTT_DEFINE_BIT(RETAINED, 7);

  enum class MQTTConfig : uint8_t {
    QOS_0 = 0,
    QOS_1 = 1,
    QOS_2 = 2,
    REQUESTABLE = REQUESTABLE_SETMASK,
    SETTABLE = SETTABLE_SETMASK,
    AUTO_PUBLISH = AUTO_PUBLISH_SETMASK,
    RETAINED = RETAINED_SETMASK
  };

  inline MQTTConfig operator|(MQTTConfig a, MQTTConfig b) {
    return static_cast<MQTTConfig>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
  };
  inline MQTTConfig operator+(MQTTConfig a, MQTTConfig b) {
    return static_cast<MQTTConfig>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
  };

  enum TopicOrder : uint8_t {
    UNSPECIFIED,
    TOP_DOWN,
    BOTTOM_UP
  };

  static MQTTConfig DEFAULT_CONFIG = MQTTConfig::AUTO_PUBLISH + MQTTConfig::SETTABLE + MQTTConfig::REQUESTABLE;
  static TopicOrder DEFAULT_TOPIC_ORDER = TopicOrder::TOP_DOWN;
  static String DEFAULT_TOPIC_PATTERN("%s");
  static String DEFAULT_REQUEST_PATTERN("%s/get");
  static String DEFAULT_SET_PATTERN("%s/set");

  static bool isTopicValid(const char* topic) {
      if (topic == nullptr)
          return false;
      if (topic[0] == '\0')
          return false;
      size_t l = strlen(topic);
      for (size_t i = 0; i < l; i++) {
          // may not contain wildcard characters or blanks
          if (topic[i] == '#' || topic[i] == '+' || topic[i] == ' ')
              return false;
          // may not contain a slash except at the first position
          if (i > 0 && topic[i] == '/')
              return false;
      }
      return true;
  }

  #include "Formats.h"

  #include "Internal.h"

  // forward class declarations
  class SimpleMQTTClient;
  class MQTTGroup;
  template <typename T> class MQTTArray;

  #include "MQTTTopic.h"

  #include "MQTTValue.h"

  #include "MQTTVariable.h"

  #include "MQTTReference.h"

  #include "MQTTArray.h"

  #include "MQTTFunction.h"

#if SIMPLEMQTT_JSON_BUFFERSIZE > 0
  #include "MQTTJson.h"
#endif

  #include "MQTTGroup.h"

  #include "MQTTWill.h"

  #include "MQTTClient.h"

  #include "Impl.h"

} // namespace SimpleMQTT

#ifndef SIMPLEMQTT_NO_AUTO_USING
using namespace SimpleMQTT;
#endif
