#pragma once

#define COMPILED_DATE __DATE__
#define COMPILED_TIME __TIME__

#define CLIENT_ID "Outsidesensor"
#define BASE_MQTT_TOPIC "Outsidesensor"
#define HOSTNAME "ESP32-Outsidesensor"

static const String mqtt_client_id = String(CLIENT_ID) + "_" + String(ESP.getChipModel());
extern String ipv4;

#define LEDREDPIN 19
#define LEDGREENPIN 18
#define LEDBLUEPIN 5
#define LDRPIN 0
#define FANPIN 17