
#include <ESP8266WiFi.h>


extern "C" {
#include "user_interface.h"
}

#include "device_local.h"

#define SIMPLEMQTT_DEBUG_SERIAL Serial
// #define SIMPLEMQTT_ERROR_SERIAL Serial
// #define SIMPLEMQTT_DEBUG_MEMORY true

#include "library/SimpleMQTT.h"
using State = SimpleMQTTClient::State;


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

// static initialization
WiFiClient espClient;

SimpleMQTTClient mqttClient(espClient, CLIENT_NAME, MQTT_HOST, Topic(CLIENT_NAME));
MQTTWill will(Topic("connected"), "0");

Topic_P(testBoolTopic, "testbool");
auto testboolValue = mqttClient.add<bool>(testBoolTopic, true)->setSettable(true)->setFormat(SimpleMQTT::BoolFormat::YESNO);

auto testint8Value = mqttClient.add<int8_t>(Topic("testint8"))->setSettable(true);
auto testuint8Value = mqttClient.add<uint8_t>(Topic("testuint8"))->setSettable(true);
auto testint16Value = mqttClient.add<int16_t>(Topic("testint16"))->setSettable(true);
auto testuint16Value = mqttClient.add<uint16_t>(Topic("testuint16"))->setSettable(true);
auto testint32Value = mqttClient.add<int32_t>(Topic("testint32"))->setSettable(true);
auto testuint32Value = mqttClient.add<uint32_t>(Topic("testuint32"))->setSettable(true);
auto testint64Value = mqttClient.add<int64_t>(Topic("testint64"))->setSettable(true);
auto testuint64Value = mqttClient.add<uint64_t>(Topic("testuint64"))->setSettable(true);
auto testfloatValue = mqttClient.add<float>(Topic("testfloat"))->setSettable(true)->setFormat("%0.5f");
auto testdoubleValue = mqttClient.add<double>(Topic("testdouble"), 123.456789876543)->setSettable(true)->setFormat("%0.10f");

int intvar;
auto intvarValue = mqttClient.add(Topic("intvar"), &intvar)->setSettable(true);
float floatvar;
auto floatvarValue = mqttClient.add(Topic("floatvar"), &floatvar)->setSettable(true)->setFormat("%0.4f");
double doublevar;
auto doublevarValue = mqttClient.add(Topic("doublevar"), &doublevar)->setSettable(true)->setFormat("%0.8f");


auto testMessageValue = mqttClient.add(Topic("testmessage"), "Test!");
auto testPeriodicIntValue = mqttClient.add<int>(Topic("/special/periodicint"));

const float floatArray[] = {1.02323f, 12.324524f, 545.132323f, 5465464.3434f, -1235354.3434f, 0660956.45435f};
auto floatArrayTopic = mqttClient.add(Topic("float_array"), floatArray, sizeof(floatArray) / sizeof(float))->setSettable(true); //->element()->setFormat("%0.4f");

double doubleArray[] = {1.02323f, 12.324524f, 545.132323f, 5465464.3434f, -1235354.3434f, 0660956.45435f};
auto doubleArrayTopic = mqttClient.add(Topic("double_array"), doubleArray, sizeof(doubleArray) / sizeof(double))->setSettable(true)->setElementFormat("%0.8f");

auto validatedInt = mqttClient.add<int>(Topic("validated_int"));
auto free_heap = mqttClient.add<uint32_t>(Topic("free_heap"));

auto x = mqttClient.add<int>(Topic("x"), 42);

typedef struct {
  const int tsInt;
  float tsFloat;
  int8_t tsSmall;
  String tsString;
  bool tsBool;
} TestStruct;
TestStruct testStruct{1, 100.5, -42, {"Hello World!"}, true};

auto testGroup = mqttClient.add(Topic("testgroup"))->setSettable(true);
auto groupedInt = testGroup->add(Topic("int"), &testStruct.tsInt);
auto groupedFloat = testGroup->add(Topic("float"), &testStruct.tsFloat)->setFormat("%0.3f");
auto groupedSmall = testGroup->add(Topic("small"), &testStruct.tsSmall);
auto groupedString = testGroup->add(Topic("string"), testStruct.tsString);
auto groupedBool = testGroup->add(Topic("bool"), &testStruct.tsBool)->setFormat(SimpleMQTT::BoolFormat::TRUEFALSE);

auto testGroup2 = testGroup->add(Topic("group2"));
auto groupedInt2 = testGroup2->add(Topic("int"), -1);

char* charptr = "0123456789";
auto groupedString2 = testGroup2->add(Topic("charptr"), charptr);

int intArray[] = {100, 50, 25, 12, 6, 3, 1, 0};
auto groupedIntArray = testGroup2->add(Topic("intArray"), intArray, sizeof(intArray) / sizeof(int));

