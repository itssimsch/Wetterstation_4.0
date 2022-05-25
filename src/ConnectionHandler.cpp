#include "Arduino.h"

// Load Config files
#include "config_global.h"

#ifdef INSIDESENSOR
#include "config_insidesensor.h"
#endif
#ifdef OUTSIDESENSOR
#include "config_outsidesensor.h"
#endif

#include "ConnectionHandler.h"

int numMQTTConnects = 0;
int numWifiConnects = 0;
unsigned long rollover_count = 0;
int wifiConnectAttempts = 0;
char uptime_dd_hh_mm_ss[12] = "";
unsigned long last_uptime = 0;
unsigned long uptime = 0;
String ipv4 = "";

String getServerIp(){
    String finalServerIp;
    int dnsResolveTries = 0;

    // Try to resolve hostname if defined
    #ifdef MQTT_SERVER_HOSTNAME
        while (mdns_init() != ESP_OK)
        {
            delay(1000);
            Serial.println("Starting MDNS...");
        }
        do
        {
            Serial.println("Resolving host...");
            delay(250);
            finalServerIp = MDNS.queryHost(MQTT_SERVER_HOSTNAME).toString();
            dnsResolveTries++;
        } while (finalServerIp == "0.0.0.0" && dnsResolveTries < MAX_DNS_RESOLVE_TRIES);
        if(finalServerIp != "0.0.0.0") return finalServerIp;
    #endif

    // Use predefined IP for connection if only ip is defined or resolving hostname failed
    #ifdef MQTT_SERVER_IP
        finalServerIp = MQTT_SERVER_IP;
        return finalServerIp;
    #endif

    // If hostname resolve failed and no IP was defined restart ESP
    ESP.restart();
    
}

void handleConnects()
{
    // Init Wifi-Connection
    ledFlasher.attach(0.1, []
                      { ledFlash(LEDREDPIN); });
    wifiConnectAttempts = 0;

    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(SSID, PASS);
    }

    while (WiFi.status() != WL_CONNECTED && wifiConnectAttempts < MAX_WIFI_CONNECT_ATTEMPTs)
    {
        delay(WIFI_CONNECT_ATTEMPT_DELAY);
        wifiConnectAttempts++;
        Serial.printf("Attempting Wifi-Connection: Attempt %d / %d\n", wifiConnectAttempts, MAX_WIFI_CONNECT_ATTEMPTs);
    }

    if (WiFi.status() == WL_CONNECTED)
    {

        WiFi.mode(WIFI_STA);
        WiFi.setHostname(HOSTNAME);
        Serial.println("Wi-Fi CONNECTED");
        numWifiConnects++;
        ipv4 = WiFi.localIP().toString();
        ledFlasher.detach();
        digitalWrite(LEDREDPIN, LOW); // Turn the Green LED off - we want it off all the time that the S20 is powered from the mains and in normal mode
        wifiConnected = true;
        MQTTclient.disconnect();                  // Because it takes around 35 seconds before MQTTclient.connected() will return false, we need to do a disconnect before running Check_Connections()
        timer.enable(check_connections_timer_id); // Re-enable the timers that do things with Wi-Fi & MQTT now that we're no longer in stand-alone mode...
        verifyConnections();
    }
    else
    {
        // we get here if we tried multiple times, but can't connect to Wi-Fi. We need to go into stand-alone mode and wait a while before re-trying...
        ledFlasher.detach();
        digitalWrite(LEDREDPIN, HIGH); // Turn the Green LED on - we want it on to indicate that we're in 'stand-alone' mode
        wifiConnected = false;
        timer.disable(check_connections_timer_id);                                // We can suspend the timers that do things with Wi-Fi & MQTT if we're in stand-alone mode (we'll still leave the hearbeat running to print serial debug)...
        long_delay_timout_id = timer.setTimeout(RETRY_WAIT_TIME, handleConnects); // Create a one-shot timeout timer to try re-connecting again after a loger delay
    }
}

