/*----------------------------------------------------------------------------------------------------
  Project Name : MQTT LED Monitor 1.0
  Author: Marc Stähli
  D1 mini & 3 FC11 LED moudles 1 green, 2 red: 
------------------------------------------------------------------------------------------------------*/

char ssid[] = "your_ssid";                           // WiFi Router ssid
char pass[] = "your_wifi_password";             // WiFi Router password

const char* mqtt_server = "192.168.188.87";       // MQTT Server address (mosquitto on orange pi)

/****** Additional Settings **************************************************************************/

// NTP
#define NTP_SERVER      "ch.pool.ntp.org"
#define TZ              1                         // (utc+) TZ in hours
#define TZ_SEC          ((TZ)*3600)               // European DST will be calculated in the code
