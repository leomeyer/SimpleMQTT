
# SimpleMQTT
An ESP8266/ESP32 library to simplify MQTT integration for IoT devices.

## Quickstart
Define a SimpleMQTTClient instance:

    WiFiClient espClient;
    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");

Add a topic:

    auto myTopic = mqttClient.add("my_topic", "Hello SimpleMQTT!");

Repeatedly call the `handle()` function:

    void loop() {
      if (WiFi.status() == WL_CONNECTED) {
    	mqttClient.handle();
      }
    }
    
The topic will be published on `test.mosquitto.org` at `simplemqtt/my_topic`. 
To change the value of the topic, publish a new value to `simplemqtt/my_topic/set`.

Full sketch:

    #include <ESP8266WiFi.h>
    #include "SimpleMQTT.h"
    
    WiFiClient espClient;
    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");
    auto myTopic = mqttClient.add("my_topic", "Hello SimpleMQTT!");
    
    void setup_wifi() {
      delay(10);
      WiFi.mode(WIFI_STA);
      WiFi.begin("<wifi_ssid>", "<wifi_password>");
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

## Features
- Supports all fundamental data types (integral types, floating point types, `bool`) as well as arrays of these types
- Supports character arrays (fixed length) and `String` objects (variable length)
- Can either manage topic values by itself or reference existing variables
- Can detect changes to registered variables and auto-publish if necessary
- Supports different formatting options depending on data type
- Conversion range checks ensure that only valid values are accepted
- Supports custom data types and individual validation functions
- Dynamic memory allocation (default) or fixed-size static memory with overflow crash protection
- Topic names may be static `char*` constants, dynamic `String` values or come from `PROGMEM`
- Simple management of more complex topic hierarchies
- Optional status message to provide feedback via an MQTT topic
- Optional will message for the broker to set on disconnect

## Requirements
This library requires the PubSubClient library (https://github.com/knolleary/pubsubclient).
The `SimpleMQTTClient` class extends the `PubSubClient` class which makes it easier to migrate existing code
as most functions of the `PubSubClient` base class can still be used.

## Topics
A  topic is the basic element of the SimpleMQTT implementation. It is represented by the `MQTTTopic` class.
A topic has a name that may not be empty or contain the characters `#`, `+` or ` ` (the blank character). It may not end with a `/` (slash).
Topics whose name does not start with a slash appear as subtopics of their parent topic. If the name starts with a slash the topic appears as its own "root topic", possibly outside of the parent topic's hierarchy.
The topmost parent topic is the one defined when the `SimpleMQTTClient` class is instantiated:

    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");

In this case `simplemqtt` becomes the "parent namespace" for all topics that are added to this `mqttClient` and whose names do not start with a slash.

To add a subtopic without a value of its own use:

	auto myGroup = mqttClient.add("my_group");
	
In this example `myGroup` can become the parent for additional topics, for example:

	auto myInt = myGroup->add("my_int", 10000);
	
`myInt` is a topic whose initial value is the integer `10000` and whose full topic name is `simplemqtt/my_group/my_int`.
Please note that function calls on the `mqttClient` object must use the dot syntax (`.`) whereas function calls on other objects need to use the arrow syntax (`->`).

To register a topic for an existing variable supply a pointer to the variable to the `add()` function:

	bool my_bool;
	auto myBool = mqttClient.add("my_bool", &my_bool);


## Topic configuration
You can specify whether you want a certain topic to be

- **settable** (i. e. SimpleMQTT will react to messages that change the value of this topic),
- **requestable** (i. e. SimpleMQTT will re-publish the current value of the topic on request),
- **auto-publishing** (i .e. if the topic refers to a variable it will be re-published when the variable's value has been changed),
- is **retained** (i. e. the MQTT broker will keep the topic even after the client has disconnected).
Additionally, you can specify the QoS (quality of service) of topics. It depends on the `PubSubClient` implementation and the MQTT broker's settings whether this specification has an effect or not.

By default, topics are settable, requestable and auto-publishing and have a QoS of 0. You can change the default configuration during the initialization of the `SimpleMQTTClient`class:

    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org", 
      MQTTConfig::REQUESTABLE + MQTTConfig::RETAINED + MQTTConfig::QOS_1);

Possible values of `MQTTConfig` are:  `REQUESTABLE`, `SETTABLE`, `AUTO_PUBLISH`,`RETAINED`, `QOS_0`, `QOS_1` and `QOS_2`.
When a topic is being added to its parent topic it automatically inherits the current configuration of the parent. You can also change a individual topic's configuration later on:

	auto myGroup =  mqttClient.add("my_group")->setSettable(false)->setRetained(true)->setQoS(1);

The topic configuration, however, must be applied before the `mqttClient.handle()` function is called for the first time. If you change the configuration after this point the behaviour is undefined.

## Setting and requesting topics via MQTT messages
If a topic is settable SimpleMQTT listens to the `/set`subtopic of the topic. When the MQTT broker publishes something for this subtopic SimpleMQTT attempts to change the value of the topic from the specified message payload.
Note that topics that refer to `const`variables are never settable.
How the payload is interpreted depends on the data type of the topic and its format. Generally, a value received via MQTT is only applied if it fits into the underlying data type. Exceptions are the 64 bit integer types and double floating point values. These values cannot be safely checked for underflow or overflow for technical reasons.

If the new value is different from the previous value the topic will be marked as changed. Its `hasBeenChanged()` function will return `true` in this case. Note that the default behavior is to remove the changed flag when `hasBeenChanged()` is called. If you don't want this you can use `hasBeenChanged(false)`.

Typically you would poll the topics of interest in the `loop()` function for changes. This requires you to declare the topics as global variables while adding them:

	WiFiClient espClient;
	SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");
	bool my_bool;
	auto myBool = mqttClient.add("my_bool", &my_bool);
	
	... setup code ...
	
	void loop() {
	  if (WiFi.status() == WL_CONNECTED) {
	    mqttClient.handle();
	  }

	  if (myBool->hasBeenChanged()) {
	    Serial.print("my_bool has been changed to: ");
	    Serial.println(my_bool ? "true" : "false");
	  }
	}


In most cases this is perfectly acceptable. If you want to avoid global topic variables, however, you can call the `getChange()` function to get any topic that has been changed. If no topic has been changed this function returns `nullptr`.

	void loop() {
	  if (WiFi.status() == WL_CONNECTED) {
	    mqttClient.handle();
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
	}

If a topic is requestable SimpleMQTT listens to the `/get` subtopic of this topic (except for the topmost topic, i. e. the`mqttClient` itself). If a message for this topic has been received (regardless of its payload) the requested topic will be published again. If you request a group topic that has subtopics all its subtopics will be published again.

A topic will also be published again if a `/set` message is received regardless of whether the message changes the value of the topic or not.

## Status message and debugging
To receive live feedback about success or failure of the received messages you can use SimpleMQTT's built-in status message function. All you need to do is add a status topic to your `SimpleMQTTClient` instance:

	void setup() {
	  mqttClient.setStatusTopic(F("status"));
	  ...
	}
To enable debugging output on a `Serial`stream (or similar) you can use the following defines before including `SimpleMQTT.h`:

	#define  SIMPLEMQTT_DEBUG_SERIAL Serial
	#define  SIMPLEMQTT_ERROR_SERIAL Serial
	#define  SIMPLEMQTT_DEBUG_MEMORY  true
	#include  "SimpleMQTT.h"

`SIMPLEMQTT_DEBUG_SERIAL` will implicitly define `SIMPLEMQTT_ERROR_SERIAL` if you leave it undefined. `SIMPLEMQTT_DEBUG_MEMORY` reveals additional information about static memory management and is mostly used for development purposes.

## Value formatting
`bool` values support three different formats: "true"/"false", "yes"/"no" and "1/0". Three is a special format called `ANY` that uses "true"/"false" as output format but accepts any of the three as input formats.
Usage:

	auto myBool = mqttClient.add("my_bool", &my_bool)->setFormat(BoolFormat::TRUEFALSE);
or

	auto myBool = mqttClient.add("my_bool", &my_bool)->setFormat(BoolFormat::YESNO);
or

	auto myBool = mqttClient.add("my_bool", &my_bool)->setFormat(BoolFormat::ONEZERO);
or

	auto myBool = mqttClient.add("my_bool", &my_bool)->setFormat(BoolFormat::ANY);

Integral values (`uint8_t`,  `int8_t`,  `uint16_t`,  `int16_t`,  `uint32_t`,  `int32_t`) may be formatted as decimal, hexadecimal or octal numbers. Note that the 64 bit values do not support such formatting.

	auto myInt = myGroup->add(F("my_int"), 10000)->setFormat(IntegralFormat::DECIMAL);
or

	auto myInt = myGroup->add(F("my_int"), 10000)->setFormat(IntegralFormat::HEXADECIMAL);
or

	auto myInt = myGroup->add(F("my_int"), 10000)->setFormat(IntegralFormat::OCTAL);

Floating point values can be formatted using a format string specification such as commonly used with `Serial.printf()` or similar functions. For example, to specify five decimal digits you can use:

	auto myFloat = myGroup->add(F("my_float"), 300000000.0f)->setFormat("%.5f");
The default number of decimals for floating point values is 2. This default is determined by the board libraries. To explicitly apply the default formatting use:

	auto myFloat = myGroup->add(F("my_float"), 300000000.0f)->setFormat(nullptr);

If numeric values are being set via MQTT messages the payload is parsed according to the formatting specification of the topic. For floating point values this means that the received value is rounded such that the internal representation does not exceed the specified number of digits in the format string.

To control the default presets for the different data type formats you can use the following global variables:

	namespace SimpleMQTT {
		static BoolFormat DEFAULT_BOOL_FORMAT = BoolFormat::ANY;
		static IntegralFormat DEFAULT_INTEGRAL_FORMAT = IntegralFormat::DECIMAL;
		static char* DEFAULT_FLOAT_FORMAT = nullptr;
		static char* DEFAULT_DOUBLE_FORMAT = nullptr;
	}
	
For example, to set the default format for all integral topics to hexadecimal in your sketch, use 

	SimpleMQTT::DEFAULT_INTEGRAL_FORMAT = IntegralFormat::HEXADECIMAL;

in your `setup()` function.

## Topic value validation
SimpleMQTT tries to ensure that values that don't fit into a topic's data type do not result in unspecified behavior. This works well except for 64 bit integral and floating point values. If a payload is invalid it is indicated via the status message.

If you want to further restrict the values that you want to accept for a given topic you can specify a payload handler to parse, check and provide feedback on the supplied payload. For example:

	auto myInt = myGroup->add(F("my_int"), 10)->setPayloadHandler([](auto* object, const char* payload) {
	  int newValue;
	  if (!object->parseValue(payload, &newValue))
		return ResultCode::INVALID_PAYLOAD;
	  if (newValue < 0 || newValue > 10) {
		object->getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object->getFullTopic(), 
		  "Only values between 0 and 10 allowed!");
		return ResultCode::INVALID_VALUE;
	  }
	  object->set(newValue, true);
	  return ResultCode::OK;
	});
This lambda validation function first uses the object's `parseValue()` function to get the integer value of the payload. This function already applies all the necessary format conversion and data type range checks. Note that this function may also fail, in which case the result code `INVALID_PAYLOAD` should be returned.
Next, the function checks whether the value is within the accepted range and sets the status message accordingly if this is not the case. Otherwise the `set()` function is called which takes care of the internal topic flags,

## Arrays
SimpleMQTT can also work with arrays of the fundamental data types. To add an array topic you have to supply a pointer to the array and the array length to the add function:

	float floatArray[] = {1.02323f, 12.324524f, 545.132323f, 5465464.3434f, -1235354.3434f, 0660956.45435f};
	auto floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float));

