#include <Arduino.h>
#include <WiFi.h>
#include "ArrayQueue.h"
#include "ArduinoJson.h"
//#include "FreeRTOS.h"
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"
#include "Config.h"

#define POT_MIN 20000 //in ohms

#define PWM_PIN 4

#define RELAY_0 12//pot or r/r circuit

#define RELAY_1 25 //relay control pins
#define RELAY_2 26 
#define RELAY_3 27
#define RELAY_4 32
#define RELAY_5 33

#define VOLTAGE_PIN 34//

#define MESSAGE_MAX_LEN 16384 // size of message buffers
#define COMMAND_BUFFER_LEN 256  // local command queue max length

#define WIFI_TIMEOUT_MS 10000 // timeout for connecting to wifi network

#define ONBOARD_LED_PIN 2

const int freq = 10000;
const int ledChannel = 0;
const int resolution = 8;

// TODO look into wifimanager library or similar solutions to prevent wifi settings from being stored in plaintext
const char *ssid = CONFIG_WIFI_NAME;
const char *password = CONFIG_WIFI_PASSWORD;

// string containing HostName, Device Id & Device Key
static const char *connectionString = CONFIG_CONNECTION_STRING;

// telemetry message
const char *messageData = "{\"messageId\":%d, \"voltage\":%lf}";
const char *responseMessageData = "{\"status\": %d}";

static int messageCount = 1;
static long interval = 2000; //ms between telemetry messages
static bool messageSending = true;
static uint64_t send_interval_ms;
static bool ledValue = false;

// task handlers
TaskHandle_t commsTaskHandler;

static void bright(double brightness);
static void res(double res); 

/* //////////////// Azure Utilities //////////////// */

static void sendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println(F("Info: Send Confirmation Callback finished"));
  }
}

static void messageCallback(const char *payload, int size)
{
  Serial.print(F("Info: Message callback:"));
  Serial.println(payload);
}

static void deviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payload, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

static int deviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.print("Info: Trying to invoke method ");
  Serial.println(methodName);

  char responseMessage[MESSAGE_MAX_LEN];

  int result = 200;

  StaticJsonDocument<MESSAGE_MAX_LEN> doc;
  deserializeJson(doc, payload, MESSAGE_MAX_LEN);
  Serial.print(F("Info: Received payload: "));
  serializeJson(doc, Serial);
  Serial.println();

  if (strcmp(methodName, "start") == 0)
  {
    messageSending = true;
    Serial.println(F("Info: Started sending telemetry messages"));
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    messageSending = false;
    Serial.println(F("Info: Stopped sending telemetry messages"));
  }
  else if (strcmp(methodName, "update") == 0)
  {
    ledValue = !ledValue;
    digitalWrite(ONBOARD_LED_PIN, ledValue);
    Serial.println(F("Info: Toggled on-board LED"));
  }
  else if (strcmp(methodName, "pwm") == 0)
  {
    
  }
  else if (strcmp(methodName, "interval") == 0)
  {
    JsonVariant newInterval = doc["interval"];
    interval = newInterval.as<int>();
    Serial.print(F("Info: Changed telemetry interval to "));
    Serial.print(interval);
    Serial.println(F("ms"));
  }
  else if (strcmp(methodName, "data") == 0)
  {
    JsonVariant newInterval = doc["resistance"];
    double resistance = newInterval.as<double>();
    newInterval = doc["brightness"];
    double brightness = newInterval.as<double>();

    res(resistance);
    bright(brightness);
    
  }
  else
  {
    result = 404;
    Serial.println(F("Warning: Received invalid command"));
  }

  snprintf(responseMessage, MESSAGE_MAX_LEN, responseMessageData, result);

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}

//INIT WIFI
static bool initWifi(int timeoutInMs)
{
  Serial.print(F("Info: Attempting to connect to "));
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int endTime = millis() + timeoutInMs;

  do
  {
    //delay(500);
    //Serial.print(".");
    delay(50);
    if (millis() > endTime) 
    {
      Serial.println(F("Error: Could not connect to WiFi"));
      return false;
    }
  } while (WiFi.status() != WL_CONNECTED);

  Serial.print(F("Info: WiFi connected. Local IP Address: "));
  Serial.println(WiFi.localIP());
  return true;
}

// methods

