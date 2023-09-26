/////////////////////////////////////////////////////////////////////
// SimpleMQTT formatting options and routines
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT
/////////////////////////////////////////////////////////////////////

enum class NoFormat : uint8_t {};

enum class IntegralFormat : uint8_t {
  OCTAL = 8,
  DECIMAL = 10,
  HEXADECIMAL = 16
};

enum class BoolFormat : uint8_t {
  TRUEFALSE,  // "true"/"false"
  YESNO,      // "yes"/"no"
  ONOFF,      // "on"/"off"
  ONEZERO,    // "1"/"0"
  ANY         // output like TRUEFALSE. Input may be one of the above.
};

// global default presets
static BoolFormat DEFAULT_BOOL_FORMAT = BoolFormat::ANY;
static IntegralFormat DEFAULT_INTEGRAL_FORMAT = IntegralFormat::DECIMAL;
static char* DEFAULT_FLOAT_FORMAT = nullptr;
static char* DEFAULT_DOUBLE_FORMAT = nullptr;

// general format type template
template<typename T, typename Enable = void>
struct format_type { typedef NoFormat type; };

// format type template for integral types
template<typename T>
struct format_type<T, typename std::enable_if<std::is_integral<T>::value>::type> { typedef IntegralFormat type; };

// format type template for floating point types (char*)
template<typename T>
struct format_type<T, typename std::enable_if<std::is_floating_point<T>::value>::type> { typedef const char* type; };

// format type template for bool type
template<>
struct format_type<bool> { typedef BoolFormat type; };
template<>
struct format_type<const bool> { typedef BoolFormat type; };

// default values for the available formats
template <typename Format>
Format getDefaultFormat() { return Format{}; }

template <>
BoolFormat getDefaultFormat<BoolFormat>() { return DEFAULT_BOOL_FORMAT; }

template <>
IntegralFormat getDefaultFormat<IntegralFormat>() { return DEFAULT_INTEGRAL_FORMAT; }

namespace __internal {

  // format functions

  String boolToString(bool b, BoolFormat format) {
    switch (format) {
      case BoolFormat::ANY:
      case BoolFormat::TRUEFALSE: return String() + (b ? F("true") : F("false"));
      case BoolFormat::YESNO: return String() + (b ? F("yes") : F("no"));
      case BoolFormat::ONOFF: return String() + (b ? F("on") : F("off"));
      case BoolFormat::ONEZERO: return String() + (b ? F("1") : F("0"));
    }
    return String();
  }

  template <typename T, typename Format = NoFormat>
  String formatValue(T value, Format) { 
    String s;
    s += value;
    return s;
  }

  template <>
  String formatValue<bool>(bool value, BoolFormat format) { 
    return boolToString(value, format);
  }

  template <typename T>
  String formatValue(T value, IntegralFormat format) { 
    return String(value, (uint8_t)format); // == IntegralFormat::DEFAULT_INTFORMAT ? IntegralFormat::DECIMAL : format));
  }
/*
  template <>
  String formatValue(int64_t value, IntegralFormat) {
    // String does not support 64 bit value formatting (crash)
    return String(value);
  }

  template <>
  String formatValue(uint64_t value, IntegralFormat) { 
    // String does not support 64 bit value formatting (crash)
    return String(value);
  }
*/
  template <typename T>
  String formatValue(T value, const char* format) { 
    if (format == nullptr)
      return String(value);
    char buffer[SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER];
    snprintf(buffer, SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER, format, value);
    return String(buffer);
  }

// parse functions

