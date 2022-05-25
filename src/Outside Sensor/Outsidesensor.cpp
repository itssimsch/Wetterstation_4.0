// BME
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// GY-302
#include <BH1750.h>

// ElegentOTA
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "ConnectionHandler.h"

// Config files
#include "config_global.h"
#include "config_outsidesensor.h"

Adafruit_BME280 bme;
BH1750 GY302;
double tempOutside, humidityOutside, absHumdityOutside, pressureOutside;
double tempOffset, humidityOffset, pressureOffset = 0.0;
uint16_t lux;

AsyncWebServer server(80);

WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
Ticker ledFlasher;
SimpleTimer timer;
bool wifiConnected = false;
int check_connections_timer_id = 0;
int long_delay_timout_id = 0;
int heartbeat_timer_id = 0;

double calculateAbsoluteHumidity(double temp, double rel_hum)
{
  // Source: https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
  return (6.112 * (pow(M_E, ((17.67 * temp) / (temp + 243.5))) * rel_hum * 2.1674)) / (273.15 + temp);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0';
  if (String(topic) == "Outsidesensor/Control/TemperatureOffset")
    tempOffset = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Control/HumidityOffset")
    humidityOffset = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Control/PressureOffset")
    pressureOffset = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Control/Rebooting" && String((char *)payload) == "true")
  {
    MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Status", "Dead", true);
    ESP.restart();
  }
  if (String(topic) == "Outsidesensor/Control/Fanstate")
  {
    if (String((char *)payload) == "true")
      digitalWrite(FANPIN, HIGH);
    else
      digitalWrite(FANPIN, LOW);
  }
}

void timerEvent()
{
  heartbeat();

  lux = GY302.readLightLevel();
  tempOutside = bme.readTemperature() + tempOffset;
  humidityOutside = bme.readHumidity() + humidityOffset;
  pressureOutside = bme.readPressure() / 100.0 + pressureOffset;
  absHumdityOutside = calculateAbsoluteHumidity(tempOutside, humidityOutside);

  if (MQTTclient.connected())
  {
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Temperature", String(tempOutside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Humidity", String(humidityOutside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Pressure", String(pressureOutside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/AbsoluteHumidity", String(absHumdityOutside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Lux", String(lux).c_str(), true);
  }
}

void initWebserver()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->redirect(NODE_RED_DB_URL); });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
            {request->send(200, "text/plain", "Rebooting!"); ESP.restart(); });

  AsyncElegantOTA.begin(&server);
  server.begin();
}

void setup()
{
  Serial.begin(9600);

  bme.begin(0x76);
  GY302.begin();

  timer.setInterval(3600000, []() {});
  heartbeat_timer_id = timer.setInterval(HEARTBEAT_DELAY, timerEvent);
  check_connections_timer_id = timer.setInterval(CHECK_CONNECT_DELAY, verifyConnections);

  pinMode(LEDREDPIN, OUTPUT);
  pinMode(LEDGREENPIN, OUTPUT);
  pinMode(LEDBLUEPIN, OUTPUT);
  pinMode(FANPIN, OUTPUT);

  handleConnects();
  MQTTclient.setCallback(callback);
  MQTTclient.subscribe("Outsidesensor/Control/TemperatureOffset");
  MQTTclient.subscribe("Outsidesensor/Control/HumidityOffset");
  MQTTclient.subscribe("Outsidesensor/Control/PressureOffset");
  MQTTclient.subscribe("Outsidesensor/Control/Rebooting");
  MQTTclient.subscribe("Outsidesensor/Control/Fanstate");
  initWebserver();
}

void loop()
{
  timer.run();
  if (wifiConnected)
  {
    MQTTclient.loop();
    AsyncElegantOTA.loop();
  }
}