This topic will be published to the MQTT broker with the payload `1.02,12.32,545.13,5465464.50,-1235354.38,660956.44` (because the default number of decimals is 2). To specify a different format for an array topic's elements use the `setElementFormat()` function whose parameter type depends on the underlying data type just like the `setFormat()` function described above:

	auto floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float))
	  ->setElementFormat("%.5f");

In this case the payload will be represented as `1.02323,12.32452,545.13232,5465464.50000,-1235354.37500,660956.43750`.

Topics of arrays that are `const`  cannot be set. Non-const arrays can be set via MQTT messages by publishing a payload containing the new values to the `/set` subtopic of the array topic, with the following rules:

- The new array's values must be separated by a comma `,` in the payload
- Empty values do not modify the array element at a given position
- Incomplete values (i .e. fewer elements than the length of the array) are allowed
- Surplus elements are ignored.

The individual array values can be custom-validated with a payload handler just like the example above, except that you use the arrays `element()` function to access the underlying object that is responsible for the conversion:

	floatArrayTopic->element()->setPayloadHandler([](auto* object, const char* payload) {
	  float newValue;
	  if (!object->parseValue(payload, &newValue))
	    return ResultCode::INVALID_PAYLOAD;
	  if (newValue < -10000.0f || newValue > 10000.0f) {
	    object->getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object->getFullTopic(), 
	      "Only values between -10000 and 10000 allowed!");
	    return ResultCode::INVALID_VALUE;
	  }
	  object->set(newValue, true);
	  return ResultCode::OK;
	});

