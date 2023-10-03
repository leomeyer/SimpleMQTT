


# SimpleMQTT
A C++ library to simplify MQTT integration for IoT devices.

## Quickstart
Define a `SimpleMQTTClient` instance:

    WiFiClient espClient;
    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");

Add a topic:

    auto& myTopic = mqttClient.add("my_topic", "Hello SimpleMQTT!");

Repeatedly call the `handle()` function:

    void loop() {
      if (WiFi.status() == WL_CONNECTED) {
    	mqttClient.handle();
      }
    }
    
The topic will be published on `test.mosquitto.org` at `simplemqtt/my_topic`. 

Full sketch:

    #include <ESP8266WiFi.h>
    #include "SimpleMQTT.h"
    
    WiFiClient espClient;
    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");
    auto& myTopic = mqttClient.add("my_topic", "Hello SimpleMQTT!");
    
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
- Supports character arrays (fixed length) and `String` objects (variable length), both constant or modifiable
- Can either manage topic values by itself or refer to existing variables in your program
- Can detect changes to variables and automatically publish them if necessary
- Supports subscribing to any topic on an MQTT broker (to listen to other devices' messages)
- Optional support for working with complex JSON structures (using ArduinoJson)
- Supports different formatting options depending on data type
- Conversion range checks ensure that only valid values are accepted
- Supports custom data types and individual validation functions
- Dynamic memory allocation (default) or fixed-size static memory with overflow crash protection
- Topic names may be static `char*` constants, dynamic `String` values or come from `PROGMEM`
- Simple management of complex topic hierarchies
- Optional status message to provide feedback via an MQTT topic
- Optional will message for the broker to set on disconnect
- Control topic tree layout by specifying patterns for request, set, and topics themselves

## Requirements
**A C++ compiler with standard C++17 or higher is required. The current version of AVR-GCC supplied with the Arduino IDE (7.3.0) does not fully support C++17. This means that you cannot currently use this library with AVR microprocessors.**

Supported Arduino IDE boards:
- ESP8266

This library requires the [PubSubClient library](https://github.com/knolleary/pubsubclient).
The `SimpleMQTTClient` class extends the `PubSubClient` class which makes it easy to migrate existing code
as most functions of the `PubSubClient` base class can still be used without changes.

If you want to easily work with JSON payloads you need to install the [ArduinoJson library](https://arduinojson.org/) and define a JSON memory buffer size before including `SimpleMQTT.h`: 

	#define SIMPLEMQTT_JSON_BUFFERSIZE  1024

## Topics
A  topic is the basic element of the SimpleMQTT implementation. It may either serve as a group (a "container" that contains other topics) or refer to a certain value or variable that represents some state in your program.
### Topic name rules
A topic has a name that may not be empty or contain the characters `#`, `+` or ` ` (the blank character). It may not contain a `/` (slash) except at the very start of the name.
Topics whose name does not start with a slash appear as subtopics of their parent topic. If the name starts with a slash the topic appears as its own "top-level topic" which can be outside of the parent topic's hierarchy. This is useful if you want to listen to messages from other devices or services. Special rules apply to [Top-level topics](#top-level-topics).

The topmost parent topic is specified when the `SimpleMQTTClient` class is instantiated:

    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");

In this example`simplemqtt` will be the "parent namespace" for all topics that are added to this `mqttClient` and whose names do not start with a slash.

For readability most of the following examples use static `char` array constants as topic names. The topic names may also be stored in flash memory (`PROGMEM`), however. For details how to do this please see section [Memory Management](#memory-management) below.

### Adding topics
To add a group topic without a value of its own use:

	auto& myGroup = mqttClient.add("my_group");
	
`myGroup` can become the parent for additional topics, for example:

	auto& myInt = myGroup.add("my_int", 10000);
	
Group topics are used to give more complex topic trees an orderly structure. They can publish all their subtopics recursively at once which is useful when a number of topics represents some common state (i. e. they are interdependent or should be published together).

`myInt` is a topic whose initial value is the integer `10000` and whose full topic name is `simplemqtt/my_group/my_int`.

To explicitly state which data type is to be used you can supply a template parameter to the `add()` function, by either supplying an initial value or not:

	auto& myInt8 = myGroup.add<int8_t>("my_int8");
	auto& myInt16 = myGroup.add<int16_t>("my_int16", 10000);
	
Or you can let the compiler infer the data type from the value argument:

	auto& myLong = myGroup.add("my_long", 10000L);

To register a topic for an existing variable supply a pointer to the variable to the `add()` function:

	bool my_bool;
	auto& myBool = mqttClient.add("my_bool", &my_bool);

The data type of the topic will be determined by the data type of the underlying variable. Please do not use pointers to objects such as `String` (see [Working with string topics](#working-with-string-topics) below for examples).

If you don't care about the global topic variables you can use a shortcut syntax to add more complex topic hierarchies. Example:

	auto& dummy = mqttClient.add("group")
	              .add("i", 1)
	              .parent().add("l", 1L)
	              .parent().add("b", false)
	              .parent().add("f", 1.0f);
Calling the `parent()` function on the added values yields the `group` topic which can then be used to add further topics.

You can add dynamic topics at runtime as well (do this before the `handle()` function is first called, for example during `setup()`). In these cases pass a `String` object that contains the topic name to the `add()` function:

	for  (int i = 0; i < 10; i++)  {
		mqttClient.add(String("dynamic") + i, i);
	}
### Setting and getting topic values
A topic representing a value or referring to a variable provides a `set(T)` method that you can use to modify its value. The parameter type `T` of this function is the underlying data type.  You can also assign values using the `=` operator, provided the underlying data type is not `const`:

	myInt = 42;
is equivalent to

	myInt.set(42);

The `value()` method returns the current value of the topic. Or you can use the topic variable directly:

	Serial.println(myInt.value() + 1);
is equivalent to

	Serial.println(myInt + 1);
	

You can use these functions only if you have remembered the topic as a variable during topic adding. Only then will the topic's data type be known. There is no provision to determine a topic's data type at runtime and you cannot use topic objects in a type-safe manner if you do not store their references in dedicated variables.

However, to set any topic's value from a string you can use the `setFromPayload(const char* payload)` function.  It depends on the type of topic whether it will accept the value or not, and the `payload` must conform to the current formatting specifications of the topic.
To get the current value of a topic as a `String` you can use the `getPayload()` function.

Setting a topic's value will cause it to be published if the topic is configured as auto-publishing (see [Topic configuration](#topic-configuration). This also applies if a settable topic is set via its MQTT set message, regardless of whether its value has actually been changed or not.

## Topic configuration
You can specify whether you want a certain topic to be

- **settable** (i. e. SimpleMQTT will react to MQTT messages that change the value of this topic),
- **requestable** (i. e. SimpleMQTT will re-publish the current value of the topic on request),
- **auto-publishing** (i .e. the topic will publish itself if necessary as opposed to manual publishing),
- is **retained** (i. e. the MQTT broker will keep the topic even after the client has disconnected).

Additionally, you can specify the QoS (quality of service) of topics. It depends on the `PubSubClient` implementation and the MQTT broker's settings whether this specification has an effect or not.

By default, topics are settable, requestable and auto-publishing and have a QoS of 0. You can change the default configuration during the initialization of the `SimpleMQTTClient`class:

    SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org", 
      MQTTConfig::REQUESTABLE + MQTTConfig::RETAINED + MQTTConfig::QOS_1);

Possible values of `MQTTConfig` are:  `REQUESTABLE`, `SETTABLE`, `AUTO_PUBLISH`,`RETAINED`, `QOS_0`, `QOS_1` and `QOS_2`.
When a topic is being added to its parent topic it automatically inherits the current configuration of the parent. You can also change a individual topic's configuration later on:

	auto& myGroup =  mqttClient.add("my_group").setSettable(false).setRetained(true).setQoS(1);

The topic configuration, however, should be applied before the `mqttClient.handle()` function is called for the first time. You can change topic configuration after this point but this is not recommended.

## Setting and requesting topics via MQTT messages
If a topic is settable SimpleMQTT listens to the `/set`subtopic of the topic by default. When the MQTT broker publishes something for this subtopic SimpleMQTT attempts to change the internal value from the specified message payload.
Topics that refer to `const`variables are never settable.
How the payload is interpreted depends on the data type of the topic and its format. Generally, a value received via MQTT is only applied if it fits into the underlying data type. Otherwise it is discarded and an optional error message is sent to the broker.

If the new value is different from the previous value the topic will be marked as changed. Its `hasBeenChanged()` function will return `true` in this case. Note that the default behavior is to remove the changed flag when `hasBeenChanged()` is called. If you don't want this you can use `hasBeenChanged(false)`.

Typically you would poll the topics of interest in the `loop()` function for changes. You can declare the topics as global variables while adding them:

	WiFiClient espClient;
	SimpleMQTTClient mqttClient(espClient, "simplemqtt", "test.mosquitto.org");
	bool my_bool;
	auto& myBool = mqttClient.add("my_bool", &my_bool);
	
	... setup code ...
	
	void loop() {
	  if (WiFi.status() == WL_CONNECTED) {
	    mqttClient.handle();
	  }

	  if (myBool.hasBeenChanged()) {
	    Serial.print("my_bool has been changed to: ");
	    Serial.println(my_bool ? "true" : "false");
	  }
	}


In most cases this is perfectly acceptable. If you want to avoid global topic variables, however, you can call the `getChange()` function to get any topic that has been changed. If no topic has been changed this function returns `nullptr`. You therefore have to work with a pointer here and use the arrow syntax `->`.

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

Another alternative is to use the `get()` method or the array index syntax `[]` like

	if (mqttClient.get(0)->hasBeenChanged()) ...
	if (mqttClient[0]->hasBeenChanged()) ...
	if (mqttClient.get("my_bool")->hasBeenChanged()) ...
	if (mqttClient["my_bool"]->hasBeenChanged()) ...

The above examples are all equivalent. It is not very safe to use these methods, however, as changes to your topic names or structure may cause the `get()` methods to return a `nullptr` which will crash your program.

If a topic is requestable SimpleMQTT by default listens to the `/get` subtopic of this topic (except for the topmost topic, i. e. the`mqttClient` itself). If a message for this topic has been received (regardless of its payload) the requested topic will be published again. If you request a group topic that has subtopics all its subtopics will be published again.

A topic that is auto-publishing will also be published again if a `/set` message is received regardless of whether the message changes the value of the topic or not.

You can customize the patterns for the request and set topics to match your specific requirements.

## Top-level topics 
A top-level topic is a topic whose name or any of its parents' names starts with a slash (`/`). They do not appear under the `mqttClient`'s root topic name and may refer to any other topic in the MQTT broker's topic tree. The leading slash is omitted when such a topic is published or subscribed to.

You would use these topics to listen to messages that are published by devices other than your own or your device's clients. This allows you to "eavesdrop" on these messages and reflect the state of other devices within variables of your own program.

Because these topics are "foreign" to your device they are by default not requestable and not auto-publishing. It makes no sense to request the state of another device from "your" device; also you don't want to interfere with or overwrite the state that is published by other devices. In fact setting such a topic to auto-publishing would lead to an infinite loop of receiving, publishing, receiving, and so on.

A top-level topic needs to be settable to be useful. However, it does not use SimpleMQTT's convention of the "set" message being a different topic (with `/set` appended to the topic's name). Instead the topic's name itself is used for the registration on the broker. SimpleMQTT detects this by checking whether the first character of the full topic name is a slash.

As an example, suppose you have a weather station that publishes the current outside temperature as the topic `weather/outside_temp` to your MQTT broker. Let's assume the value is a string that represents a float value with two decimals. To listen to messages that are published to this topic you would use:

	auto& outsideTemp = mqttClient.add("/weather").add<float>("outside_temp");

Note the leading slash in `/weather` that defines this group topic as a top-level topic.
SimpleMQTT will register with the broker to listen for messages to `weather/outside_temp`. If a message is received that changes the current value in your program you can detect this in `loop()` using:

	if (outsideTemp.hasBeenChanged()) {
		Serial.printf("Outside temp: %.2f\n", outsideTemp.value());
	}
A few caveats apply. You should never set such a topic to auto-publishing or call `republish()`  because this would result in an infinite loop of receiving/publishing. You can, however, set the value internally without negative consequences.
Also, you should always add top-level topics directly to the `mqttClient`.  While not strictly technically necessary this makes it easier for you to find them using the `get()` or `[]` functionality if necessary.


## Status message and debugging
To receive immediate feedback via MQTT about success or failure of the received messages you can use SimpleMQTT's built-in status message function. All you need to do is add a status topic to your `SimpleMQTTClient` instance:

	void setup() {
	  mqttClient.setStatusTopic(F("status"));
	  ...
	}
The status message is published in JSON format. It contains a `code` field of type `int8_t` and an optional `topic` field.
If code is greater or equal to zero, the status message will contain a `message` field, for example:

	{"code":0,"message":"OK","topic":"simplemqtt/testfloat/set"}
 If `code` is negative there will be an additional `error` field with details about the error, for example:

	{"code":-1,"error":"Invalid payload: x","topic":"simplemqtt/testfloat/set"}
	
Codes below 0 are reserved for SimpleMQTT's error messages. Codes above 0 can be freely assigned.

To enable debugging output on a `Serial`stream (or similar) you can use the following defines before including `SimpleMQTT.h`:

	#define  SIMPLEMQTT_DEBUG_SERIAL Serial
	#define  SIMPLEMQTT_ERROR_SERIAL Serial
	#define  SIMPLEMQTT_DEBUG_MEMORY  true
	#include  "SimpleMQTT.h"

`SIMPLEMQTT_DEBUG_SERIAL` will implicitly define `SIMPLEMQTT_ERROR_SERIAL` if you leave it undefined. `SIMPLEMQTT_DEBUG_MEMORY` reveals additional information about internal memory management and is mostly used for development purposes.

## Working with string topics
Strings can be constant or modifiable. They can have fixed or variable lengths. For fixed-length strings use `char*` arrays. For variable-length strings the `String` class is used. The amount of free RAM on your device limits the size of variable strings you can work with.

To add a string topic with a fixed length use this code:

	char* auto fixedLengthString = (char*)"Fixed length string";
	auto& fixedLengthStringValue = mqttClient.add("fixedLengthString", fixedLengthString, strlen(fixedLengthString));
Use only static `char*` array variables. If you use `char*` arrays declared in functions the array will be overwritten once the function has been left which will probably crash your program!
If a value is set via MQTT it is automatically truncated if it is too long for the array.

Constant strings cannot be changed:

	const char constantString[] = "Constant string";
	auto& constantStringValue = mqttClient.add("constantString", constantString);
or

	auto& constantStringValue2 = mqttClient.add("constantString2", "Constant string 2");

To use a variable-length string you have a number of options:

	auto& variableString = mqttClient.addString("variableString", String("variableString"));
This type of code uses an explicit default value to initialize the value on addition. You can also use:

	auto& variableString2 = mqttClient.addString("variableString2");
or

	auto& variableString2 = mqttClient.add<String>("variableString2");
to leave the topic initially empty.

To refer  to an existing `String` variable pass it when adding the topic:

	String variableString3("Variable String 3");
	auto& variableString3Value = mqttClient.add("variableString3", variableString3);
`const String` variables are not modifiable, such as:

	const String constantString4("Constant String 4");
	auto& constantString4Value = mqttClient.add("constantString4", constantString4);


## Value formatting
### `bool` values
`bool` values support different formats: "true"/"false", "yes"/"no", "on"/"off" and "1/0". There is a special format called `ANY` that uses "true"/"false" as output format but accepts any of the above values as input. Input values are parsed case-insensitively. The special input "toggle" sets the value of the `bool` to its inverse.
Usage:

	myBool.setFormat(BoolFormat::TRUEFALSE);
or

	myBool.setFormat(BoolFormat::YESNO);
or

	myBool.setFormat(BoolFormat::ONOFF);
or

	myBool.setFormat(BoolFormat::ONEZERO);
or

	myBool.setFormat(BoolFormat::ANY);

### Integral values
Integral values (`uint8_t`,  `int8_t`,  `uint16_t`,  `int16_t`,  `uint32_t`,  `int32_t`) may be formatted as decimal, hexadecimal or octal numbers. The 64 bit values do not support such formatting.

	myInt.setFormat(IntegralFormat::DECIMAL);
or

	myInt.setFormat(IntegralFormat::HEXADECIMAL);
or

	myInt.setFormat(IntegralFormat::OCTAL);

### Floating point values
Floating point values can be formatted using a format string specification such as commonly used with `Serial.printf()` or similar functions. For example, to specify five decimal digits you can use:

	myFloat.setFormat("%.5f");
The default number of decimals for floating point values is 2. This default is determined by the board libraries. To explicitly apply the default formatting use:

	myFloat.setFormat(nullptr);

If numeric values are being set via MQTT messages the payload is parsed according to the formatting specification of the topic. For floating point values this means that the received value is rounded such that the internal representation does not exceed the specified number of digits in the format string. The special values "nan", "-inf" and "inf" are also recognized.

### Default formats
To control the default formats for the different data types you can use the following global variables:

	namespace SimpleMQTT {
		static BoolFormat DEFAULT_BOOL_FORMAT = BoolFormat::ANY;
		static IntegralFormat DEFAULT_INTEGRAL_FORMAT = IntegralFormat::DECIMAL;
		static char* DEFAULT_FLOAT_FORMAT = nullptr;
		static char* DEFAULT_DOUBLE_FORMAT = nullptr;
	}
	
For example, to set the default format for all integral topics to hexadecimal in your sketch, use 

	DEFAULT_INTEGRAL_FORMAT = IntegralFormat::HEXADECIMAL;

in your `setup()` function (you don't have to use the namespace prefix `SimpleMQTT::` as this namespace will be automatically used if you include the library).

## Topic value validation
SimpleMQTT tries to ensure that values that don't fit into a topic's data type do not result in unspecified behavior. If a payload is invalid it is indicated via the status message (if `setStatusTopic()` has been used).

If you need custom code to validate the values that you want to accept for a given topic you can specify a payload handler to parse, check and provide feedback on the supplied payload. For example:

	auto& myInt = myGroup.add(F("my_int"), 10).setPayloadHandler([](auto& object, const char* payload) {
	  int newValue;
	  if (!object.parseValue(payload, &newValue))
		return ResultCode::INVALID_PAYLOAD;
	  if (newValue < 0 || newValue > 10) {
		object.getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object.getFullTopic(), 
		  "Only values between 0 and 10 allowed!");
		return ResultCode::INVALID_VALUE;
	  }
	  object.set(newValue, true);
	  return ResultCode::OK;
	});
This lambda validation function first uses the object's `parseValue()` function to get the integer value of the payload. This function already applies all the necessary format conversion and data type range checks. Note that this function may also fail, in which case the result code `INVALID_PAYLOAD` should be returned.
Next, the function checks whether the value is within the accepted range and sets the status message accordingly if this is not the case (this message is published to the MQTT broker if `setStatusTopic()` has been used). Otherwise the `set()` function is called which takes care of the internal topic flags.

## Arrays
SimpleMQTT can also work with arrays of the fundamental data types. To add an array topic you have to supply a pointer to the array and the array length to the add function:

	float floatArray[] = {1.02323f, 12.324524f, 545.132323f, 5465464.3434f, -1235354.3434f, 0660956.45435f};
	auto& floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float));

This topic will be published to the MQTT broker with the payload `1.02,12.32,545.13,5465464.50,-1235354.38,660956.44` (because the default number of decimals is 2). To specify a different format for an array topic's elements use the `setFormat()` function whose parameter type depends on the underlying data type:

	auto& floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float))
	  .setFormat("%.5f");

