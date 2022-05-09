#pragma once

#define COMPILED_DATE __DATE__
#define COMPILED_TIME __TIME__

#define CLIENT_ID "ESP32-1"
#define BASE_MQTT_TOPIC "test/esp32-1"

static const String mqtt_client_id = String(CLIENT_ID) + "_" + String(ESP.getChipModel());
static String ipv4;

 
static const int ledRedPin = 19;
static const int ledGreenPin = 18;
static const int ledBluePin = 5;