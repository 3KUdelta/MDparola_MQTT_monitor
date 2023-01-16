/*----------------------------------------------------------------------------------------------------
  Project Name : MQTT LED Monitor 1.0
  Author: Marc Stähli
  ESP32 & 3 FC11 LED moudles 1 green, 2 blue
  ------------------------------------------------------------------------------------------------------*/

#include "Settings.h"  // Don't forget to set your settings!

#include <MD_Parola.h>      // For LED matrix display
#include <MD_MAX72xx.h>     // For LED matrix display
#include <SPI.h>            // For connecting matrix display
#include <PubSubClient.h>   // For MQTT
#include <WiFi.h>    // For WiFi connection with ESP8266
#include <WiFiUdp.h>        // For NTP Signal fetch
#include <EasyNTPClient.h>  // For NTP Signal read https://github.com/aharshac/EasyNTPClient
#include <TimeLib.h>        // For converting NTP time https://github.com/PaulStoffregen/Time.git
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 12
#define MAX_ZONES 2

#define CLK_PIN 32
#define DATA_PIN 33
#define CS_PIN 27

int pirSensor = 5;  // Pin, where PIR sensor is connected

#define SPEED_TIME 25
#define PAUSE_TIME 2000

WiFiUDP udp;
EasyNTPClient ntpClient(udp, NTP_SERVER, 0);

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void (*resetFunc)(void) = 0;  // declare reset function @ address 0

WiFiClient espClient;            // needed for MQTT
PubSubClient client(espClient);  // needed for MQTT

// Global variables
String tempstr;

char actualtime[16];
char outdoortemp[128];
char outdoorhumi[128];
char tempforecast[128];
char compforecast[128];
char cloudforecast[128];
char preciforecast[128];
char trend[128];

bool wiw_accurate_data = false;
bool sws_accurate_data = false;

char *mqtterror = "MQTT Error";
char *wifierror = "WiFi Error";
char *weathererror = "Keine Daten!";

char *message[7] = { outdoortemp, outdoorhumi, tempforecast, compforecast, cloudforecast, preciforecast, trend };

int utc_timediff;
unsigned long timestamp;
unsigned long timebuffer;
unsigned long local_time;
String IPaddress;

AsyncWebServer server(80);  // ElegantOTA

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
  0x00,
  0x81,
  0xc3,
  0xe7,
  0xff,
  0x7e,
  0x7e,
  0x3c,
  0x00,
  0x42,
  0xe7,
  0xe7,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x24,
  0x66,
  0xe7,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x3c,
  0x7e,
  0xff,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x24,
  0x66,
  0xe7,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x42,
  0xe7,
  0xe7,
  0xff,
  0xff,
  0x7e,
  0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
static const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00,
  0x81,
  0xc3,
  0xe7,
  0xff,
  0x7e,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x7b,
  0xf3,
  0x7f,
  0xfb,
  0x73,
  0xfe,
  0x00,
  0x42,
  0xe7,
  0xe7,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x7b,
  0xf3,
  0x7f,
  0xfb,
  0x73,
  0xfe,
  0x24,
  0x66,
  0xe7,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x7b,
  0xf3,
  0x7f,
  0xfb,
  0x73,
  0xfe,
  0x3c,
  0x7e,
  0xff,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x73,
  0xfb,
  0x7f,
  0xf3,
  0x7b,
  0xfe,
  0x24,
  0x66,
  0xe7,
  0xff,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x73,
  0xfb,
  0x7f,
  0xf3,
  0x7b,
  0xfe,
  0x00,
  0x42,
  0xe7,
  0xe7,
  0xff,
  0xff,
  0x7e,
  0x3c,
  0x00,
  0x00,
  0x00,
  0xfe,
  0x73,
  0xfb,
  0x7f,
  0xf3,
  0x7b,
  0xfe,
};

static uint8_t display = 0;  // current display mode

void setup(void) {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Starting MQTT LED Monitor");

  client.setBufferSize(512);  // Increasing PubSubClient buffer

  P.begin(MAX_ZONES);
  P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);

  P.setZone(0, 0, 7);
  P.setZone(1, 8, 11);

  P.setIntensity(0, 2);  // Zone, Val between 1 and 15
  P.setIntensity(1, 2);

  go_online();        // open WiFi and go online
  connect_to_MQTT();  // connect to MQTT mosquitto server
  get_NTP_time();     // get time from NTP server

  P.displayZoneText(1, actualtime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  pinMode(pirSensor, INPUT);
}

