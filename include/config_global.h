#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <SimpleTimer.h>
#include <ESPmDNS.h>

#define SSID ""
#define PASS ""
#define MQTT_SERVER_HOSTNAME "rpi4"
#define MQTT_SERVER_IP "10.0.0.137" // If server hostname is defined, ip acts as a fallback system
#define MQTT_SERVER_PORT 1883
#define MQTT_USERNAME "sensor"
#define MQTT_PASSWORD "sensor"
#define WIFI_CONNECT_ATTEMPT_DELAY 500   
#define MAX_WIFI_CONNECT_ATTEMPTs 10
#define MAX_DNS_RESOLVE_TRIES 10    
#define RETRY_WAIT_TIME 60000 
#define HEARTBEAT_DELAY 5000                                
#define CHECK_CONNECT_DELAY 5000
#define NODE_RED_DB_URL "http://rpi4:1880/ui" 

// Vars
extern WiFiClient My_WiFi_Client;
extern PubSubClient MQTTclient;
extern Ticker ledFlasher;  
extern SimpleTimer timer;

// Connections  
extern bool wifiConnected;
extern int wifiConnectAttempts; 

// Timers
extern int heartbeat_timer_id;
extern int check_connections_timer_id;
extern int long_delay_timout_id;

// Monitoring variables
extern int numWifiConnects;               
extern int numMQTTConnects;               
extern unsigned long uptime;
extern char uptime_dd_hh_mm_ss [12];
extern unsigned long last_uptime; 
extern unsigned long rollover_count; 
