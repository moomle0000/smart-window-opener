#include <dht.h>
#include "handler/setwifi.h"
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include "SinricProTemperaturesensor.h"
#include "handler/Temperaturerang.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#define APP_KEY "0c94c439-5c73-45d2-bcdd-0cb3f9650e9d"
#define APP_SECRET "c7c2b17f-b3a8-4364-89a2-fd88dbe5c871-b58304a1-eab8-4d9a-b365-0d07d8bd1e1d"
#define TEMP_SENSOR_ID "656259c2a3c6b579a1afe4cc"

#define windowmode "65625fc2031135be133ebc28"
#define windowmoderamg "65635fbf031135be133f393b"
#define BAUD_RATE 115200     // Change baudrate to your need (used for serial monitor)
#define EVENT_WAIT_TIME 3000 // send event every 60 seconds

#define DHT_SENSOR_PIN 21     // ESP32 pin GPIO23 connected to DHT11
#define DHT_SENSOR_TYPE DHT11 // DHT11 sensor type

bool isSwitchOn = false;
// Create an instance of the dht sensor
Temperaturerang &temperaturerang = SinricPro[windowmoderamg];
// dht dht(dht_SENSOR_PIN, dht_SENSOR_TYPE);

#define TEMP_SENSOR_ID "656259c2a3c6b579a1afe4cc"
#define BAUD_RATE 115200      // Change baudrate to your need (used for serial monitor)
#define EVENT_WAIT_TIME 20000 // send event every 60 seconds
int lastloop = 9;

bool deviceIsOn;                              // Temeprature sensor on/off state
float temperature;                            // actual temperature
float humidity;                               // actual humidity
float lastTemperature;                        // last known temperature (for compare)
float lastHumidity;                           // last known humidity (for compare)
unsigned long lastEvent = (-EVENT_WAIT_TIME); // last time event has been sent                          // Temeprature sensor on/off state

int temperatureset;

bool controaltime = false;

bool globalPowerState;
std::map<String, int> globalRangeValues;

#define BAUD_RATE 115200 // Change baudrate to your need (used for serial monitor)
#define WIFI_SSID "TP-LINK_Extender_2.4GHz"
#define WIFI_PASS "jf981103"

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

int enA = 21; // for speed control motor low
int in1 = 2;  // for direction control motor low
int in2 = 4;  // for direction control motor low
// Motor B Left/Right connections
int enB = 15; // for speed control motor high
int in3 = 18; // for direction control motor High    USE THIS
int in4 = 19; // for direction control motor     USE THIS
// main setup function
// It initializes the serial communication, sets the pin modes for various pins, sets up the WiFi connection, sets up a SinricPro connection, and begins a server.

WebServer server(80);
#define HOSTNAME "SinricPro"

float getTemperature()
{
  // Construct the full URL with the API key
  String fullUrl = String('https://api.openweathermap.org/data/2.5/weather?lat=26.167432&lon=50.627536&appid=') + '2e52efe9e6a0612cebc1f4c5ed270fa6';

  // Create an HTTP object
  HTTPClient http;
  http.begin(fullUrl);

  // Get the HTTP response
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      // Parse JSON response
      String payload = http.getString();
      // Parse JSON data using ArduinoJson library
      DynamicJsonDocument jsonDoc(1024); // Adjust the size based on your JSON response
      deserializeJson(jsonDoc, payload);
      JsonObject main = jsonDoc["main"];
      float temperaturecontry = main["temp"];

      return temperaturecontry - 273.15;
    }
    else
    {
      Serial.print("HTTP request failed with error code: ");
      Serial.println(httpCode);
    }
  }
  else
  {
    Serial.println("HTTP request failed");
  }

  // End the HTTP request
  http.end();
}

bool PowerState(String deviceId, bool &state)
{
  if (deviceId == windowmode)
  {
    Serial.println('Window mode turned ' + state ? "on" : "off");
    if (isSwitchOn)
    {
      Serial.printf("Window mode turned OFF\n");
      isSwitchOn = false;
      controaltime = true;
    }
    else
    {
      Serial.printf("Window mode turned ON\n");
      isSwitchOn = true;
      controaltime = true;
    }
  }
  if (deviceId == TEMP_SENSOR_ID)
  {
    Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state ? "on" : "off");
    deviceIsOn = state; // turn on / off temperature sensor
  }

  if (deviceId == windowmoderamg)
  {
    Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
    globalPowerState = state;
    isSwitchOn = state;
    return true; // request handled properly
  }
  return true;
}

