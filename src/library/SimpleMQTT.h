/////////////////////////////////////////////////////////////////////
// SimpleMQTT library master include
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

#if (__cplusplus < 201703L)
  #error This library requires a C++ standard of at least C++17!
#endif

#ifdef FLASHEND
  #if (FLASHEND < 0x7FFF)
    #error This library requires at least 32 kB of program memory!
  #endif
  #if (FLASHEND == 0x7FFF)
    #define SIMPLEMQTT_OPTIMIZE_MEMORY  true
  #endif
#endif

#if defined(ESP8266) || defined(ESP32)
  #include <functional>
  #include <string>
  #define SIMPLEMQTT_HAS_STD_STRING true

#elif not __has_include("type_traits.h")  //defined(__AVR__)

  #include "type_traits.h"    // C++17 STL
  #include <alloca.h>

  // provide missing conversion functions   
  int64_t strtoll(const char* str, char** endptr, uint8_t format) {
    return 0;
  }
  uint64_t strtoull(const char* str, char** endptr, uint8_t format) {
    return 0;
  }
#endif

#ifndef SIMPLEMQTT_BUFFERSIZE
  #ifdef SIMPLEMQTT_OPTIMIZE_MEMORY
    #define SIMPLEMQTT_BUFFERSIZE 64
  #else
    #define SIMPLEMQTT_BUFFERSIZE 256
    // JSON is only available if there is enough memory
    #if SIMPLEMQTT_JSON_BUFFERSIZE > 0
      #include <ArduinoJson.h>
    #endif
  #endif
#endif

#define PubSubClientLibrary             1
#define ArduinoMqttClientLibrary        2

// check which client library to use
#if __has_include(<ArduinoMqttClient.h>)
  // https://github.com/arduino-libraries/ArduinoMqttClient
  #define SIMPLEMQTT_CLIENT_LIBRARY   ArduinoMqttClientLibrary
  #ifdef SIMPLEMQTT_DEBUG_SERIAL
    #pragma message "----------------> SimpleMQTTClient is using this MQTT library: ArduinoMqttClient"
  #endif
#elif __has_include(<PubSubClient.h>)
  // https://github.com/knolleary/pubsubclient
  #define SIMPLEMQTT_CLIENT_LIBRARY   PubSubClientLibrary
  #ifdef SIMPLEMQTT_DEBUG_SERIAL
    #pragma message "----------------> SimpleMQTTClient is using this MQTT library: PubSubClient"
  #endif
#else
  #ifdef SIMPLEMQTT_DEBUG_SERIAL
    #pragma message "A MQTT client library could not be determined. To directly connect to a MQTT broker please include one of [<ArduinoMqttClient.h>, <PubSubClient.h>]."
    #pragma message "The StreamClientImpl for use with a proxy is still available."
  #endif
#endif

#ifndef SIMPLEMQTT_MAX_STATIC_RAM
  #if defined(ESP8266) || defined(ESP32)
    #define SIMPLEMQTT_MAX_STATIC_RAM 4096
  #else
    #define SIMPLEMQTT_MAX_STATIC_RAM 256
  #endif
#endif

#define SIMPLEMQTT_RETRY_AFTER_ERROR  5000

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

// Serial stream default baud rate for proxy communication.
#ifndef SIMPLEMQTT_DEFAULT_PROXY_BAUDRATE
  #define SIMPLEMQTT_DEFAULT_PROXY_BAUDRATE 57600
#endif

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  #ifndef SIMPLEMQTT_ERROR_SERIAL
    #define SIMPLEMQTT_ERROR_SERIAL SIMPLEMQTT_DEBUG_SERIAL
  #endif
#endif

#ifndef SIMPLEMQTT_DEBUG_PREFIX
  #ifdef __AVR__
    #define SIMPLEMQTT_DEBUG_PREFIX  F("[MQTT] DBG ")
  #else
    #define SIMPLEMQTT_DEBUG_PREFIX  PSTR("[MQTT] DBG ")
  #endif
#endif

#ifndef SIMPLEMQTT_ERROR_PREFIX
  #ifdef __AVR__
    #define SIMPLEMQTT_ERROR_PREFIX  F("[MQTT] ERR ")
  #else
    #define SIMPLEMQTT_ERROR_PREFIX  PSTR("[MQTT] ERR ")
  #endif
#endif

#ifndef SIMPLEMQTT_TIMESTAMP
  #define SIMPLEMQTT_TIMESTAMP     PSTR("%d ms: "), millis()
#endif

#ifdef __AVR__
  // simple replacement for printf_P
  void _debugPrintf_P(Print& out, const char* fmt, va_list args) {
    PGM_P p = reinterpret_cast<PGM_P>(fmt);
    uint8_t c = pgm_read_byte(p++);
    while (c != 0) {
        if (c == '%') {
          char f = pgm_read_byte(p++);
          if (f == '\0') {
            out.print((char)c);
            break;
          }
          else
          if (f == 'd') {
              int i = va_arg(args, int);
              out.print(i);
          } else 
          if (f == 's') {
              char* s = va_arg(args, char*);
              out.print(s);
          } else {
            out.print(f);
            va_arg(args, int);
          }
        } else
          out.print((char)c);
      c = pgm_read_byte(p++);
    }
  }

  void debugPrintf(Print& out, const __FlashStringHelper* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    _debugPrintf_P(out, (const char*)fmt, args);
    va_end(args);
  }

  void debugPrintf(Print& out, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    uint8_t c = fmt[0];
    size_t pos = 0;
    while (c != 0) {
        if (c == '%') {
          char f = fmt[++pos];
          if (f == '\0') {
            out.print((char)c);
            break;
          }
          else
          if (f == 'd') {
              int i = va_arg(args, int);
              out.print(i);
          } else 
          if (f == 's') {
              char* s = va_arg(args, char*);
              out.print(s);
          } else {
            out.print(f);
            va_arg(args, int);
          }
        } else
          out.print((char)c);
      c = fmt[++pos];
    }
    va_end(args);
  }

  void debugPrintf(Print& out, int i) {
    out.print(i);
  }
