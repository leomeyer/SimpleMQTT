

#define WIFI_SSID "<wifi_ssid>"             // the name of the network to connect to
#define WIFI_PASSWORD "<wifi_password>"     // network password if needed

#define CLIENT_NAME "simplemqtt"            // name of the device; should be unique on your network

#define MQTT_HOST "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_USER ""                        // optional; if required by the MQTT host
#define MQTT_PASSWORD ""                    // ditto
#define MQTT_PREFIX CLIENT_NAME             // global topic prefix for all topics managed by the device
