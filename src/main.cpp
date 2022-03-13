#include <ArduinoJson.h>
#include "NTPClient.h"
#include <Arduino.h>
#include "sensors.h"
#include "sqlite3.h"
#include "SPIFFS.h"
#include "events.h"
#include "RTClib.h"
#include <DHT_U.h>
#include <WiFi.h>
#include "web.h"

// #define ALARM_INTERRUPT_PIN 27
#define DHT_PIN 18
#define VALVE_PIN 5
#define LDR_PIN 33
#define EARTH_0 35

#define POST_INTERVAL 120
#define TIME_UPDATE_INTERVAL 120
#define MEASUREMENT_INTERVAL 120

RTC_PCF8563 rtc;
sqlite3 *db;

const char *ssid = "AUREO";
const char *password = "BananaCanela";

WiFiUDP udp;
NTPClient ntpClient = NTPClient(udp);

EventsManager* evManager = new EventsManager();

DHT_Unified dht(DHT_PIN, DHT22);

Sensor* sensors[] = 
{ 
  new Sensor(1, new GenericSensor(EARTH_0), &readEarthSensor), 
  new Sensor(2, new GenericSensor(LDR_PIN), &readLDRSensor),
  new Sensor(3, &dht, &readDhtTemperature),
  new Sensor(4, &dht, &readDhtHumidity),
};

const int sensorCount = sizeof(sensors)/sizeof(void*);

int g_epochNow = 0;

void setValveOn () { digitalWrite(VALVE_PIN, LOW);}
void setValveOff () { digitalWrite(VALVE_PIN, HIGH);}

void wifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(2);
  }
}

void setEpochNow()
{
  DateTime now = rtc.now();
  g_epochNow = now.unixtime();

  evManager->add(g_epochNow + TIME_UPDATE_INTERVAL, &setEpochNow);
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
  evManager->add(g_epochNow + seconds, &setValveOff);
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
  pinMode(DHT_PIN, INPUT);
  pinMode(VALVE_PIN, OUTPUT_OPEN_DRAIN);
  pinMode(LDR_PIN, INPUT);
  pinMode(EARTH_0, INPUT);
}

void setup()
{
  SPIFFS.begin();

  if (sqlite3_open("/spiffs/my.db", &db))
    return;

  sqlite3_initialize();
  sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS measurements (value FLOAT, type TEXT, date DATETIME, sensor_id INT)", NULL, NULL, NULL);

  initializeSensors();
  wifiConnect();

  if (!rtc.begin()) {
    while (1) delay(10);
  }

  rtc.start();
  ntpClient.begin();
  setTimeByNTP(rtc);
  setEpochNow();

  evManager->add(g_epochNow+MEASUREMENT_INTERVAL, makeAllMeasurements);
  evManager->add(g_epochNow+POST_INTERVAL, postAllMeasurements);

  startServer();
}

//Checks if the alarm fired each second
void loop () 
{
  if(rtc.alarmFired()){ 
    rtc.clearAlarm();
    evManager->advance();
  }

  vTaskDelay(1000);
}