Note that you should perform any `element()`  function calls in the `setup()` function (or similar) rather than during the addition of the topic if you want to have the array topic as a global variable, such as in

	auto floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float));
because the `element()`  function returns a different object that does not represent the array topic but its internal conversion helper and is therefore not the object you would really want here. Also, avoid calling `setPayloadHandler()` on the array topic itself because in this case you will have to re-implement the array parse logic which is probably not what you want, either.

## Memory management
Usually it is recommended (except if you are really tight on static RAM) to use dynamic memory management. If limited static RAM becomes a problem the first step is usually to move as many static `char` arrays as possible to the `PROGMEM`.  This way string constants won't take any space in static RAM; the drawback is a small runtime overhead needed for copying the string from flash memory to the DRAM. So, instead of using static char arrays like

	bool my_bool;
	auto myBool = mqttClient.add("my_bool", &my_bool);
you can declare the topic name first as a flash memory string using the `Topic_P` macro:

	bool my_bool;
	Topic_P(myBoolTopic, "my_bool");
	auto myBool = mqttClient.add(myBoolTopic, &my_bool);
This is the preferred way if you need the topic variable (`myBool` ) globally for later access.

If you add your topics during `setup()` time things become even more easy. You do not have to declare the topic name beforehand; instead you just use a flash memory string defined using the macro `F()`:

	auto myLong = myGroup->add(F("my_long"), 200000L);

