// for Arduino Uno and similar devices without on-board WiFi.

#include <SPI.h>
#include <Ethernet.h>

// you can use any of these MQTT libraries by simply including one
// #include <ArduinoMqttClient.h>
#include <PubSubClient.h>

#include "secrets.h"

// Update these with values suitable for your network.
byte mac[]= {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED};
EthernetClient ethClient;

#include <SimpleMQTT.h>
using State = MQTTClient::State;

#define CLIENT_NAME "avr_ethernet"
SimpleMQTTClient mqttClient(ethClient, CLIENT_NAME, MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
Topic_P(willTopic, "connected");
MQTTWill will(willTopic, "0");
uint32_t uptime_ms;
uint32_t lastPublishMillis;
// for calculation of free RAM
#if defined(__AVR__)
extern char __heap_start, *__brkval; 
int16_t free_ram;
#endif

void setup() {
  Serial.begin(9600);
  Serial.flush();
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println(F("\nStartup..."));

  // You can use Ethernet.init(pin) to configure the CS pin
  //Ethernet.init(10);  // Most Arduino shields
  //Ethernet.init(5);   // MKR ETH Shield
  //Ethernet.init(0);   // Teensy 2.0
  //Ethernet.init(20);  // Teensy++ 2.0
  //Ethernet.init(15);  // ESP8266 with Adafruit FeatherWing Ethernet
  //Ethernet.init(33);  // ESP32 with Adafruit FeatherWing Ethernet

  // The sketch will be much smaller if DHCP is not used.
  // If possible, use something like:
  // #define IP   IPAddress(a, b, c, d)
#ifdef IP
  Serial.print("Initializing Ethernet with fixed IP: ");
  Ethernet.begin(mac, IP);
#else
  Serial.print("Initializing Ethernet with DHCP: ");
  if (Ethernet.begin(mac) == 0) {
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected. Stopping.");
    } else {
      Serial.println("Unknown error. Stopping.");
    }
    while (true);
  }
#endif    

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found. Stopping.");
    while (true);
  }

  Serial.print("Success. IP address: ");
  Serial.println(Ethernet.localIP());

  auto& group = mqttClient.add(F("group"));
  // change this string by publishing to "avr_ethernet/group/string/set"
  group.add<String>(F("string")).set("set me!");
  
  mqttClient.add(F("uptime_ms"), &uptime_ms);
  #if defined(__AVR__)
  mqttClient.add(F("free_ram"), &free_ram);
  #endif

  will.set("1");
  mqttClient.setWill(&will);

  Serial.println("Setup done.");
}

void loop() {
  static State state = State::DISCONNECTED;

  if ((lastPublishMillis == 0) || (millis() - lastPublishMillis > 10000)) {
    uptime_ms = millis();
    lastPublishMillis = millis();
    Serial.print("Uptime: ");
    Serial.println(uptime_ms);
    Serial.print(F("MQTT client state: "));
    Serial.println(mqttClient.getStateText(state).c_str());
    #if defined(__AVR__)
    int v; 	
    free_ram = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
    #endif
  }

  State newState = mqttClient.handle(state);
  bool stateChange = newState != state;
  state = newState;

  if (stateChange) {
    lastPublishMillis = 0;
    Serial.print(F("MQTT client state: "));
    Serial.println(mqttClient.getStateText(state).c_str());
  }

  if (state == State::DISCONNECTED)
    digitalWrite(LED_BUILTIN, LOW);
  else
  if (state == State::CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
  }

  auto topic = mqttClient.getChange();
  while (topic != nullptr) {
    lastPublishMillis = 0;
    Serial.print(topic->name());
    Serial.print(" has been changed to: ");
    Serial.println(topic->getPayload().c_str());
    topic = mqttClient.getChange();
  }
}

