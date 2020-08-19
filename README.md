# enee101-esp

Modified version of example sketch demonstrating ability to send and receive coordinate data from Azure IoT hub. From https://docs.microsoft.com/en-us/samples/azure-samples/esp32-iot-devkit-get-started/sample/

Program accepts inputs as direct methods from Azure IoT hub to move stepper motors and sends ultrasonic sensor distance data to hub. 

Designed for the ENEE101 Covid19 Response Project. 

## Setup

### In Platformio

1. Clone the entire repository into a folder 
2. Open project using Platformio using any editor
3. Build project
4. Download any missing libraries using "pio lib install <>", replacing <> with the name of any library missing
5. Go to config setup

### In Arduino IDE

1. Download main.cpp and rename to main.ino
2. Copy the folders "ArduinoJson", "ArrayQueue", "ESP32 Azure IoT Arduino" and "FreeRTOS" from ./lib/ into Arduino/libraries folder 
3. Replace \#include "Config.h" with #include "../lib/Config/src/Config.h" in main.ino
4. Download any missing libraries using the Arduino Library Manager
5. Go to config setup

### Config Setup
These steps assume that there is a functional Azure IoT Hub and device already set up. Instructions to do so can be found at https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal

1. Go to Azure IoT Hub and go to the "IoT Devices" tab
2. Click on the device name that you wish to assign
3. Copy the primary connection string or secondary connection string into the CONFIG_CONNECTION_STRING define in Config.h
4. Add WiFi connection details to CONFIG_WIFI_NAME and CONFIG_WIFI_PASSWORD

## Direct Methods

### Start

Causes device to resume sending telemetry messages

#### Payload

None

#### Example Message

```
{
    "methodName": "start",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```

### Stop

Causes device to halt sending telemetry messages

#### Payload

None

#### Example Message

```
{
    "methodName": "start",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```

### LED

Toggles the state of the on-board LED assigned to pin 2

#### Payload

None

#### Example Message

```
{
    "methodName": "led",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```

### Interval

Changes the delay between telemetry messages in milliseconds

#### Payload

Positive integer value associated with "interval" to be used as new delay

#### Example Message

```
{
    "methodName": "interval",
    "responseTimeoutInSeconds": 200,
    "payload" : {"interval": 2000}
}
```

### Move

Causes servo motors to move a set amount of steps in either the positive or negative direction

#### Payload

Signed integer value associated with "x" and "y" to be used as the amount of steps moved in both directions

#### Example Message

```
{
    "methodName": "move",
    "responseTimeoutInSeconds": 200,
    "payload" : {
        "x": 400,
        "y": -6000
    }
}
```

### Array Move

Causes servo motors to execute several move commands in order

#### Payload

Array of signed integer values in sequential order associated with "x" and "y" to be used as the amount of steps moved in both directions; both arrays must be the same length

#### Example Message

```
{
    "methodName": "moveArray",
    "responseTimeoutInSeconds": 200,
    "payload" : {
        "x": [500, -120, 40000, 0, 12300],
        "y": [-500, 0, 5000, -950, 10000]
    }
}
```

### Reset

Moves servo motors back to starting position, set when program begins operation

#### Payload

None

#### Example Message

```
{
    "methodName": "reset",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```

### Clear Motor Queue

Removes all queued move and reset commands from device's internal queue

#### Payload

None

#### Example Message

```
{
    "methodName": "clear",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```

### Clear Motor Queue and Reset

Removes all queued move and reset commands from device's internal queue and then queues a reset instruction

#### Payload

None

#### Example Message

```
{
    "methodName": "clearReset",
    "responseTimeoutInSeconds": 200,
    "payload" : {}
}
```
