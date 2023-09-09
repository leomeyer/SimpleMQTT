#pragma once

#if (__cplusplus < 201703L)
  #error This library requires a C++ standard of at least C++17!
#endif

#include <PubSubClient.h>  // https://github.com/knolleary/pubsubclient

#ifndef SIMPLEMQTT_MAX_STATIC_RAM
  #if defined(ESP8266) || defined(ESP32)
    #define SIMPLEMQTT_MAX_STATIC_RAM 4096
  #endif
#endif

#define SIMPLEMQTT_PAYLOAD_HANDLER [](auto* object, const char* payload)

// Static memory buffer for topic strings to be copied from PROGMEM.
#ifndef SIMPLEMQTT_MAX_TOPIC_LENGTH
  #define SIMPLEMQTT_MAX_TOPIC_LENGTH 32
#endif

// Buffer size for conversion of values on the stack. Does not consume static memory.
#ifndef SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER
  #define SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER 100
#endif

#define SIMPLEMQTT_STATIC_MEMORY_SIZE  2048

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

  #include "src/Formats.h"

  #include "src/Internal.h"

  #include "src/MQTTTopic.h"

  #include "src/MQTTValue.h"

  #include "src/MQTTVariable.h"

  #include "src/MQTTReference.h"

  #include "src/MQTTArray.h"

  #include "src/MQTTGroup.h"

  #include "src/MQTTWill.h"

  #include "src/MQTTClient.h"

  #include "src/MQTTImpl.h"

} // namespace SimpleMQTT

#ifndef SIMPLEMQTT_NO_AUTO_USING
using namespace SimpleMQTT;
#endif