void bright(double brightness) {
  ledcWrite(ledChannel, brightness);
}

void res(double res) {
    
    if(res < POT_MIN) {
      //use resisitor circuit
      digitalWrite(RELAY_0, HIGH);

      //2.1 ohms
      if(res == 4){
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, HIGH);
      }

      //2.7 ohms
      if(res == 5){
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, LOW);
      }

      //3.3 ohms
      if(res == 8){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, LOW);
      }

      //5.8 ohms
      if(res == 9){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, LOW);
      }

      //6.6 ohms
      if(res == 10){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, LOW);
      }

      //8.2 ohms
      if(res == 11){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, HIGH);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, HIGH);
      }

      //10 ohms
      if(res == 15){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, HIGH);
      }

      //14 ohms
      if(res == 16){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, LOW);
      }

      //19 ohms
      if(res == 20){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, HIGH);
      }

      //22 ohms
      if(res == 23){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, HIGH);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, LOW);
      }

      //35 ohms
      if(res == 36){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, HIGH);
      }

      //46 ohms
      if(res == 47){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, HIGH);
        digitalWrite(RELAY_5, LOW);
      }

      //145 ohms
      if(res == 144){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, HIGH);
      }

      //4000 ohms
      if(res == 4000){
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        digitalWrite(RELAY_3, LOW);
        digitalWrite(RELAY_4, LOW);
        digitalWrite(RELAY_5, LOW);
      }
    }
    else {
      //use pot
    }

    Serial.print(res);
}

// COMMS TASK

static void commsTask(void *pvParameters)
{
  Serial.print(F("Info: Starting messaging task on core "));
  Serial.println(xPortGetCoreID());

  // initialize wifi module
  bool hasWifi = false;
  do
  {
    hasWifi = initWifi(WIFI_TIMEOUT_MS);
  } while (!hasWifi);

  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "ENEE101-ESP32");
  Esp32MQTTClient_Init((const uint8_t *) connectionString, true);

  Esp32MQTTClient_SetSendConfirmationCallback(sendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(messageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(deviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(deviceMethodCallback);

  send_interval_ms = millis();

  while (true)
  {
    if (hasWifi)
    {
      if (messageSending && (int)(millis() - send_interval_ms) >= interval)
      {
        send_interval_ms = millis();
        char messagePayload[MESSAGE_MAX_LEN];

        // suspend task scheduler to ensure specific timing of ultrasonic sensors is met
        vTaskSuspendAll();
        float voltage = (analogRead(VOLTAGE_PIN)/4) + 50;
        xTaskResumeAll();

        // copy into message
        snprintf(messagePayload, MESSAGE_MAX_LEN, messageData, messageCount++, voltage);

        Serial.print(F("Info: Sending message to server "));
        Serial.println(messagePayload);

        EVENT_INSTANCE *message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
        Esp32MQTTClient_SendEventInstance(message);
      }

    }
    else 
    {
      // try to connect if connection is lost
      hasWifi = initWifi(WIFI_TIMEOUT_MS);
    }

    // check for new messages 
    if (WiFi.status() == WL_CONNECTED) 
    {
      Esp32MQTTClient_Check();
    }
    else
    {
      hasWifi = false;
    }
    
    vTaskDelay(100);
  }

}

/* //////////////// Arduino Sketch //////////////// */

void setup()
{
  pinMode(RELAY_0, OUTPUT);
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);
  pinMode(RELAY_5, OUTPUT);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(PWM_PIN, ledChannel);
  ledcWrite(ledChannel, 256);

  pinMode(VOLTAGE_PIN, INPUT);
  analogSetAttenuation(ADC_0db);

  digitalWrite(RELAY_0, LOW);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  digitalWrite(RELAY_3, LOW);
  digitalWrite(RELAY_4, LOW);
  digitalWrite(RELAY_5, LOW);

  digitalWrite(ONBOARD_LED_PIN, ledValue);

  Serial.begin(115200);
  Serial.println(F("ESP32 Device"));
  Serial.println(F("Initializing..."));

  xTaskCreatePinnedToCore(
    commsTask,
    "Comms Task",
    65536,
    NULL,
    1,
    &commsTaskHandler,
    0);

  delay(200);

}

void loop()
{
 vTaskSuspend(NULL);
}