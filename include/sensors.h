#pragma once

enum MeasurementType { AIR_TEMP, AIR_HUM, GND_HUM, LGT_INT };
extern const char* MeasurementTypeStrings[] ;

struct Sensor;

struct Measurement
{
    public:
        MeasurementType type;
        float value;     
        Sensor* sensor;
        int date; //Epoch time
        Measurement(float value, MeasurementType type, int date, Sensor* sensor);
        void serialize (String& output);
};

struct Sensor
{   
    private:
        Measurement* (*readCallback)(Sensor*);
    public:
        int id;
        void* sensor_class;
        Measurement* lastMeasurement;

        Sensor(int id, void* sensor_class, Measurement* (*callback)(Sensor*));
        void makeMeasurement();
};

struct GenericSensor
{
        int pin;
        GenericSensor(int pin);
};

Measurement* readLDRSensor (Sensor* sensor);
Measurement* readEarthSensor (Sensor* sensor);
Measurement* readDhtHumidity(Sensor* sensor);
Measurement* readDhtTemperature (Sensor* sensor);