
#if defined(ESP8266)
  #include <ESP8266WiFi.h>

  extern "C" {
  #include "user_interface.h"
  }
#elif defined(ESP32)
  #include <WiFi.h>
#endif

#include "secrets.h"

#define CLIENT_NAME "simplemqtt"

#define SIMPLEMQTT_DEBUG_SERIAL Serial
// #define SIMPLEMQTT_ERROR_SERIAL Serial
// #define SIMPLEMQTT_DEBUG_MEMORY true

#include <SimpleMQTT.h>

using State = SimpleMQTTClient::State;

// static initialization
WiFiClient espClient;

SimpleMQTTClient mqttClient(espClient, CLIENT_NAME, MQTT_HOST);

MQTTWill will("connected", "0");

// fundamental values

auto& values = mqttClient.add("values");

#define ADD_VALUE(parent, type, name) \
  auto& parent##name##Topic = parent.add<type>(#name, 42);

ADD_VALUE(values, bool, bool);
ADD_VALUE(values, int8_t, int8);
ADD_VALUE(values, uint8_t, uint8);
ADD_VALUE(values, int16_t, int16);
ADD_VALUE(values, uint16_t, uint16);
/*
ADD_VALUE(values, int32_t, int32);
ADD_VALUE(values, uint32_t, uint32);
ADD_VALUE(values, int64_t, int64);
ADD_VALUE(values, uint64_t, uint64);
ADD_VALUE(values, float, float);
ADD_VALUE(values, double, double);
*/
// variables

auto& variables = mqttClient.add("variables");

#define ADD_VARIABLE(parent, type, name) \
  type parent##name##Var = 42; \
  auto& parent##name##VarTopic = parent.add(#name, &parent##name##Var); \
  const type parent##name##Var_const{ 42 }; \
  auto& parent##name##VarConstTopic = parent.add(#name "_const", &parent##name##Var_const);

ADD_VARIABLE(variables, bool, bool);
ADD_VARIABLE(variables, int8_t, int8);
ADD_VARIABLE(variables, uint8_t, uint8);
ADD_VARIABLE(variables, int16_t, int16);
ADD_VARIABLE(variables, uint16_t, uint16);
/*
ADD_VARIABLE(variables, int32_t, int32);
ADD_VARIABLE(variables, uint32_t, uint32);
ADD_VARIABLE(variables, int64_t, int64);
ADD_VARIABLE(variables, uint64_t, uint64);
ADD_VARIABLE(variables, float, float);
ADD_VARIABLE(variables, double, double);
*/
// arrays

auto& arrays = mqttClient.add("arrays");
bool arraysboolArray[] = { false, true, false, true, true, false, true, false };
auto& arraysboolArrayTopic = arrays.add("bool", arraysboolArray);
const bool arraysboolArray_const[] = { false, true, false, true, true, false, true, false };
auto& arraysboolArrayTopic_const = arrays.add("bool_const", arraysboolArray_const);

#define ADD_ARRAY(parent, type, name) \
  type parent##name##Array[] = { 42, 43, 44, 45, 46, 47, 48, 49 }; \
  auto& parent##name##ArrayTopic = parent.add(#name, parent##name##Array); \
  const type parent##name##Array_const[] = { 42, 43, 44, 45, 46, 47, 48, 49 }; \
  auto& parent##name##ArrayConstTopic = parent.add(#name "_const", parent##name##Array_const);
ADD_ARRAY(arrays, int8_t, int8);
ADD_ARRAY(arrays, uint8_t, uint8);
ADD_ARRAY(arrays, int16_t, int16);
ADD_ARRAY(arrays, uint16_t, uint16);
/*
ADD_ARRAY(arrays, int32_t, int32);
ADD_ARRAY(arrays, uint32_t, uint32);
ADD_ARRAY(arrays, int64_t, int64);
ADD_ARRAY(arrays, uint64_t, uint64);
ADD_ARRAY(arrays, float, float);
ADD_ARRAY(arrays, double, double);
*/

// strings

