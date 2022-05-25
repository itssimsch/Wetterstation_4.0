// BME
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// OLED
#include <U8g2lib.h>
#define INSIDE 0
#define OUTSIDE 1

// ElegentOTA
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "ConnectionHandler.h"

// Loaud Config files
#include "config_global.h"
#include "config_insidesensor.h"

Adafruit_BME280 bme;
double tempInside, humidityInside, pressureInside, absHumdityInside, tempOutside, humidityOutside, pressureOutside, absHumdityOutside;
double tempOffset, humidityOffset, pressureOffset = 0.0;

AsyncWebServer server(80);

// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// bool displayFlip = false;


WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
Ticker ledFlasher;
SimpleTimer timer;
bool wifiConnected = false;
int check_connections_timer_id = 0;
int long_delay_timout_id = 0;
int heartbeat_timer_id = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
void printRegular(const char *s, int y, boolean erase)
{

  if (erase)
  {
    u8g2.clearBuffer(); // clear the internal memory
  }
  u8g2.setFont(u8g2_font_8x13_mf);
  u8g2.drawStr(0, y, s); // write something to the internal memory
  u8g2.sendBuffer();     // transfer internal memory to the display
}

void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  switch (symbol)
  {
  case INSIDE:
    u8g2.setFont(u8g2_font_open_iconic_gui_4x_t);
    u8g2.drawGlyph(x, y, 64);
    break;
  case OUTSIDE:
    u8g2.setFont(u8g2_font_open_iconic_gui_4x_t);
    u8g2.drawGlyph(x, y, 65);
    break;
  }
}

void drawWeather(uint8_t symbol, float degree)
{
  drawWeatherSymbol(0, 35, symbol);
  u8g2.setFont(u8g2_font_logisoso22_tf);
  u8g2.setCursor(35 + 3, 30);
  u8g2.print(degree);
  u8g2.print("°C"); // requires enableUTF8Print()
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

double calculateAbsoluteHumidity(double temp, double rel_hum)
{
  // Source: https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
  return (6.112 * (pow(M_E, ((17.67 * temp) / (temp + 243.5))) * rel_hum * 2.1674)) / (273.15 + temp);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0';
  if (String(topic) == "Outsidesensor/Data/Temperature")
    tempOutside = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Data/Humidity")
    humidityOutside = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Data/Pressure")
    pressureOutside = atof((char *)payload);
  if (String(topic) == "Outsidesensor/Data/AbsoluteHumidity")
    absHumdityOutside = atof((char *)payload);
  if (String(topic) == "Insidesensor/Control/TemperatureOffset")
    tempOffset = atof((char *)payload);
  if (String(topic) == "Insidesensor/Control/HumidityOffset")
    humidityOffset = atof((char *)payload);
  if (String(topic) == "Insidesensor/Control/PressureOffset")
    pressureOffset = atof((char *)payload);
  if (String(topic) == "Insidesensor/Control/Rebooting" && String((char *)payload) == "true")
  {
    MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Status", "Dead", true);
    ESP.restart();
  }
}

void timerEvent()
{
  heartbeat();
  tempInside = bme.readTemperature() + tempOffset;
  humidityInside = bme.readHumidity() + humidityOffset;
  pressureInside = bme.readPressure() / 100.0 + pressureOffset;
  absHumdityInside = calculateAbsoluteHumidity(tempInside, humidityInside);

  if (MQTTclient.connected())
  {
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Temperature", String(tempInside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Humidity", String(humidityInside).c_str(), true);
    MQTTclient.loop();
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/Pressure", String(pressureInside).c_str(), true);
    MQTTclient.publish(BASE_MQTT_TOPIC "/Data/AbsoluteHumidity", String(absHumdityInside).c_str(), true);
  }
  /*
  if (displayFlip)
  {
    String insideMessage1 = "LF: " + String(humidityInside) + "%/" + String(absHumdityInside) + "g";
    String insideMessage2 = "LD: " + String(pressureInside) + "hPa";
    u8g2.clearBuffer();              // clear the internal memory
    drawWeather(INSIDE, tempInside); // draw the icon and degree only once
    printRegular(insideMessage2.c_str(), 50, false);
    printRegular(insideMessage1.c_str(), 62, false);
  }
  else
  {
    String outsideMessage1 = "LF: " + String(humidityOutside) + "%/" + String(absHumdityOutside) + "g";
    String outsideMessage2 = "LD: " + String(pressureOutside) + "hPa";
    u8g2.clearBuffer();                // clear the internal memory
    drawWeather(OUTSIDE, tempOutside); // draw the icon and degree only once
    printRegular(outsideMessage1.c_str(), 50, false);
    printRegular(outsideMessage2.c_str(), 62, false);
  }
  displayFlip = !displayFlip;*/
}

void initWebserver()
{
  // printRegular("Init Webserver", 10, true);
  // printRegular(WiFi.localIP().toString().c_str(), 22, false);
  // printRegular(SSID, 34, false);
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
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);
  bme.begin(0x76);

  /* // Init OLED
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_8x13_mf);*/

  timer.setInterval(3600000, []() {});
  heartbeat_timer_id = timer.setInterval(HEARTBEAT_DELAY, timerEvent);
  check_connections_timer_id = timer.setInterval(CHECK_CONNECT_DELAY, verifyConnections);

  pinMode(LEDREDPIN, OUTPUT);
  pinMode(LEDGREENPIN, OUTPUT);
  pinMode(LEDBLUEPIN, OUTPUT);

  // printRegular("Connecting...", 10, true);

  handleConnects();
  MQTTclient.setCallback(callback);
  MQTTclient.subscribe("Outsidesensor/Data/Temperature");
  MQTTclient.subscribe("Outsidesensor/Data/Humidity");
  MQTTclient.subscribe("Outsidesensor/Data/Pressure");
  MQTTclient.subscribe("Outsidesensor/Data/AbsoluteHumidity");
  MQTTclient.loop();
  MQTTclient.subscribe("Insidesensor/Control/TemperatureOffset");
  MQTTclient.subscribe("Insidesensor/Control/HumidityOffset");
  MQTTclient.subscribe("Insidesensor/Control/PressureOffset");
  MQTTclient.subscribe("Insidesensor/Control/Rebooting");
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
