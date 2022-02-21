#include <ArduinoJson.h>
#include "NTPClient.h"
#include <Arduino.h>
#include "sensors.h"
#include "SPIFFS.h"
#include "events.h"
#include "RTClib.h"
#include <DHT_U.h>
#include <WiFi.h>
#include "web.h"

// #define ALARM_INTERRUPT_PIN 27
#define DHT11_PIN 18
#define VALVE_PIN 5
#define LDR_PIN 33
#define EARTH_0 35

#define POST_INTERVAL 60
#define MEASUREMENT_INTERVAL 60

RTC_PCF8563 rtc;
const char *ssid = "AUREO";
const char *password = "BananaCanela";

WiFiUDP udp;
NTPClient ntpClient = NTPClient(udp);
EventsManager* evManager = new EventsManager();

//Sensors
DHT_Unified dht(DHT11_PIN, DHT11);

int g_epochNow = 0;

Sensor* sensors[] = 
{ 
  new Sensor(1, new GenericSensor(EARTH_0), &readEarthSensor), 
  new Sensor(2, new GenericSensor(LDR_PIN), &readLDRSensor),
  new Sensor(3, &dht, &readDhtTemperature),
  new Sensor(4, &dht, &readDhtHumidity),
};

const int sensorCount = sizeof(sensors)/sizeof(void*);

void setValveOn () { digitalWrite(VALVE_PIN, LOW);}
void setValveOff () { digitalWrite(VALVE_PIN, HIGH);}

void wifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println ("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print('.');
    delay(500);
  }

  Serial.printf ("Online");
}

void setEpochNow()
{
  DateTime now = rtc.now();
  g_epochNow = now.unixtime();

  evManager->add(g_epochNow + 60, &setEpochNow);
}

void postAllMeasurements ()
{
  for (int i = 0;i < sensorCount;i++)
  {
    Measurement* lastMeasurement = sensors[i]->lastMeasurement;
    String json; lastMeasurement->serialize(json);
    String table("measurements");
    insertJsonObject(table, json);
  }

  evManager->add(g_epochNow + POST_INTERVAL, postAllMeasurements);
}

void makeAllMeasurements()
{
  for (int i = 0;i < sensorCount;i++)
  {
    sensors[i]->makeMeasurement();
  }
  
  evManager->add(g_epochNow + MEASUREMENT_INTERVAL, makeAllMeasurements);
}

void initiateIrrigation (int seconds)
{
  setValveOn();
  evManager->add(rtc.now().unixtime() + seconds, &setValveOff);
}

void setTimeByNTP (RTC_PCF8563& rtc)
{
  ntpClient.update();
  uint32_t epochTime = ntpClient.getEpochTime();
  DateTime now = DateTime(epochTime) - /*TimeZone*/ TimeSpan(0, 3, 0, 0);
  rtc.adjust(now);
}

void initializeSensors()
{
  dht.begin();
  pinMode(DHT11_PIN, INPUT);
  pinMode(VALVE_PIN, OUTPUT_OPEN_DRAIN);
  pinMode(LDR_PIN, INPUT);
  pinMode(EARTH_0, INPUT);
}

void setup()
{
  Serial.begin(115200);
  SPIFFS.begin();

  initializeSensors();
  wifiConnect();
  // setValveOff ();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  rtc.start();
  ntpClient.begin();
  setTimeByNTP(rtc);
  setEpochNow();

  evManager->add(g_epochNow+MEASUREMENT_INTERVAL, makeAllMeasurements);
  evManager->add(g_epochNow+POST_INTERVAL, postAllMeasurements);

  startServer();
  xTaskCreate(&serveFoverer, "WebServer", 50000, NULL, 0, NULL); //Start web sever thread
}

void loop () 
{
  if(rtc.alarmFired()){ 
    rtc.clearAlarm();
    evManager->advance();
  }

  vTaskDelay(1000);
}