  bool parseBool(const char* str, bool* b, BoolFormat format) {
    *b = false;
    switch (format) {
      case BoolFormat::TRUEFALSE:
        if (strcmp_P(str, PSTR("true")) == 0)
          *b = true;
        else if (strcmp_P(str, PSTR("false")) != 0)
          return false;
        return true;
      case BoolFormat::YESNO:
        if (strcmp_P(str, PSTR("yes")) == 0)
          *b = true;
        else if (strcmp_P(str, PSTR("no")) != 0)
          return false;
        return true;
      case BoolFormat::ONOFF:
        if (strcmp_P(str, PSTR("on")) == 0)
          *b = true;
        else if (strcmp_P(str, PSTR("off")) != 0)
          return false;
        return true;
      case BoolFormat::ONEZERO:
        if (strcmp_P(str, PSTR("1")) == 0)
          *b = true;
        else if (strcmp_P(str, PSTR("0")) != 0)
          return false;
        return true;
      case BoolFormat::ANY:
        if (parseBool(str, b, BoolFormat::TRUEFALSE)) return true;
        if (parseBool(str, b, BoolFormat::YESNO)) return true;
        if (parseBool(str, b, BoolFormat::ONOFF)) return true;
        if (parseBool(str, b, BoolFormat::ONEZERO)) return true;
        break;
    }
    return false;
  }

  template<typename T, typename Format = typename format_type<T>::type>
  bool parseIntegralType(const char* s, T* valptr, Format format) {
    if constexpr (std::is_const_v<T>)
      return false;
    if (*s == '\0')
      return false;
    char* endptr;
    int64_t v = strtoll(s, &endptr, (uint8_t)format);
    if (*endptr != '\0')
      return false;
    T value = (T)v;
    if (value != v)
      return false;
    if constexpr (!std::is_const_v<T>)
      *valptr = (T)v;
    else
      return false;
    return true;
  }

  // range checking templates for 64 bit types
  template<>
  bool parseIntegralType(const char* s, uint64_t* valptr, IntegralFormat format) {
    if (*s == '\0')
      return false;
    char* endptr;
    uint64_t v = strtoull(s, &endptr, (uint8_t)format);
    if (*endptr != '\0')
      return false;
    // compare with string
    String f = formatValue(v, format);
    if (f != s)
      return false;
    *valptr = v;
    return true;
  }

  template<>
  bool parseIntegralType(const char* s, int64_t* valptr, IntegralFormat format) {
    if (*s == '\0')
      return false;
    char* endptr;
    int64_t v = strtoll(s, &endptr, (uint8_t)format);
    if (*endptr != '\0')
      return false;
    // compare with string
    String f = formatValue(v, format);
    if (f != s)
      return false;
    *valptr = v;
    return true;
  }

  template<typename T, typename Format = typename format_type<T>::type>
  bool parseFractionalType(const char* s, T* valptr, Format format) {
    if constexpr (std::is_const_v<T>)
      return false;
    if (*s == '\0')
      return false;
    char* endptr;
    double v = strtod(s, &endptr);
    if (*endptr != '\0')
      return false;
    // no range check here (double != float for most cases)
    if (format != nullptr && format[0] != '\0') {
      // apply strict decimal conversion by converting to string and back
      String str;
      char buffer[SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER];
      snprintf(buffer, SIMPLEMQTT_FRACTIONAL_CONVERSION_BUFFER, format, v);
      str += buffer;
      v = strtod(str.c_str(), &endptr);
      if (*endptr != '\0')
        return false;
    }
    if constexpr (!std::is_const_v<T>)
      *valptr = (T)v;
    else
      return false;
    return true;
  }

  template <typename T, typename Format = typename format_type<T>::type> 
  typename std::enable_if<std::is_integral<T>::value, bool>::type parseValue(const char* str, T* value, Format format) { 
    return parseIntegralType<T>(str, value, format);
  }

  template <typename T, typename Format = typename format_type<T>::type>
  typename std::enable_if<std::is_floating_point<T>::value, bool>::type parseValue(const char* str, T* value, Format format) { 
    return parseFractionalType<T>(str, value, format);
  }

  bool parseValue(const char* str, bool* value, BoolFormat format) {
    String s(str);
    s.toLowerCase();
    if (s == "toggle") {
      *value = !*value;
      return true;
    }
    return parseBool(s.c_str(), value, format);
  }

  bool parseValue(const char* str, String* value, NoFormat) { 
    *value = String(str);
    return true;
  }
  
  bool parseValue(const char* str, const String* value, NoFormat) { 
    return false;
  }
  
} // namespace __internal
