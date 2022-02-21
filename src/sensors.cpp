#include <ArduinoJson.h>
#include <Arduino.h>
#include "sensors.h"
#include <stdio.h>
#include <DHT_U.h>
#include "main.h"

//LDR - https://www.allaboutcircuits.com/projects/design-a-luxmeter-using-a-light-dependent-resistor
#define MAX_ADC_READING 4096.0f
#define ADC_REF_VOLTAGE 3.3f
#define REF_RESISTANCE 9930.0f 
#define LUX_CALC_SCALAR 12518931.0f
#define LUX_CALC_EXPONENT -1.405f

const char* MeasurementTypeStrings[] = { "AIR_TEMP", "AIR_HUM", "GND_HUM", "LGT_INT" };

GenericSensor::GenericSensor(int pin)
{
    this->pin = pin;
}

Measurement::Measurement(float value, MeasurementType type, int date, Sensor* sensor)
{
  this->type = type;
  this->value = value;
  this->date = date;
  this->sensor = sensor;
}

void Measurement::serialize (String& output)
{ 
  DynamicJsonDocument doc(256);
  
  doc["sensor_id"] = sensor->id;
  doc["type"] = MeasurementTypeStrings[type];
  doc["value"] = value;
  doc["date"] = DateTime(date).timestamp();
  
  serializeJson(doc, output);
}

Sensor::Sensor(int id, void* sensor_class, Measurement* (*callback)(Sensor*))
{
  this->sensor_class = sensor_class;
  this->id = id;
  this->readCallback = callback;
}

void Sensor::makeMeasurement()
{
  lastMeasurement = readCallback(this);
}

float readStableValue(int pin)
{
    float value = 0;
  
    for(int i = 0; i < 100; i++)
    {
      value += analogRead(pin);
    }
    return value/100.0f;
}

float getHumidityPercentage(int analogValue, float wetValue, float dryValue){
  return max(0.0f, min(100.0f, (1.0f - (analogValue - wetValue) / (dryValue - wetValue)) * 100.0f));
}

Measurement* readDhtTemperature (Sensor* sensor)
{
  sensors_event_t event;
  DHT_Unified* dht = (DHT_Unified*) sensor->sensor_class;
  dht->temperature().getEvent(&event);
  return new Measurement(event.temperature, MeasurementType::AIR_TEMP, g_epochNow, sensor);
}

Measurement* readDhtHumidity(Sensor* sensor)
{
  sensors_event_t event;
  DHT_Unified* dht = (DHT_Unified*) sensor->sensor_class;
  dht->humidity().getEvent(&event);
  return new Measurement(event.temperature, MeasurementType::AIR_HUM, g_epochNow, sensor);
}

Measurement* readEarthSensor (Sensor* sensor)
{ 
  int pin = *(int*)sensor->sensor_class;
  float rawValue = readStableValue(pin);
  float percentage = getHumidityPercentage(rawValue, 1500.0f, 2190.0f);
  return new Measurement(percentage, MeasurementType::GND_HUM, g_epochNow, sensor);
}

float getLuxValue(float raw)
{
  float rVoltage = raw/MAX_ADC_READING * ADC_REF_VOLTAGE;
  float ldrVoltage = ADC_REF_VOLTAGE - rVoltage;
  float ldrResistance = ldrVoltage/rVoltage * REF_RESISTANCE;
  return LUX_CALC_SCALAR * pow(ldrResistance, LUX_CALC_EXPONENT); // LUX
}

Measurement* readLDRSensor (Sensor* sensor)
{
  int pin = *(int*)sensor->sensor_class;
  float rawValue = readStableValue(pin);
  float lux = getLuxValue(rawValue);
  return new Measurement(lux, MeasurementType::LGT_INT, g_epochNow, sensor);
}