In this case the payload will be represented as `1.02323,12.32452,545.13232,5465464.50000,-1235354.37500,660956.43750`.

Values of arrays that are `const`  cannot be set. Non-const arrays can be set via MQTT messages by publishing a payload containing the new values to the `/set` subtopic of the array topic, with the following rules:

- The new array's values must be separated by a comma `,` in the payload
- Empty values do not modify the array element at the respective position
- Incomplete values (i .e. fewer elements than the length of the array) are allowed
- Surplus elements are ignored
- Any invalid value causes the whole message to be discarded

The individual array values can be custom-validated with a payload handler just like the example above, except that you use the arrays `element()` function to access the underlying object that is responsible for the conversion:

	floatArrayTopic.element().setPayloadHandler([](auto& object, const char* payload) {
	  float newValue;
	  if (!object.parseValue(payload, &newValue))
	    return ResultCode::INVALID_PAYLOAD;
	  if (newValue < -10000.0f || newValue > 10000.0f) {
	    object.getClient()->setStatus((int8_t)ResultCode::INVALID_VALUE, object.getFullTopic(), 
	      "Only values between -10000 and 10000 allowed!");
	    return ResultCode::INVALID_VALUE;
	  }
	  object.set(newValue, true);
	  return ResultCode::OK;
	});

