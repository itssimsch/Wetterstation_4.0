// BME
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "ConnectionHandler.h"

// Config files
#include "config_global.h"
#include "config.h"


void setup()
{
    Serial.begin(9600);
    

    timer.setInterval(3600000, [] () {});                                                           
    heartbeat_timer_id = timer.setInterval(HEARTBEAT_DELAY, heartbeat);
    check_connections_timer_id = timer.setInterval(CHECK_CONNECT_DELAY, verifyConnections);

    pinMode(ledRedPin,OUTPUT);
    pinMode(ledGreenPin,OUTPUT);
    pinMode(ledBluePin,OUTPUT);

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