#endif

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  #ifdef __AVR__
    #define SIMPLEMQTT_DEBUG(fmt, ...) { \
      SIMPLEMQTT_DEBUG_SERIAL.print(SIMPLEMQTT_DEBUG_PREFIX); \
      debugPrintf(SIMPLEMQTT_DEBUG_SERIAL, fmt, ##__VA_ARGS__); }
  #else
    #define SIMPLEMQTT_DEBUG(...)    { \
      SIMPLEMQTT_DEBUG_SERIAL.printf_P((const char*)SIMPLEMQTT_DEBUG_PREFIX); \
      SIMPLEMQTT_DEBUG_SERIAL.printf_P((const char*)SIMPLEMQTT_TIMESTAMP); \
      SIMPLEMQTT_DEBUG_SERIAL.printf_P((const char*)__VA_ARGS__); }
  #endif
#else
  #define SIMPLEMQTT_DEBUG(...)    {}
#endif

#ifdef SIMPLEMQTT_ERROR_SERIAL
  #ifdef __AVR__
    #define SIMPLEMQTT_ERROR(fmt, ...) { \
      SIMPLEMQTT_ERROR_SERIAL.print(SIMPLEMQTT_ERROR_PREFIX); \
      debugPrintf(SIMPLEMQTT_ERROR_SERIAL, fmt, ##__VA_ARGS__); }
  #else
    #define SIMPLEMQTT_ERROR(...)    { \
      SIMPLEMQTT_ERROR_SERIAL.printf_P((const char*)SIMPLEMQTT_ERROR_PREFIX); \
      SIMPLEMQTT_ERROR_SERIAL.printf_P((const char*)SIMPLEMQTT_TIMESTAMP); \
      SIMPLEMQTT_ERROR_SERIAL.printf_P((const char*)__VA_ARGS__); }
    #endif
#else
  #define SIMPLEMQTT_ERROR(...)    {}
#endif

#ifndef SIMPLEMQTT_DEBUG_MEMORY
  #define SIMPLEMQTT_DEBUG_MEMORY false
#endif

#ifdef SIMPLEMQTT_OPTIMIZE_MEMORY
  #ifndef SIMPLEMQTT_OPTIMIZE_NO_SETTERS
    #define SIMPLEMQTT_OPTIMIZE_NO_SETTERS true
  #endif
  #ifndef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
    #define SIMPLEMQTT_OPTIMIZE_NO_PATTERNS true
  #endif
#endif

#define SIMPLEMQTT_DEBUG_SET_FROM_PAYLOAD   SIMPLEMQTT_DEBUG(F("%s.setFromPayload: %s\n"), MQTTTopic::getFullTopic().c_str(), payload);

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
#ifndef SIMPLEMQTT_OPTIMIZE_NO_PATTERNS
  static String DEFAULT_TOPIC_PATTERN;
  static String DEFAULT_REQUEST_PATTERN;
  static String DEFAULT_SET_PATTERN;
#endif

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
  class MQTTClient;
  class MQTTGroup;
  template <typename T> class MQTTArray;

  // callback function type
#if defined(ESP8266) || defined(ESP32)
  typedef std::function<bool(MQTTClient* client, const char* topic, const char* payload, unsigned int length)> t_mqttCallback;
#else
  typedef bool (*t_mqttCallback)(MQTTClient* client, const char* topic, const char* payload, unsigned int length);
#endif

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

#if SIMPLEMQTT_CLIENT_LIBRARY == ArduinoMqttClientLibrary
  #include "ArduinoMqttClientImpl.h"
  #define SIMPLEMQTT_IMPL_CLASS  MQTTClientImpl<MqttClient, Client>
#elif SIMPLEMQTT_CLIENT_LIBRARY == PubSubClientLibrary
  #include "PubSubClientImpl.h"
  #define SIMPLEMQTT_IMPL_CLASS  PubSubClientImpl
#else
  // #error "MQTT_CLIENT_LIBRARY not defined or not supported"
  #include "StreamClientImpl.h"
  #define SIMPLEMQTT_IMPL_CLASS  MQTTStreamClientImpl
#endif

#ifdef SIMPLEMQTT_DEBUG_SERIAL
  #define _SM_VALUE_TO_STRING(x) #x
  #define _SM_VALUE(x) _SM_VALUE_TO_STRING(x)
  #define _SM_PRINT_MACRO_AT_COMPILE_TIME(text, var)  #text _SM_VALUE(var)
  #pragma message _SM_PRINT_MACRO_AT_COMPILE_TIME("----------------> The actual SimpleMQTTClient implementation is: ", _SM_VALUE_TO_STRING(SIMPLEMQTT_IMPL_CLASS))
#endif

  #include "Impl.h"

  using SimpleMQTTClient = SIMPLEMQTT_IMPL_CLASS;

} // namespace SimpleMQTT

using SimpleMQTTClient = SimpleMQTT::SimpleMQTTClient;

#ifndef SIMPLEMQTT_NO_AUTO_USING
using namespace SimpleMQTT;
#endif
