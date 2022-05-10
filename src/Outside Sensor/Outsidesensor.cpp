// BME
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "ConnectionHandler.h"

// Config files
#include "config_global.h"
#include "config_outsidesensor.h"

Adafruit_BME280 bme;
double tempInside, humInside, pressure;

WiFiClient My_WiFi_Client;
PubSubClient MQTTclient(My_WiFi_Client);
Ticker ledFlasher;  
SimpleTimer timer;
bool wifiConnected = false;
int check_connections_timer_id = 0;
int long_delay_timout_id = 0;
int heartbeat_timer_id = 0;

void setup()
{
    Serial.begin(9600);
    

    timer.setInterval(3600000, [] () {});                                                           
    heartbeat_timer_id = timer.setInterval(HEARTBEAT_DELAY, heartbeat);
    check_connections_timer_id = timer.setInterval(CHECK_CONNECT_DELAY, verifyConnections);

    pinMode(LEDREDPIN,OUTPUT);
    pinMode(LEDGREENPIN,OUTPUT);
    pinMode(LEDBLUEPIN,OUTPUT);

    handleConnects();
}

void loop()
{
  timer.run();
  
  if (!wifiConnected)
  {
    MQTTclient.loop();
  }
}