#include "HumTempSensore.hpp"

bool readHumTempSensor(DHT dht, HumTempData* data) {
    data->humidity = dht.readHumidity();
    data->temperature = dht.readTemperature();

    Serial.flush();

    if (isnan(data->humidity) || isnan(data->temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        return false;
    }

    return true;
}