auto& strings = mqttClient.add("strings");
char* fixedLengthString = (char*)"fixed length string";
auto& fixedLengthStringTopic = strings.add("fixedLengthString", fixedLengthString);
const char constantString1[] = "constant string 1";
auto& constantString1Topic = strings.add("constantString1", constantString1);
auto& constantString2Topic = strings.add("constantString2", "constant string 2");
const String constantString3("constant string 3");
auto& constantString3Topic = strings.add("constantString3", constantString3);
auto& variableString1Topic = strings.add("variableString1", new String("variable string 1"));
auto& variableString2Topic = strings.add<String>("variableString2").setTo("variable string 2");
String variableString3("variable string 3");
auto& variableString3Topic = strings.add("variableString3", variableString3);

// groups

auto& group = mqttClient.add("group");

ADD_VALUE(group, uint8_t, uint8);
ADD_VARIABLE(group, float, float);
ADD_ARRAY(group, int, int);
auto& groupstring = group.add<String>("string").setTo("groupstring");

// subgroup with variables from a struct
auto& testGroup = group.add("subgroup");

typedef struct {
  const int tsInt;
  float tsFloat;
  int8_t tsSmall;
  String tsString;
  bool tsBool;
} TestStruct;
TestStruct testStruct{ 1, 100.5, -42, { "Hello World!" }, true };

auto& groupedInt = testGroup.add("int", &testStruct.tsInt);
auto& groupedFloat = testGroup.add("float", &testStruct.tsFloat);
auto& groupedSmall = testGroup.add("small", &testStruct.tsSmall);
auto& groupedString = testGroup.add("string", &testStruct.tsString);
auto& groupedBool = testGroup.add("bool", &testStruct.tsBool);

// validation tests

auto& validation = mqttClient.add("validation");
auto& validatedInt = validation.add<int>("int0..10")
                       .setFormat(IntegralFormat::OCTAL)
                       .setPayloadHandler([](auto& object, const char* payload) {
                         decltype(object.value()) newValue;
                         if (!object.parseValue(payload, &newValue))
                           return ResultCode::INVALID_PAYLOAD;
                         if (newValue < 0 || newValue > 10) {
                           object.getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object.getFullTopic() + ": Only values between 0 and 10 allowed!");
                           return ResultCode::INVALID_VALUE;
                         }
                         object.set(newValue);
                         return ResultCode::OK;
                       });

auto& validatedFloat = validation.add<float>("float-1000..1000")
                         .setFormat("%.3f")
                         .setPayloadHandler(SIMPLEMQTT_PAYLOAD_HANDLER {
                           decltype(object.value()) newValue;
                           if (!object.parseValue(payload, &newValue))
                             return ResultCode::INVALID_PAYLOAD;
                           if (newValue < -1000.0f || newValue > 1000.0f) {
                             object.getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object.getFullTopic() + ": Only values between -1000 and 1000 allowed!");
                             return ResultCode::INVALID_VALUE;
                           }
                           object.set(newValue);
                           if (object.hasBeenChanged())
                             Serial.printf_P("Value of %s has been changed to %s\n", object.name(), object.getPayload().c_str());
                           return ResultCode::OK;
                         });

auto& validatedDoubleArray = validation.add<double, 8>("double-10000..10000").setFormat("%0.5f");

// top-level topic test

auto& ac1Temp = mqttClient.add("/MHI-AC-Ctrl-1").add<float>("Tsht21");

// JSON

auto& json = mqttClient.add("json");
auto& testJson = json.addJsonTopic("testJson");
//auto& gosund1Status = json["/stat"].add("gosund1").addJsonTopic("STATUS8");
StaticJsonDocument<200> gosund1StatusFilter;
auto& gosund1Status = json["/stat"].add("gosund1").addJsonTopic("STATUS8", &gosund1StatusFilter);

// functions

auto& functions = mqttClient.add("functions");

// get function
auto& fnMillis = functions.add("millis", &millis);

// set function
void logToSerial(const char* s) {
  Serial.println(s);
}
auto& fnLog = functions.add("log", &logToSerial);