void loop(void) {

  if (digitalRead(pirSensor) == 1) {  // PIR Sensor activated --> timer gets started
    timebuffer = millis();
    //Serial.println("Sensor active");
  }

  if (!wiw_accurate_data) {
    tempstr = "wiw sending no data..";
    tempstr.toCharArray(tempforecast, 128);
    tempstr.toCharArray(compforecast, 128);
    tempstr.toCharArray(cloudforecast, 128);
    tempstr.toCharArray(preciforecast, 128);
  }

  if (!sws_accurate_data) {
    tempstr = "sws sending no data..";
    tempstr.toCharArray(outdoortemp, 128);
    tempstr.toCharArray(outdoorhumi, 128);
    tempstr.toCharArray(trend, 128);
  }

  if (!client.connected()) {  // MQTT
    reconnect();
  }
  client.loop();  // MQTT

  if (millis() - timebuffer < 120000) {  // After movement, display remains on for 2 minutes = 120000 ms

    //Serial.println("Timebuffer active");

    sprintf(actualtime, "%02u:%02u", hour(now() + utc_timediff), minute(now() + utc_timediff));

    P.displayAnimate();

    if (P.getZoneStatus(0)) {
      switch (display) {
        case 0:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_UP_LEFT);
          display++;

          break;

        case 1:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SPRITE);
          display++;

          break;

        case 2:
          P.displayZoneText(0, message[display], PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_DOWN_LEFT);
          display++;

          break;

        case 3:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_UP_LEFT);
          display++;

          break;

        case 4:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_DOWN_LEFT);
          display++;

          break;

        case 5:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_UP_LEFT);
          display++;

          break;

        case 6:
          P.displayZoneText(0, message[display], PA_LEFT, SPEED_TIME, PAUSE_TIME, PA_SCROLL_LEFT, PA_SCROLL_DOWN_LEFT);
          display = 0;

          break;
      }
      P.displayReset(0);
    }
    P.displayReset(1);
  } else {
    P.displayClear();  // if no movement on PIR, display shut off
  }

  if ((now() - timestamp) > 600) {  // every 10 minutes
    get_NTP_time();                 // get all xx a time update from NTP Server --> avoiding a constant read from time server
  }
}

void callback(char *topic, byte *payload, unsigned int length) {  // function to retreive all messages from MQTT Server

  char received_rounded[128];

  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  // print received message raw to serial monitor
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "marc/weather/immensee") {

    // The filter: it contains "true" for each value we want to keep
    StaticJsonDocument<128> filter;
    filter["root"]["temperature"] = true;
    filter["root"]["humidity"] = true;
    filter["root"]["pressurestate"] = true;
    filter["root"]["trendinwords"] = true;
    filter["root"]["timestamp"] = true;

    StaticJsonDocument<256> jsonDoc;

    DeserializationError err = deserializeJson(jsonDoc, payload, DeserializationOption::Filter(filter));
    if (err) {
      Serial.print("deserializeJson() failed with code ");
      Serial.println(err.f_str());
    }

    if (now() - long(jsonDoc["root"]["timestamp"]) < 700) sws_accurate_data = true;
    else sws_accurate_data = false;

    dtostrf(float(jsonDoc["root"]["temperature"]), 2, 1, received_rounded);
    tempstr = translateSpecialChars(String("Aussen: ") + String(received_rounded) + "°");
    tempstr.toCharArray(outdoortemp, 128);
    Serial.println(tempstr);

    dtostrf(float(jsonDoc["root"]["humidity"]), 2, 1, received_rounded);
    tempstr = String("LF: ") + String(received_rounded) + "%";
    tempstr.toCharArray(outdoorhumi, 128);
    Serial.println(tempstr);

    tempstr = translateSpecialChars(jsonDoc["root"]["pressurestate"].as<String>() + ", " + jsonDoc["root"]["trendinwords"].as<String>());
    tempstr.toCharArray(trend, 128);
    Serial.println(tempstr);

  } else if (String(topic) == "marc/indoor/epaperdisplay") {

    // The filter: it contains "true" for each value we want to keep
    StaticJsonDocument<128> filter2;
    filter2["root"]["temperature"] = true;

    StaticJsonDocument<128> jsonDoc2;

    DeserializationError err = deserializeJson(jsonDoc2, payload, DeserializationOption::Filter(filter2));
    if (err) {
      Serial.print("deserializeJson() failed with code ");
      Serial.println(err.f_str());
    }
  } else if (String(topic) == "marc/weather/immensee_wiw") {

    StaticJsonDocument<512> jsonDoc3;

    DeserializationError err = deserializeJson(jsonDoc3, payload, length);
    if (err) {
      Serial.print("deserializeJson() failed with code ");
      Serial.println(err.f_str());
    }

    Serial.println(now() - long(jsonDoc3["root"]["owm_wiw_timestamp"]));

    if (now() - long(jsonDoc3["root"]["owm_wiw_timestamp"]) < 4000) wiw_accurate_data = true;
    else wiw_accurate_data = false;

    tempstr = translateSpecialChars(jsonDoc3["root"]["owm_wiw_temp_forecast"].as<String>());
    tempstr.toCharArray(tempforecast, 128);
    Serial.println(tempstr);

    tempstr = translateSpecialChars(jsonDoc3["root"]["owm_wiw_comp_forecast"].as<String>());
    tempstr.toCharArray(compforecast, 128);
    Serial.println(tempstr);

    tempstr = translateSpecialChars(jsonDoc3["root"]["owm_wiw_cloud_forecast"].as<String>());
    tempstr.toCharArray(cloudforecast, 128);
    Serial.println(tempstr);

    tempstr = translateSpecialChars(jsonDoc3["root"]["owm_wiw_preci_forecast"].as<String>());
    tempstr.toCharArray(preciforecast, 128);
    Serial.println(tempstr);
  }
}

