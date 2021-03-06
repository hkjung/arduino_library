#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

extern "C" {
#include "user_interface.h"
}

#include <Thingplus.h>

//////////////////////////////////////////////////////////////////
const char *ssid = "";					                              	//FIXME
const char *password = "";			                            		//FIXME
const char *apikey = "";  		                            			//FIXME APIKEY
const char *ledId = "led-000000000000-0";		                  	//FIXME LED ID
const char *temperatureId = "temperature-000000000000-0";	      //FIXME TEMPERATURE ID
//////////////////////////////////////////////////////////////////

int LED_GPIO = 8;
int TEMP_GPIO = A0;
int reportIntervalSec = 60;

static WiFiClient wifiClient;

static void _serialInit(void)
{
	Serial.begin(115200);
	while (!Serial);// wait for serial port to connect.
	Serial.println();
}

static void _wifiInit(void)
{
#define WIFI_MAX_RETRY 150

	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);

	Serial.print("[INFO] WiFi connecting to ");
	Serial.println(ssid);

	int retry = 0;
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(retry++);
		Serial.print(".");
		if (!(retry % 50))
		  Serial.println();
		if (retry == WIFI_MAX_RETRY) {
			Serial.println();
			Serial.print("[ERR] WiFi connection failed. SSID:");
			Serial.print(ssid);
			Serial.print(" PASSWD:");
			Serial.println(password);
			while (1) {
				yield();
			}
		}
		delay(100);
	}

	Serial.println();
	Serial.print("[INFO] WiFi connected");
	Serial.print(" IP:");
	Serial.println(WiFi.localIP());
}

static void _gpioInit(void) {
	pinMode(LED_GPIO, OUTPUT);
}

char* actuatingCallback(const char *id, const char *cmd, JsonObject& options) {
	if (strcmp(id, ledId) == 0) { 
		if (strcmp(cmd, "on") == 0) {
			digitalWrite(LED_GPIO, HIGH);
			return "success";
		}
		else  if (strcmp(cmd, "off") == 0) {
			digitalWrite(LED_GPIO, LOW);
			return "success";
		}
	}

	return NULL;
}

void setup() {
	_serialInit();

	uint8_t mac[6];
	WiFi.macAddress(mac);

	Serial.print("[INFO] Gateway Id:");
	Serial.println(WiFi.macAddress());

	_wifiInit();
	_gpioInit();

	Thingplus.begin(wifiClient, mac, apikey);
	Thingplus.actuatorCallbackSet(actuatingCallback);
	Thingplus.connect();
}

time_t current;
time_t nextReportInterval = now();

float temperatureGet() {
	int B = 3975;
	float temperature;
	float resistance;
	int a = analogRead(TEMP_GPIO);
	resistance=(float)(1023-a)*10000/a; //get the resistance of the sensor;
	temperature=1/(log(resistance/10000)/B+1/298.15)-273.15;//convert to temperature via datasheet;
	return temperature;
}

void loop() {
	Thingplus.loop();

	current = now();
	if (current > nextReportInterval) {
		Thingplus.gatewayStatusPublish(true, reportIntervalSec * 3);
		Thingplus.sensorStatusPublish(temperatureId, true, reportIntervalSec * 3);
		Thingplus.sensorStatusPublish(ledId, true, reportIntervalSec * 3);
		Thingplus.valuePublish(temperatureId, temperatureGet());
		nextReportInterval = current + reportIntervalSec;
	}
}
