#include <Arduino.h>
#include <DHT.h>
#include "HumTempSensore.hpp"

#define DHTPIN 10
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  delay(2000);
  HumTempData data;
  if (readHumTempSensor(dht, &data)){
    
  }
}