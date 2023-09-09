// Basic example for the SimpleMQTT library.
// For ESP8266 and related devices.
// Copyright (c) Leo Meyer, leo@leomeyer.de
// Licensed under the MIT license.
// https://github.com/leomeyer/SimpleMQTT

#include <ESP8266WiFi.h>
#include "device_local.h"
#include "SimpleMQTT.h"

WiFiClient espClient;
SimpleMQTTClient mqttClient(espClient, CLIENT_NAME, MQTT_HOST, Topic(CLIENT_NAME));
auto myTopic = mqttClient.add("BasicTopic", "Hello SimpleMQTT!");

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
  setup_wifi();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    mqttClient.handle();
  }
}
