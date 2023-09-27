/////////////////////////////////////////////////////////////////////
// SimpleMQTT internal functions and management structures
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

namespace __internal {

  static char topicBuffer[SIMPLEMQTT_MAX_TOPIC_LENGTH + 1];
  const char* EMPTY = "";

  // static memory management >>
  #if SIMPLEMQTT_STATIC_MEMORY_SIZE > 0
    static uint8_t staticMemory[SIMPLEMQTT_STATIC_MEMORY_SIZE];
    static ptrdiff_t memPointer = 0;

    // satisfy alignment requirements
    #define SIMPLEMQTT_MEMALIGN         4
    #define SIMPLEMQTT_MEMBLOCK(size)   ((size + (SIMPLEMQTT_MEMALIGN - 1)) & ~(SIMPLEMQTT_MEMALIGN - 1))

    template<typename T>
    T* allocate(T object) {
      size_t size = SIMPLEMQTT_MEMBLOCK(sizeof(T));
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Allocating %d bytes for object at address %d...\n"), size, memPointer);
      if (memPointer + size > SIMPLEMQTT_STATIC_MEMORY_SIZE)
        return nullptr;
      memcpy(&staticMemory[memPointer], &object, size);
      memPointer += size;
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Success, new address is %d\n"), memPointer);
      return (T*)(void*)(&staticMemory[memPointer - size]);
    }

    template<typename T>
    T* allocate(size_t length) {
      if (length == 0)
        return nullptr;
      size_t size = SIMPLEMQTT_MEMBLOCK(length);
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Allocating %d bytes at address %d...\n"), size, memPointer);
      if (memPointer + size > SIMPLEMQTT_STATIC_MEMORY_SIZE)
        return nullptr;
      memPointer += size;
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Success, new address is %d\n"), memPointer);
      return (T*)(void*)(&staticMemory[memPointer - size]);
    }

    template<typename T>
    void deallocate(T*) {
      size_t size = SIMPLEMQTT_MEMBLOCK(sizeof(T));
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Deallocating %d bytes from address %d...\n"), size, memPointer);
      memPointer -= size;
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("New address is %d\n"), memPointer);
    }

    void deallocate(size_t length) {
      if (length == 0)
        return;
      size_t size = SIMPLEMQTT_MEMBLOCK(length);
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("Deallocating %d bytes from address %d...\n"), size, memPointer);
      memPointer -= size;
      if (SIMPLEMQTT_DEBUG_MEMORY)
        SIMPLEMQTT_DEBUG(PSTR("New address is %d\n"), memPointer);
    }
  #endif
  // << static memory management

  extern const void* INVALID_PTR;  // pointer to fallback object in case memory allocation fails

  template<typename T>
  T* checkAllocation(T* object, const char*
  #ifdef SIMPLEMQTT_ERROR_SERIAL
    type
  #endif
  ) {
    if (object == nullptr) {
      SIMPLEMQTT_ERROR(PSTR("Unable to allocate %d bytes for object of type %s!\n"), sizeof(T), type);
      return (T*)INVALID_PTR;
    }
    return object;
  };

  #if SIMPLEMQTT_STATIC_MEMORY_SIZE > 0
    #define SIMPLEMQTT_ALLOCATE_MEM(typeName, length)    __internal::allocate<typeName>(length)
    #define SIMPLEMQTT_ALLOCATE_CLASS(className, ...)    __internal::allocate(className(__VA_ARGS__))
    #define SIMPLEMQTT_ALLOCATE(className, ...)          __internal::checkAllocation<className>(SIMPLEMQTT_ALLOCATE_CLASS(className, __VA_ARGS__), #className);
    #define SIMPLEMQTT_DEALLOCATE_MEM(object, length)    __internal::deallocate(length)
    #define SIMPLEMQTT_DEALLOCATE(object)                __internal::deallocate(object);
  #else
    #define SIMPLEMQTT_ALLOCATE_MEM(typeName, length)    (typeName*)malloc(length)
    #define SIMPLEMQTT_ALLOCATE_CLASS(className, ...)    new (std::nothrow) className(__VA_ARGS__)
    #define SIMPLEMQTT_ALLOCATE(className, ...)          __internal::checkAllocation<className>(SIMPLEMQTT_ALLOCATE_CLASS(className, __VA_ARGS__), #className);
    #define SIMPLEMQTT_DEALLOCATE_MEM(object, length)    { free((void*)object); }
    #define SIMPLEMQTT_DEALLOCATE(object)                { free((void*)object); }
  #endif

  class _Topic {
  protected:
    const char* topic;
    uint8_t flags = 0;

  public:
    template<size_t N>
    _Topic(const char (&t)[N], bool progmem = false) : topic(t) {
      static_assert(N > 1, "Empty topic not allowed!");
      static_assert(N < SIMPLEMQTT_MAX_TOPIC_LENGTH, "Topic too long!");
      if (progmem)
        flags = 1;
    };

    _Topic(const __FlashStringHelper* t, bool)
      : topic((const char*)t) {
      flags = 1;
    };

    _Topic(const __FlashStringHelper* t)
      : topic((const char*)t) {
      flags = 1;
    };
	
    _Topic(String t) {
      topic = (const char*)SIMPLEMQTT_ALLOCATE_MEM(char, t.length() + 1);
      if (topic == nullptr) {
        SIMPLEMQTT_ERROR(PSTR("Unable to allocate memory for String topic '%s'\n"), t.c_str());
        topic = EMPTY;
      } else {
        if (SIMPLEMQTT_DEBUG_MEMORY)
          SIMPLEMQTT_DEBUG(PSTR("Allocated memory for String topic '%s'\n"), t.c_str());
        strcpy((char*)topic, t.c_str());
        flags = 8;
      }
    };

    virtual const char* get() const {
      // from RAM?
      if ((flags & 1) == 0)
        return topic;
      // from flash
      strncpy_P(topicBuffer, topic, SIMPLEMQTT_MAX_TOPIC_LENGTH);
      return topicBuffer;
    };

    bool isValid() {
      // not yet checked?
      if ((flags & 2) == 0) {
        flags |= 2 /* checked */ | (isTopicValid(get()) ? 4 : 0);
        if ((flags & 4) == 0) {
          SIMPLEMQTT_ERROR(PSTR("Invalid topic: '%s'\n"), get());
        }
      }
      return ((flags & 4) == 4);
    };

    void release() {
      if ((flags & 8) == 8)
        SIMPLEMQTT_DEALLOCATE_MEM(topic, strlen(topic) + 1);
    };
  };

  extern const _Topic invalidTopic;

  template<size_t N>
  static constexpr auto& CHECKTOPIC(const char (&t)[N]) {
    static_assert(N > 1, "Empty topic not allowed!");
    static_assert(N < SIMPLEMQTT_MAX_TOPIC_LENGTH, "Topic too long!");
    return t;
  }

}   // namespace __internal

#define Topic(t)          __internal::_Topic(__internal::CHECKTOPIC(t))
#define Topic_F(t)        __internal::_Topic(F(t), __internal::CHECKTOPIC(t) != nullptr)
#define Topic_P(name, t)  static auto& PROGMEM name##_pstr = __internal::CHECKTOPIC(t);  static __internal::_Topic name(name##_pstr, true);

#define SIMPLEMQTT_CHECK_VALID(retval) \
  if (this == __internal::INVALID_PTR) return retval;

#define SIMPLEMQTT_VALIDATE(topic, retval) \
  if (this == __internal::INVALID_PTR || !topic.isValid()) return retval;