void go_online() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname("MQTTLEDDISPLAY");  //This changes the hostname of the ESP8266 to display neatly on the network esp on router.
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
      P.displayZoneText(0, wifierror, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);  // send error message to display
      P.displayReset(0);
      delay(60000);  // wait one minute, then do a reset and try again
      resetFunc();
    }
    Serial.print(".");
  }
  Serial.println("Wifi connected ok.");
  IPaddress = WiFi.localIP().toString();
  Serial.println(IPaddress);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", SendHTML());
  });

  AsyncElegantOTA.begin(&server);  // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}  //end go_online

void get_NTP_time() {
  Serial.println("---> Now reading time from NTP Server");

  while (!ntpClient.getUnixTime()) {
    delay(100);
    Serial.print(".");
  }
  setTime(ntpClient.getUnixTime());  // get UNIX timestamp (seconds from 1.1.1970 on)
  utc_timediff = TZ_SEC;

  if (summertime_EU(year(now()), month(now()), day(now()), hour(now()), TZ)) {
    utc_timediff += 3600;
  }

  local_time = now() + utc_timediff;

  timestamp = now();  // now equals UTC

  Serial.println("Time adjusted to European summertime.");
  Serial.print("Current local time: ");
  Serial.println(local_time);
  Serial.print(hour(local_time));
  Serial.print(": ");
  Serial.print(minute(local_time));
  Serial.print(": ");
  Serial.println(second(local_time));

  client.publish("marc/debug", "MQTT LED Monitor: NTP time fetched from internet");
  delay(50);

}  // end get_NTP_time()

void connect_to_MQTT() {
  Serial.print("---> Connecting to MQTT, ");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("reconnecting MQTT...");
    reconnect();
  }
  Serial.println("MQTT connected ok.");
}  //end connect_to_MQTT

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
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("marc/debug", "MQTT LED Monitor: Client started..");
      // Subscribe to our topics
      client.subscribe("marc/weather/immensee");
      client.subscribe("marc/weather/immensee_wiw");
      client.subscribe("marc/indoor/epaperdisplay");
      delay(50);
    } else {
      Serial.print(" ...failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      P.displayAnimate();
      P.displayZoneText(0, mqtterror, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);  // send error message to display
      P.displayReset(0);

      // Wait 5 seconds before retrying to connect or to reset
      delay(5000);

      if (WiFi.status() != WL_CONNECTED) {
        go_online();
      } else {
        resetFunc();
      }
    }
  }
}  //end void reconnect

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false;  // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true;   // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true;
  else
    return false;
}

String translateSpecialChars(String str) {
  unsigned int l = str.length();  // how many chars were sent?
  char bla[l];                    // define working array of chars
  str.toCharArray(bla, l + 1);    // move string into array of chars
  for (int i = 0; i < l; i++) {
    if (int(bla[i]) == 195) {  // special char is being announced (first is 195)
      bla[i] = 255;            // filling with marker (255 not used)
      switch (int(bla[i + 1])) {
        case 132: bla[i + 1] = 196; break;  //Ä
        case 164: bla[i + 1] = 228; break;  //ä
        case 150: bla[i + 1] = 214; break;  //Ö
        case 182: bla[i + 1] = 246; break;  //ö
        case 156: bla[i + 1] = 220; break;  //Ü
        case 188: bla[i + 1] = 252; break;  //ü
      }
    }
    if (int(bla[i]) == 194) {  // special char is announced (first is 194)
      bla[i] = 255;            // filling with marker (255 not used)
      bla[i + 1] = 176;        // Degree sign (°)
    }
    for (int i = 0; i < l; i++) {  // deleting marker und shifting to left
      if (int(bla[i]) == 255) {
        for (int ii = i; ii < l; ii++) {
          bla[ii] = bla[ii + 1];
        }
      }
    }
  }
  str = String(bla);
  return str;
}

String SendHTML(void) {  // Welcome screen if you arrive at ESP's IP address
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>ESP32 OTA</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 20px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Welcome to \"MQTT Messenger\"-OTA page !!</h1>\n";
  ptr += "</div>\n";
  ptr += "<br>You are currently on the ESP WebServer to initiate over-the-air updates.<br>\n";
  ptr += "<a href=\"http://\n";
  ptr += IPaddress;
  ptr += "/update\">\n";
  ptr += "Go to OTA update page...\n";
  ptr += "</a>\n";
  ptr += "</html>\n";
  return ptr;
}