// get/set function
String fnGetSetString("getSetStringValue");
String fnGet() {
  return fnGetSetString;
}
void fnSet(String s) {
  fnGetSetString = s;
}
auto& fnGetSet = functions.add("getSetString", &fnGet, &fnSet);

// stability tests

// adding "group" fails because it already exists, must not produce crash
auto& dummy = mqttClient.add("group")
                .add("i", 1)
                .parent()
                .add("l", 1L)
                .parent()
                .add("b", false)
                .parent()
                .add("f", 1.0f);

// must fail at compile time!
// Topic_P(emptyTest, "");
// auto& m1 = mqttClient.add<String>("");
// auto& m1 = mqttClient.add<String>("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

// global variables
uint32_t uptime_ms;
uint32_t lastPublishMillis;

// info topics
auto& message = mqttClient.add<String>("message");
auto& free_heap = mqttClient.add<uint32_t>("free_heap");

// test definitions
#define DEFAULT_TEST_TIMEOUT 2500
typedef void (*TestAction)();
typedef bool (*TestCheck)();

struct Test {
  const char* name;
  TestAction action;
  TestCheck check;
  bool succeed = true;
  int timeout = DEFAULT_TEST_TIMEOUT;
};

#define PUBLISH_MUST_MODIFY_EQUAL(topic, value) []() { mqttClient.publish(mqttClient[#topic].getSetTopic().c_str(), #value); }, []() { return mqttClient[#topic].getPayload() == #value; }
#define PUBLISH_MUST_NOT_MODIFY_UNEQUAL(topic, value, compare) []() { mqttClient.publish(mqttClient[#topic].getSetTopic().c_str(), #value); }, []() { return mqttClient[#topic].getPayload() != #compare; }, false

Test tests[] = {
  { "Set bool value",                             PUBLISH_MUST_MODIFY_EQUAL(values/bool, false) },
  { "Set int8 value",                             PUBLISH_MUST_MODIFY_EQUAL(values/int8, -42) },
  { "Check int8 value range",                     PUBLISH_MUST_NOT_MODIFY_UNEQUAL(values/int8, 255, -42) },
  { "Set bool variable",                          PUBLISH_MUST_MODIFY_EQUAL(variables/bool, false) },
  { "Set int8 variable",                          PUBLISH_MUST_MODIFY_EQUAL(variables/int8, -42) },
  { "Check int8 variable range",                  PUBLISH_MUST_NOT_MODIFY_UNEQUAL(variables/int8, 255, -42) },
  { "Check validation",                           PUBLISH_MUST_NOT_MODIFY_UNEQUAL(validation/int0..10, 42, 0) },
  { nullptr, nullptr, nullptr }
};

Test* currentTest = tests;
uint32_t testStart;
int testsTotal;
int testsFailed;
bool runTests;

// setup functions

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setup_wifi();

  //  SimpleMQTT::DEFAULT_TOPIC_PATTERN = "test/%s";

  // DEFAULT_INTEGRAL_FORMAT = IntegralFormat::HEXADECIMAL;
  mqttClient.setStatusTopic(Topic_F("status"));

  // must fail at compile time!
  // Topic_F("");

  mqttClient.add(F("xxx")).add(F("yyy"), "Test yyy...");


  // we're only interested in the voltage; init filter for JSON parser
  gosund1StatusFilter[F("StatusSNS")][F("ENERGY")][F("Voltage")] = true;

  message = F("Message");

  validatedDoubleArray.element().setPayloadHandler([](auto& object, const char* payload) {
    decltype(object.value()) newValue;
    if (!object.parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    if (newValue < -10000.0f || newValue > 10000.0f) {
      object.getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object.getFullTopic() + ": Only values between -10000 and 10000 allowed!");
      return ResultCode::INVALID_VALUE;
    }
    object.set(newValue);
    return ResultCode::OK;
  });

  testJson["Test"] = "Hello Json!";
  testJson["Number"] = 2;
  testJson["Array"].add(42);
  will.set("1");
  mqttClient.setWill(&will);
  mqttClient.add(F("uptime_ms"), &uptime_ms);

  auto& dynamic = mqttClient.add(String("dynamic"));
  for (int i = 0; i < 10; i++) {
    dynamic.add<int8_t>(String("dynamic") + i, i).setSettable(false);
  }

  mqttClient.printTo(Serial);

  mqttClient.get(0).printTo(Serial);
  mqttClient[1].printTo(Serial);
  mqttClient.get("x").printTo(Serial);
  mqttClient["testgroup"].printTo(Serial);
  mqttClient["testgroup/float"].printTo(Serial);
  mqttClient["/special"].printTo(Serial);
  mqttClient["/special/periodicint"].printTo(Serial);

  Serial.print("arraysboolArray[0] = ");
  Serial.println(arraysboolArrayTopic[0] ? "true" : "false");
  arraysboolArrayTopic[0] = "true";
  Serial.print("arraysboolArray[0] = ");
  Serial.println(arraysboolArrayTopic[0] ? "true" : "false");

  Serial.print("arraysboolArray_const[0] = ");
  Serial.println(arraysboolArrayTopic_const[0] ? "true" : "false");
}

