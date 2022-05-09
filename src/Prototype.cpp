#include <WiFi.h>
#include <PubSubClient.h>

// BME
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char *ssid = "";
const char *password = "";
const char *mqttServer = "10.0.0.35";
const int mqttPort = 1883;
const char *mqttUser = "sensor";
const char *mqttPassword = "sensor";

Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    pinMode(23, OUTPUT);
    digitalWrite(23, HIGH);
    bme.begin(0x76);

    Serial.begin(9600);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");

    client.setServer(mqttServer, mqttPort);
    while (!client.connected())
    {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP32Client", mqttUser, mqttPassword))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    
}

void loop()
{
    client.loop();
    client.publish("test/temp", String(bme.readTemperature()).c_str());
    delay(1000);
}