// PowerStateController
void updatePowerState(bool state)
{
  temperaturerang.sendPowerStateEvent(state);
}

// RangeController
void updateRangeValue(String instance, int value)
{
  temperaturerang.sendRangeValueEvent(instance, value);
}
// RangeController
bool onRangeValue(const String &deviceId, const String &instance, int &rangeValue)
{
  Serial.printf("[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
  globalRangeValues[instance] = rangeValue;
  temperatureset = rangeValue;
  return true;
}

bool onAdjustRangeValue(const String &deviceId, const String &instance, int &valueDelta)
{
  globalRangeValues[instance] += valueDelta;
  Serial.printf("[Device: %s]: Value for \"%s\" changed about %d to %d\r\n", deviceId.c_str(), instance.c_str(), valueDelta, globalRangeValues[instance]);
  globalRangeValues[instance] = valueDelta;
  return true;
}

void setupSinricPro()
{
  // PowerStateController
  temperaturerang.onPowerState(PowerState);
  // RangeController
  temperaturerang.onRangeValue("temperaturerange", onRangeValue);
  temperaturerang.onAdjustRangeValue("temperaturerange", onAdjustRangeValue);
  // add device to SinricPro
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  mySensor.onPowerState(PowerState);
  SinricProSwitch &mySwitch = SinricPro[windowmode];
  mySwitch.onPowerState(PowerState);
  // setup SinricPro
  SinricPro.onConnected([]()
                        { Serial.printf("Connected to SinricPro\r\n"); });
  SinricPro.onDisconnected([]()
                           { Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void handleTemperaturesensor()
{
  unsigned long actualMillis = millis();
  if (actualMillis - lastEvent < EVENT_WAIT_TIME)
    return; // only check every EVENT_WAIT_TIME milliseconds

  temperature = dht.readTemperature(); // get actual temperature in °C
                                       //  temperature = dht.getTemperature() * 1.8f + 32;  // get actual temperature in °F
  humidity = dht.readHumidity();       // get actual humidity

  // if (isnan(temperature) || isnan(humidity))
  // {                                           // reading failed...
  //   Serial.printf("DHT reading failed!\r\n"); // print error message
  //   return;                                   // try again next time
  // }

  // Serial.println("Temperature: " + String(temperature) + "°C Humidity: " + String(humidity) + "%");

  if (temperature == lastTemperature || humidity == lastHumidity)
    return; // if no values changed do nothing...

  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];    // get temperaturesensor device
  bool success = mySensor.sendTemperatureEvent(temperature, humidity); // send event
  // if (success)
  // { // if event was sent successfuly, print temperature and humidity to serial
  //   Serial.printf("Temperature: %2.1f Celsius\tHumidity: %2.1f%%\r\n", temperature, humidity);
  // }
  // else
  // { // if sending event failed, print error message
  //   Serial.printf("Something went wrong...could not send Event to server!\r\n");
  // }

  lastTemperature = temperature; // save actual temperature for next compare
  lastHumidity = humidity;       // save actual humidity for next compare
  lastEvent = actualMillis;      // save actual time for next compare
}

void stopMotor()
{
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
void openwindow(int time)
{
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  // delay(time);
  // stopMotor();
}

void closewindow(int time)
{
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  // delay(time);
  // stopMotor();
}

void functionopenwindow(float value)
{
  int intNumber = static_cast<int>(value);

  openwindow(intNumber);
  delay(intNumber);
  stopMotor();
}

void functionclosewindow(float value)
{
  int intNumber = static_cast<int>(value);
  closewindow(intNumber);
  delay(intNumber);
  stopMotor();
}

bool windowmoded = true;

// Flag to track motor movement
bool motorMoved = false;

// Previous temperature reading
float prevTemperature = 0.0;
void controalWindow()
{
  if (isSwitchOn)
  {
    float floatValue = static_cast<float>(temperatureset);

    // Check if the temperature has changed
    if (temperature != prevTemperature)
    {
      // If the temperature is greater than the threshold, move the motor forward
      if (temperature > floatValue)
      {
        Serial.println("Moving forward");
        openwindow(6000);

        delay(6000);

        // Stop the motor
        Serial.println("Stopping");
        stopMotor();

        // Set the flag to indicate that the motor has moved
        motorMoved = true;
      }
      // If the temperature is smaller than the threshold, move the motor backward
      else
      {
        Serial.println("Moving backward");
        closewindow(6000);
        delay(6000);

        // Stop the motor
        Serial.println("Stopping");
        stopMotor();

        // Reset the flag
        motorMoved = false;
      }

      // Update the previous temperature reading
      prevTemperature = dht.readTemperature();
    }

    delay(1000);
  }
  else
  {
    stopMotor();
  }
}

void sendstautus()
{
  // Create a JSON document
  StaticJsonDocument<200> doc;

  // Add data to the JSON document
  doc["sensor"] = "temperature";
  doc["value"] = dht.readTemperature();
  doc['windowmode'] = isSwitchOn;

  // Convert the JSON document to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send HTTP response with JSON data

  server.send(200, "application/json", jsonString);
}

void handleRoot()
{
  server.send(200, "text/html",
              "<html>"
              "<head><title>" HOSTNAME " Demo </title>"
              "<meta http-equiv=\"Content-Type\" "
              "content=\"text/html;charset=utf-8\">"
              "</head>"
              "<body>"
              "<h1>Control All IR Devices with " HOSTNAME ".</h1>"
              "<h2>RGB Board</h2>"
              "<p><a href=\"lm?code=16236607\"> RGB ON</a></p>"
              "<p><a href=\"lm?code=16203967\"> OFF</a></p>"
              "</body>"
              "</html>");
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

void requestHandlerarq()
{
  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "code" || server.argName(i) == "value" || server.argName(i) == "on")
    {
      String divceid = server.arg(0);
      bool on = server.arg(1) ? true : false;
      float value = atof(server.arg(2).c_str());

      StaticJsonDocument<200> doc;

      // Add data to the JSON document
      doc["divceid"] = divceid;
      doc["on"] = on;
      doc["value"] = value;

      // Convert the JSON document to a string
      String jsonString;
      serializeJson(doc, jsonString);

      // Send HTTP response with JSON data

      server.send(200, "application/json", jsonString);

      if (divceid == windowmode)
      {
        isSwitchOn = on;
        temperatureset = value;
        controaltime = true;
      }

      if (divceid == "open")
      {
        /* code */
        functionopenwindow(value);
        Serial.println(value);
        divceid = "";
      }

      if (divceid == "close")
      {
        /* code */
        functionclosewindow(value);
        Serial.println(value);
        divceid = "";
      }
    }
  }
  // handleRoot();
}

void beginServer()
{
  server.enableCORS();
  server.on("/", handleRoot);
  server.on("/device", requestHandlerarq);
  server.on("/info", sendstautus);
  server.begin();
  Serial.println("HTTP server started");
}

bool isopen = false;
bool isclose = false;
void windowmodefuction()
{

  if (isSwitchOn && dht.readTemperature() != prevTemperature)
  {
    float floatValue = static_cast<float>(temperatureset);
    if (dht.readTemperature() > floatValue && !isopen)
    {
      functionopenwindow(14400.0);
      isopen = true;
      isclose = false;
      delay(15000);
    }

    if (dht.readTemperature() < floatValue && !isclose)
    {
      functionclosewindow(14400.0);
      isclose = true;
      isopen = false;
      delay(15000);
    }
    prevTemperature = dht.readTemperature();
  }
}

void setup()
{
  Serial.begin(BAUD_RATE);
  Serial.printf("\r\n\r\n");
  dht.begin();
  // pinMode(23, OUTPUT); // red
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  setupWiFi(WIFI_SSID, WIFI_PASS);
  setupSinricPro();
  beginServer();
}

void loop()
{

  server.handleClient();
  SinricPro.handle();
  windowmodefuction();
  // controalWindow();

  handleTemperaturesensor();
}