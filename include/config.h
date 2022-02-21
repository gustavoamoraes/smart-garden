#pragma once

#include <Arduino.h>

class Config
{
  public:

    bool humidityCheck;
    float humidityTarget;
    bool scheduleWater;
    int* irrigationSchedule;

    Config(String& json);
    ~Config();
    Config();

    void serialize (String& output);
};

extern Config* main_config;