Note that you should perform any `element()`  function calls in the `setup()` function (or similar) rather than during the addition of the topic if you want to have the array topic as a global variable, such as in

	auto& floatArrayTopic = mqttClient.add("float_array", floatArray, sizeof(floatArray) / sizeof(float));
because the `element()`  function returns a different object that does not represent the array topic but its internal conversion helper and is therefore not the object you would really want here. Also, avoid calling `setPayloadHandler()` on the array topic itself because in this case you will have to re-implement the array parse logic which is probably not what you want, either, except for very special cases.

## Memory management
Usually it is recommended to use dynamic memory management (except if you are really tight on DRAM, i. e. data RAM). If limited DRAM becomes a problem the first step is usually to move as many static `char` arrays as possible to the `PROGMEM`.  These string constants won't take any space in the DRAM; the drawback is a small runtime overhead needed for copying the string from flash memory to the DRAM (you also need to reserve some DRAM as a copy buffer). So, instead of using static char arrays like

	bool my_bool;
	auto& myBool = mqttClient.add("my_bool", &my_bool);
you can declare the topic name first as a flash memory string using the `Topic_P` macro:

	bool my_bool;
	Topic_P(myBoolTopic, "my_bool");
	auto& myBool = mqttClient.add(myBoolTopic, &my_bool);
