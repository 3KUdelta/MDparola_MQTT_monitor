/*----------------------------------------------------------------------------------------------------
  Project Name : MQTT LED Monitor 1.0
  Author: Marc Stähli
  Node MCU & 3 FC11 LED moudles 1 green, 2 blue:
  ------------------------------------------------------------------------------------------------------*/

#include "Settings.h"              // Don't forget to set your settings!

#include <MD_Parola.h>             // For LED matrix display
#include <MD_MAX72xx.h>            // For LED matrix display
#include <SPI.h>                   // For connecting matrix display
#include <PubSubClient.h>          // For MQTT
#include <ESP8266WiFi.h>           // For WiFi connection with ESP8266
#include <WiFiUdp.h>               // For NTP Signal fetch
#include <EasyNTPClient.h>         // For NTP Signal read https://github.com/aharshac/EasyNTPClient
#include <TimeLib.h>               // For converting NTP time https://github.com/PaulStoffregen/Time.git

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12
#define MAX_ZONES 2

#define CLK_PIN   12
#define DATA_PIN  15
#define CS_PIN    13

int pirSensor = 5;                               // Pin, where PIR sensor is connected

#define SPEED_TIME  25
#define PAUSE_TIME  2000

WiFiUDP udp;
EasyNTPClient ntpClient(udp, NTP_SERVER, TZ_SEC);

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void(* resetFunc) (void) = 0;       // declare reset function @ address 0

WiFiClient espClient;               // needed for MQTT
PubSubClient client(espClient);     // needed for MQTT


// Global variables
uint8_t  curText;

uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 };

String tempvar;

char actualtime[16];
char outdoortemp[128];
char pooltemp[128];
char indoortemp[128];
char zambrettiswords[128];
char trend[128];
char *mqtterror = "MQTT Error";
char *wifierror = "WiFi Error";

char *message[5] = {outdoortemp, pooltemp, indoortemp, zambrettiswords, trend};

unsigned long timestamp;
unsigned long timebuffer;

/* displayClear() = alle Zonen löschen
 textEffect_t  effect[] =
{
  PA_RANDOM,
  PA_PRINT,
  PA_SCAN_HORIZ,
  PA_SCROLL_LEFT,
  PA_WIPE,
  PA_SCAN_VERTX,
  PA_SCROLL_UP_LEFT,
  PA_SCROLL_UP,
  PA_FADE,
  PA_OPENING_CURSOR,
  PA_GROW_UP,
  PA_SCROLL_UP_RIGHT,
  PA_BLINDS,
  PA_SPRITE,
  PA_CLOSING,
  PA_GROW_DOWN,
  PA_SCAN_VERT,
  PA_SCROLL_DOWN_LEFT,
  PA_WIPE_CURSOR,
  PA_SCAN_HORIZX,
  PA_DISSOLVE,
  PA_MESH,
  PA_OPENING,
  PA_CLOSING_CURSOR,
  PA_SCROLL_DOWN_RIGHT,
  PA_SCROLL_RIGHT,
  PA_SLICE,
  PA_SCROLL_DOWN,
};*/

// Sprite Definitions
const uint8_t F_PMAN1 = 6;
const uint8_t W_PMAN1 = 8;
static const uint8_t PROGMEM pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
static const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
};

static uint8_t display = 0;  // current display mode

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting MQTT LED Monitor");

  go_online();                    // open WiFi and go online
  connect_to_MQTT();              // connect to MQTT mosquitto server on orange pi
  get_NTP_time();                 // get time from NTP server

  P.begin(MAX_ZONES);
  P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);

  P.setZone(0, 0, 7);
  P.setZone(1, 8, 11);

  P.addChar('$', degC);          //define $ as degree C

  P.setIntensity(0,1);

  P.displayZoneText(1, actualtime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  pinMode(pirSensor, INPUT);
}

void loop(void) {

  if (digitalRead(pirSensor) == 1) {                    // PIR Sensor activated --> timer gets started
    timebuffer = millis();
    Serial.println("Sensor active");
  }

  if (!client.connected()) {    // MQTT
    reconnect();
  }
  client.loop();                // MQTT

  if (millis()-timebuffer < 120000) {                    // After movement, display remains on for 2 minutes = 120000 ms

    Serial.println("Timebuffer active");
    
    sprintf(actualtime, "%02u:%02u", hour(now()), minute(now()));

    P.displayAnimate();

    if (P.getZoneStatus(0))
    {
      switch (display)
      {
        case 0:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_UP_LEFT);
          display++;

          break;

        case 1:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_DOWN_LEFT);
          display++;

          break;

        case 2:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SPRITE);
          display++;

          break;
      
        case 3:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_UP_LEFT);
          display++;

          break;

        case 4:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_DOWN_LEFT);
          display = 0;  

        break;
      }

      P.displayReset(0);
    }

    P.displayReset(1);
  }
  else {
    Serial.println("Display off");
    P.displayClear();                      // if no movement on PIR, display shut off
  }

  if ((now() - timestamp) > 86400) {       // 86400 seconds = 24 hours
    get_NTP_time();                        // get all 24 hours a time update from NTP Server --> avoiding a constant read from time server
  }
}