void loop() {
  static State state = State::DISCONNECTED;

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    state = mqttClient.handle(state);

    if (state == State::RECONNECTED) {
      lastPublishMillis = millis();
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (mqttClient.connected()) {

    if (millis() - lastPublishMillis > 10000) {
      uptime_ms = millis();
      int uptime = millis() / 1000;
      free_heap = system_get_free_heap_size();
      message = String("Uptime: ") + uptime + " seconds";
      testGroup.republish();
      testJson["Uptime"] = uptime;
      testJson.republish();

      lastPublishMillis = millis();
      runTests = true;
    }

    if (ac1Temp.hasBeenChanged()) {
      message = "AC1 temp is now: " + ac1Temp.getPayload() + " °C";
      testJson["ac1Temp"] = ac1Temp.value();
      testJson.republish();
      Serial.printf("%s\n", message.getPayload().c_str());
    }
    if (gosund1Status.hasBeenChanged()) {
      message = String() + "Voltage is now: " + gosund1Status[F("StatusSNS")][F("ENERGY")][F("Voltage")].as<int16_t>() + " V";
      testJson["gosund1Status"] = gosund1Status.getPayload();
      testJson.republish();
      Serial.printf("%s\n", message.getPayload().c_str());
    }

    // detect and print changes
    auto topic = mqttClient.getChange();
    while (topic != nullptr) {
      Serial.print("Changed: ");
      Serial.print(topic->name());
      Serial.print(" to: ");
      Serial.println(topic->getPayload());
      topic = mqttClient.getChange();
    }

    if (runTests) {
      if (currentTest->name != nullptr && currentTest->action != nullptr && currentTest->check != nullptr) {
        if (testStart == 0) {
          testsTotal++;
          Serial.print("Test: ");
          Serial.print(currentTest->name);
          Serial.print(": ");
          testStart = millis();
          currentTest->action();
        } else {
          bool timeout = millis() > testStart + currentTest->timeout;
          // wait until successful result or timeout
          // if "succeed" is true, the check must return true before the timeout
          // else, the check must not return true before the timeout
          if (currentTest->succeed) {
            if (currentTest->check()) {
              Serial.println("SUCCESS");
              currentTest++;
              testStart = 0;
            } else
            if (timeout) {
              Serial.println("FAILED (timeout)");
              testsFailed++;
              currentTest++;
              testStart = 0;
            }
          } else {
            if (timeout) {
              Serial.println("SUCCESS");
              currentTest++;
              testStart = 0;
            } else
            if (currentTest->check()) {
              Serial.println("FAILED (check)");
              testsFailed++;
              currentTest++;
              testStart = 0;
            }
          }
        }
      } else {
        // tests completed
        if (testStart == 0) {
          if (testsFailed > 0)
            Serial.printf_P(PSTR("\nFailed %d of %d tests total.\n"), testsFailed, testsTotal);
          else
            Serial.printf_P(PSTR("\nAll %d tests succeeded.\n"), testsTotal);
            testStart = 1;
        }
      } // completed

    } // runTests
  } // connected

}