This is the preferred way if you need the topic variable (`myBool` ) globally for later access.

If you add your topics during `setup()` time things become even more easy. You do not have to declare the topic name beforehand; instead you just use a flash memory string defined using the macro `F()`:

	auto& myLong = myGroup.add(F("my_long"), 200000L);
A SimpleMQTT topic uses roughly 40 bytes of RAM (depending on the data type and not including the topic name) plus 8 bytes of RAM for internal management. Usually this memory is allocated on demand on the heap using `new()`.  Alternatively you can specify a maximum static RAM size for SimpleMQTT to use:

	#define SIMPLEMQTT_STATIC_MEMORY_SIZE 2048
	#include "SimpleMQTT.h"
In this case SimpleMQTT will use a maximum of about 2048 bytes of RAM for topics and internal management structures. This case is most useful if you add topics dynamically. If a topic that is to be added does not fit into the specified RAM amount any more it is discarded along with its management structure. To avoid a crash the returned pointer is not invalid, though; instead it points to a dummy object that is kept for this purpose. If you run into memory issues you may try this method but be aware that you may lose the ability to handle certain topics and the crash safeguard may not work in all cases, either.
Removing of once-added topics is not possible. For most IoT use cases it should be unnecessary. If you need to change your topic structure depending on device configuration you should perform a reboot and do so during initial setup. This allows the MQTT broker to clean up the session during disconnect as well.

## Topic inspection and access
To inspect the topic hierarchy in your SimpleMQTT client instance you can use the `printTo()`  function that takes a `Stream&`, for example, `Serial`, as parameter. A topic group has a `size()` function that returns the amount of its child topics. Individual topics can be accessed using the index operator with numeric index:

	mqttClient[0].printTo(Serial);
which is equivalent to calling the `get()` function:

	mqttClient.get(0).printTo(Serial);
Or you can specify a topic name or path:

	mqttClient["my_group/my_int"].printTo(Serial);
or

	mqttClient.get("my_group/my_int").printTo(Serial);
Note that these functions may return `nullptr` if  the specified topic is not found. An unguarded call to any function on a null object will crash your program so be careful if you use them.