void callback(char* topic, byte* message, unsigned int length) {       // function to retreive all messages from MQTT Server
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "home/pool/solarcroc/tempc") {
    tempvar = String("Pool: ") + (messageTemp) + ("$");                 //$ will be translated into DegC
    tempvar.toCharArray(pooltemp, 128);
    Serial.println(tempvar);
  }
  else if (String(topic) == "home/weather/solarweatherstation/tempc") {
    tempvar = String("Aussen: ") + (messageTemp) + ("$");               //$ will be translated into DegC
    tempvar.toCharArray(outdoortemp, 128);
    Serial.println(tempvar);
  }
  else if (String(topic) == "home/indoor/epaperdisplay/tempc") {
    tempvar = String("Innen: ") + (messageTemp) + ("$");                //$ will be translated into DegC
    tempvar.toCharArray(indoortemp, 128);
    Serial.println(tempvar);
  }
  else if (String(topic) == "home/weather/solarweatherstation/zambrettisays") {
     
    tempvar = String("Wetter in 5 Stunden: ") + (messageTemp);
    tempvar.toCharArray(zambrettiswords, 128);
    Serial.println(zambrettiswords);
  }
  else if (String(topic) == "home/weather/solarweatherstation/trendinwords") {
     
    tempvar = String("Trend: ") + (messageTemp);
    tempvar.toCharArray(trend, 128);
    Serial.println(trend);
  }
}

void go_online() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname("MQTTLEDDISPLAY");     //This changes the hostname of the ESP8266 to display neatly on the network esp on router.
  WiFi.begin(ssid, pass);
  Serial.print("---> Connecting to WiFi ");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    i++;
    if (i > 20) {
      Serial.println("Could not connect to WiFi!");
      Serial.println("Doing a reset now and retry a connection from scratch.");
      P.displayAnimate();
      P.displayZoneText(0, wifierror, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT); // send error message to display
      P.displayReset(0);
      delay(60000);        // wait one minute, then do a reset and try again
      resetFunc();
    }
    Serial.print(".");
  }
  Serial.println("Wifi connected ok.");
} //end go_online

void get_NTP_time() {

  Serial.println("---> Now reading time from NTP Server");

  while (!ntpClient.getUnixTime()) {
    delay(100);
    Serial.print(".");
  }
  setTime(ntpClient.getUnixTime());           // get UNIX timestamp (seconds from 1.1.1970 on)

  if (summertime_EU(year(now()), month(now()), day(now()), hour(now()), 1)) {
    adjustTime(3600);                         // adding one hour
  }

  timestamp = now();

  Serial.print("Actual UNIX timestamp: ");
  Serial.println(now());

  Serial.print("Time & Date: ");
  Serial.print(hour(now()));
  Serial.print(":");
  Serial.print(minute(now()));
  Serial.print(":");
  Serial.print(second(now()));
  Serial.print("; ");
  Serial.print(day(now()));
  Serial.print(".");
  Serial.print(month(now()));
  Serial.print(".");
  Serial.println(year(now()));

  client.publish("home/debug", "MQTT LED Monitor: NTP time fetched from internet");
  delay(50);

} // end get_NTP_time()

void connect_to_MQTT() {
  Serial.print("---> Connecting to MQTT, ");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("reconnecting MQTT...");
    reconnect();
  }
  Serial.println("MQTT connected ok.");
} //end connect_to_MQTT

void reconnect() {
  // Loop until we're reconnected
  unsigned int i = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection with ");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    Serial.print(clientId.c_str());
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("home/debug", "MQTT LED Monitor: Client ok...");
      // Subscribe to our topics
      client.subscribe("home/weather/solarweatherstation/tempc");
      client.subscribe("home/weather/solarweatherstation/humi");
      client.subscribe("home/weather/solarweatherstation/zambrettisays");
      client.subscribe("home/weather/solarweatherstation/trendinwords");
      client.subscribe("home/indoor/epaperdisplay/tempc");
      client.subscribe("home/indoor/epaperdisplay/humi");
      client.subscribe("home/pool/solarcroc/tempc");
      delay(50);
    } else {
      Serial.print(" ...failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      P.displayAnimate();
      P.displayZoneText(0, mqtterror, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);  // send error message to display
      P.displayReset(0);
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //end void reconnect

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true;  // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true;
  else
    return false;
}
