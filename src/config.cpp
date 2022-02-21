#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "config.h"

//Creates config object from its json object
Config::Config (String& json)
{
  DynamicJsonDocument doc(512);
  deserializeJson(doc, json);

  humidityCheck = doc["humidityCheck"];
  humidityTarget = doc["humidityTarget"];
  scheduleWater = doc["scheduleWater"];

  JsonArray array = doc["irrigationTimePairs"].as<JsonArray>();

  int count = array.size();
  irrigationSchedule = (int*) calloc(count + 1, sizeof(int));
  irrigationSchedule[0] = count;
  irrigationSchedule = irrigationSchedule+1;

  for (size_t i = 0; i < count; i++){
    irrigationSchedule[i] = doc["irrigationTimePairs"][i];
  }

  //Save config
  File file = SPIFFS.open("/config.json", FILE_WRITE);
  file.print(json);
  file.close();
}

//Default config
Config::Config() 
{
  humidityCheck = false;
  humidityTarget = 0.0f;
  scheduleWater = false;
  irrigationSchedule = NULL;
}

Config::~Config ()
{
  delete[] (irrigationSchedule-1);
}

Config* main_config;