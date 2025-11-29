#include "SystemManager.hpp"
#include "Secrets.hpp"
#include <ArduinoJson.h>

SystemManager::SystemManager(DHT& dht, WiFiManager* wifiManager, GcpAuth* gcpAuth, const char* deviceId)
    : _dht(dht),
      _wifiManager(wifiManager),
      _gcpAuth(gcpAuth),
      _deviceId(deviceId),
      _sendInterval(3600000), // Default: 1 Hour
    //   _sendInterval(10000), // Default: 10 seconds for testing
      _lastSendTime(0) {
}

bool SystemManager::begin() {
    Serial.println("Initializing Sensor Manager...");
    Serial.println("Sensor Manager initialized successfully");
    return true;
}

void SystemManager::setSendInterval(unsigned long intervalMs) {
    _sendInterval = intervalMs;
}

bool SystemManager::readAndSendData() {
    // Read sensor data
    HumTempData data;
    if (!readHumTempSensor(_dht, &data)) {
        Serial.println("Failed to read sensor data");
        return false;
    }
    
    Serial.println("\n--- New Reading ---");
    Serial.printf("Temperature: %.1f°C\n", data.temperature);
    Serial.printf("Humidity: %.1f%%\n", data.humidity);
    
    // Create JSON payload
    JsonDocument doc;
    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["device_id"] = _deviceId;
    
    String payload;
    serializeJson(doc, payload);
    
    // Send to GCP
    int statusCode;
    String response;
    if (_gcpAuth->sendAuthenticatedRequest(payload, statusCode, response)) {
        Serial.println("✓ Data sent successfully to GCP");
        return true;
    } else {
        Serial.printf("✗ Failed to send data (HTTP %d)\n", statusCode);
        return false;
    }
}

void SystemManager::update() {
    // Check WiFi connection
    if (!_wifiManager->ensureConnected()) {
        Serial.println("WiFi connection lost and reconnection failed. Restarting...");
        delay(5000);
        ESP.restart();
    }
    
    // Check if it's time to send data
    unsigned long currentTime = millis();
    if (currentTime - _lastSendTime >= _sendInterval || _lastSendTime == 0) {
        Serial.println("Time to read sensor and send data");
        _lastSendTime = currentTime;
        readAndSendData();
    }
}
