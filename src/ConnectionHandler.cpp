#include "Arduino.h"
#include "config_global.h"
#include "config.h"
#include "ConnectionHandler.h"

void handleConnects()
{
    // Init Wifi-Connection
    ledFlasher.attach(0.1, []{ ledFlash(ledRedPin); });
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
        Serial.println("Wi-Fi CONNECTED");
        numWifiConnects++;
        ipv4 = WiFi.localIP().toString();
        ledFlasher.detach();
        digitalWrite(ledRedPin, LOW); // Turn the Green LED off - we want it off all the time that the S20 is powered from the mains and in normal mode
        wifiConnected = true;
        MQTTclient.disconnect();                  // Because it takes around 35 seconds before MQTTclient.connected() will return false, we need to do a disconnect before running Check_Connections()
        timer.enable(check_connections_timer_id); // Re-enable the timers that do things with Wi-Fi & MQTT now that we're no longer in stand-alone mode...
        timer.enable(send_serial_timer_id);
        verifyConnections();
    }
    else
    {
        // we get here if we tried multiple times, but can't connect to Wi-Fi. We need to go into stand-alone mode and wait a while before re-trying...
        ledFlasher.detach();
        digitalWrite(ledRedPin, HIGH); // Turn the Green LED on - we want it on to indicate that we're in 'stand-alone' mode
        wifiConnected = false;
        timer.disable(check_connections_timer_id); // We can suspend the timers that do things with Wi-Fi & MQTT if we're in stand-alone mode (we'll still leave the hearbeat running to print serial debug)...
        timer.disable(send_serial_timer_id);
        long_delay_timout_id = timer.setTimeout(RETRY_WAIT_TIME, handleConnects); // Create a one-shot timeout timer to try re-connecting again after a loger delay
    }
}

void connectMQTT()
{
    ledFlasher.attach(0.1, []
                      { ledFlash(ledBluePin); });
    Serial.println("Connecting to MQTT...  ");
    MQTTclient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
    if (MQTTclient.connect(mqtt_client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD, (BASE_MQTT_TOPIC "/Meta/Status"), 0, 1, "Dead"))
    {
        Serial.println("MQTT Connected");
        ledFlasher.detach();
        digitalWrite(ledBluePin, LOW); // Turn the Green LED OFF - we want it Off in normal operation
        numMQTTConnects++;

        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MQTT_Client_ID", mqtt_client_id.c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Status", "Alive", true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Compiled_Date", COMPILED_DATE, true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Compiled_Time", COMPILED_TIME, true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MAC_Address", WiFi.macAddress().c_str(), true); 
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/IP_Address", ipv4.c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Hostname", WiFi.getHostname(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/WiFi_Connect_Count", String(numWifiConnects).c_str(), true); 
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/MQTT_Connect_Count", String(numMQTTConnects).c_str(), true); 
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Rollover_Count", String(rollover_count).c_str(), true);
        MQTTclient.loop();
    }
    else
    {
        ledFlasher.detach();
        digitalWrite(ledBluePin, HIGH);
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

    Serial.println();
    uptime = millis();
    if (last_uptime > uptime)
    {
        rollover_count++;
        MQTTclient.publish(BASE_MQTT_TOPIC "/Meta/Rollover_Count", String(rollover_count).c_str(), true);
    }

    formatUptime();

    if (MQTTclient.connected()) 
    {
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/RSSI", String(WiFi.RSSI()).c_str(), true);
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Used_RAM", String(1-((double)ESP.getFreeHeap()/(double)ESP.getHeapSize())).c_str(), true); 
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Used_Flash", String(1-((double)ESP.getFreeSketchSpace() / (double)ESP.getFlashChipSize())).c_str(), true); 
        MQTTclient.publish(BASE_MQTT_TOPIC "/Heartbeat/Uptime", uptime_dd_hh_mm_ss, true);  
        MQTTclient.loop();
    }
    last_uptime = uptime; 
} 
