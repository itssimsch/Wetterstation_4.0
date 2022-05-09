#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <SimpleTimer.h>

#define SSID ""
#define PASS ""
#define MQTT_SERVER_IP "10.0.0.35"
#define MQTT_SERVER_PORT 1883
#define MQTT_USERNAME "sensor"
#define MQTT_PASSWORD "sensor"

static WiFiClient My_WiFi_Client;
static PubSubClient MQTTclient(My_WiFi_Client);
static Ticker ledFlasher;  
static SimpleTimer timer;

// Connections       
#define WIFI_CONNECT_ATTEMPT_DELAY 500   
#define MAX_WIFI_CONNECT_ATTEMPTs 10    
#define RETRY_WAIT_TIME 60000 
static bool wifiConnected = false;
static int wifiConnectAttempts;    

// Heartbeat
#define HEARTBEAT_DELAY 5000                                
#define CHECK_CONNECT_DELAY 30000 


// Timers
static int heartbeat_timer_id;
static int check_connections_timer_id;
static int send_serial_timer_id;
static int long_delay_timout_id;


// Monitoring variables
static int numWifiConnects;               
static int numMQTTConnects;               
static unsigned long uptime;
static char uptime_dd_hh_mm_ss [12];
static unsigned long last_uptime; 
static unsigned long rollover_count; 