void connectMQTT()
{
    ledFlasher.attach(0.1, []
                      { ledFlash(LEDBLUEPIN); });
    Serial.println("Connecting to MQTT...  ");
    String serverIP = getServerIp();
    MQTTclient.setServer(serverIP.c_str(), MQTT_SERVER_PORT);
    MQTTclient.setKeepAlive(120);
    MQTTclient.setSocketTimeout(60);
    if (MQTTclient.connect(mqtt_client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD, (BASE_MQTT_TOPIC "/Meta/Status"), 1, 1, "Dead", false))
    {
        Serial.println("MQTT Connected");
        ledFlasher.detach();
        digitalWrite(LEDBLUEPIN, LOW); // Turn the Green LED OFF - we want it Off in normal operation
        numMQTTConnects++;

        MQTTclient.publish(BASE_MQTT_TOPIC "/Control/Rebooting", "false", true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MQTT_Client_ID", mqtt_client_id.c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Status", "Alive", true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Compiled_Date", COMPILED_DATE, true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Compiled_Time", COMPILED_TIME, true);
        MQTTclient.loop();
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MAC_Address", WiFi.macAddress().c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/IP_Address", ipv4.c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Hostname", WiFi.getHostname(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/WiFi_Connect_Count", String(numWifiConnects).c_str(), true);
        MQTTclient.loop();
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MQTT_Connect_Count", String(numMQTTConnects).c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Rollover_Count", String(rollover_count).c_str(), true);

#ifdef INSIDESENSOR
        MQTTclient.subscribe("Outsidesensor/Data/Temperature");
        MQTTclient.subscribe("Outsidesensor/Data/Humidity");
        MQTTclient.subscribe("Outsidesensor/Data/Pressure");
        MQTTclient.subscribe("Outsidesensor/Data/AbsoluteHumidity");
        MQTTclient.loop();
        MQTTclient.subscribe("Insidesensor/Control/TemperatureOffset");
        MQTTclient.subscribe("Insidesensor/Control/HumidityOffset");
        MQTTclient.subscribe("Insidesensor/Control/PressureOffset");
        MQTTclient.subscribe("Insidesensor/Control/Rebooting");
#endif
#ifdef OUTSIDESENSOR
        MQTTclient.subscribe("Outsidesensor/Control/TemperatureOffset");
        MQTTclient.subscribe("Outsidesensor/Control/HumidityOffset");
        MQTTclient.loop();
        MQTTclient.subscribe("Outsidesensor/Control/PressureOffset");
        MQTTclient.subscribe("Outsidesensor/Control/Rebooting");
        MQTTclient.subscribe("Outsidesensor/Control/Fanstate");
#endif
    }
    else
    {
        ledFlasher.detach();
        digitalWrite(LEDBLUEPIN, HIGH);
        Serial.printf("MQTT Connection FAILED, Return Code = %d\n", MQTTclient.state());
    }
}

void verifyConnections()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        handleConnects();
    }

    if (wifiConnected)
    {
        if (!MQTTclient.connected())
        {
            connectMQTT();
        }
    }
}

void ledFlash(int pin)
{
    int state = digitalRead(pin);
    digitalWrite(pin, !state);
}

void formatUptime()
{
    int systemUpTimeSec = int((uptime / (1000)) % 60);
    int systemUpTimeMin = int((uptime / (1000 * 60)) % 60);
    int systemUpTimeHr = int((uptime / (1000 * 60 * 60)) % 24);
    int systemUpTimeDay = int((uptime / (1000 * 60 * 60 * 24)) % 365);
    sprintf(uptime_dd_hh_mm_ss, "%02d:%02d:%02d:%02d", systemUpTimeDay, systemUpTimeHr, systemUpTimeMin, systemUpTimeSec);
}

void heartbeat()
{
    if (!wifiConnected && WiFi.status() == WL_CONNECTED)
    {
        handleConnects();
    }
    uptime = millis();
    if (last_uptime > uptime)
    {
        rollover_count++;
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Rollover_Count", String(rollover_count).c_str(), true);
    }

    formatUptime();

    if (MQTTclient.connected())
    {
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Status", "Alive", true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/RSSI", String(WiFi.RSSI()).c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Used_RAM", String(1 - ((double)ESP.getFreeHeap() / (double)ESP.getHeapSize())).c_str(), true);
        MQTTclient.loop();
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Used_Flash", String(1 - ((double)ESP.getFreeSketchSpace() / (double)ESP.getFlashChipSize())).c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Uptime", uptime_dd_hh_mm_ss, true);
    }
    last_uptime = uptime;
}
