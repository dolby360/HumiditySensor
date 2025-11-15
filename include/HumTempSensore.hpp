#pragma once

#include <Arduino.h>
#include <DHT.h>

typedef struct {
    float temperature;
    float humidity;
} HumTempData;


bool readHumTempSensor(DHT dht, HumTempData* data);