A SimpleMQTT topic uses roughly 40 bytes of RAM (depending on the data type and not including the topic name) plus 8 bytes of RAM for internal management. Usually this memory is allocated on demand on the heap using `new()`.  Alternatively you can specify a maximum static RAM size for SimpleMQTT to use:

	#define SIMPLEMQTT_STATIC_MEMORY_SIZE 2048
	#include "SimpleMQTT.h"
In this case SimpleMQTT will use a maximum of about 2048 bytes of RAM for topics and internal management structures. This case is most useful if you add topics dynamically. If a topic that is to be added does not fit into the specified RAM amount any more it is discarded along with its management structure. To avoid a crash the returned pointer is not invalid, though; instead it points to a dummy object that is kept for this purpose. If you run into memory issues you may try this method but be aware that you may lose the ability to handle certain topics and the crash safeguard may not work in all cases, either.
Removing of once-added topics is not possible. For most IoT use cases it should be unnecessary. If you need to change your topic structure depending on device configuration you should perform a reboot and do so during initial setup.

## Topic access
To inspect the topic hierarchy in your SimpleMQTT client instance you can use the `printTo()`  function that takes a `Stream&`, for example, `Serial`, as parameter. A topic group has a `size()` function that returns the amount of its child topics. Individual topics can be accessed using the index operator with numeric index:

	mqttClient[0]->printTo(Serial);