bool boolArray[] = {true, false, true, true};
auto groupedBoolArray = testGroup2->add(Topic("boolArray"), boolArray, sizeof(boolArray) / sizeof(bool))->setFormat(SimpleMQTT::BoolFormat::TRUEFALSE);

char* testCharArray = "Test Char Array";
auto testCharArrayValue = mqttClient.add(Topic("testCharArray"), testCharArray, strlen(testCharArray))->setSettable(true);

String testString("Test String");
auto testStringValue = mqttClient.add(Topic("teststring"), testString)->setSettable(true);

uint32_t lastPublishMillis;



void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setup_wifi();

  mqttClient.setStatusTopic(Topic_F("status"));

  intvarValue->setAutoPublish(true);

  auto testMessage2Value = mqttClient.add(Topic("testmessage2"), "Test 2!")->setRequestable(true)
    ->setSettable(true)
    ->setAutoPublish(true);

  testMessageValue
    ->setRequestable(true)
    ->setSettable(true)
    ->setAutoPublish(false);

  validatedInt
    ->setSettable(true)
    ->setPayloadHandler([](auto* object, const char* payload) {
      int newValue;
      if (!object->parseValue(payload, &newValue))
        return ResultCode::INVALID_PAYLOAD;
      if (newValue < 0 || newValue > 10) {
        object->getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object->getFullTopic() + ": Only values between 0 and 10 allowed!");
        return ResultCode::INVALID_VALUE;
      }
      object->set(newValue, true);
      return ResultCode::OK;
    })->setFormat(IntegralFormat::OCTAL);

  doubleArrayTopic->element()->setPayloadHandler([](auto* object, const char* payload) {
    double newValue;
    if (!object->parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    if (newValue < 0 || newValue > 10) {
      object->getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object->getFullTopic() + ": Only values between 0 and 10 allowed!");
      return ResultCode::INVALID_VALUE;
    }
    object->set(newValue, true);
    return ResultCode::OK;
  });

  will.set("1");
  mqttClient.setWill(&will);
  mqttClient.add<int8_t>(Topic_F("int8"), 100)->setSettable(true)->setFormat(IntegralFormat::HEXADECIMAL);
  mqttClient.add<int8_t>(Topic_F("int8_2"))->setSettable(true);
  mqttClient.add<bool>(Topic_F("bool"))->setSettable(true)->setFormat(BoolFormat::ANY);

  mqttClient.add<float>(Topic_F("float/1"))->setSettable(true)->setFormat("%.3f")  
  ->setPayloadHandler(SIMPLEMQTT_PAYLOAD_HANDLER {
    float newValue;
    if (!object->parseValue(payload, &newValue))
      return ResultCode::INVALID_PAYLOAD;
    object->set(newValue, true);
    if (object->hasBeenChanged())
      Serial.printf_P("Value of %s has been changed to %s\n", object->getTopic(), object->getPayload().c_str());
    return ResultCode::OK;
  });
  mqttClient.add<int8_t>(String("dynamic"));
  for (int i = 0; i < 10; i++) {
    mqttClient.add<int8_t>(String("dynamic") + i, i)->setSettable(false);
  }
}

void loop() {
  static State state = State::DISCONNECTED;

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    state = mqttClient.handle(state);

    // Serial.printf("my state = %d\n", state);
    // delay(250);

    if (state == State::RECONNECTED) {
      lastPublishMillis = millis();
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (mqttClient.connected()) {
/*
    if (millis() - lastPublishMillis > 1000) {
      intvar = millis() / 1000;
      // testMessageValue->set(String("Uptime: ") + intvar + " seconds");
      free_heap->set(system_get_free_heap_size());
      testString = String("Uptime: ") + intvar + " seconds";
      lastPublishMillis = millis();
      testGroup->republish();
    }
*/
    if (groupedBool->hasBeenChanged()) {
      Serial.printf_P(PSTR("bool = %s\n"), groupedBool->getPayload().c_str());
    }
    if (groupedBoolArray->hasBeenChanged()) {
      Serial.printf_P(PSTR("boolArray = %s\n"), groupedBoolArray->getPayload().c_str());
    }
    if (groupedIntArray->hasBeenChanged()) {
      Serial.printf_P(PSTR("intArray = %s\n"), groupedIntArray->getPayload().c_str());
    }
    if (groupedString->hasBeenChanged()) {
      Serial.println(testStruct.tsString);
    }
    if (testStringValue->hasBeenChanged()) {
      Serial.println(testString);
    }
    if (testdoubleValue->hasBeenChanged()) {
      Serial.println(testdoubleValue->get());
    }   
  }
}
