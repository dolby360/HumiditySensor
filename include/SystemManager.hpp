#pragma once

#include <Arduino.h>
#include <DHT.h>
#include "HumTempSensore.hpp"
#include "GcpAuth.hpp"
#include "WiFiManager.hpp"

class SystemManager {
public:
    SystemManager(DHT& dht, WiFiManager* wifiManager, GcpAuth* gcpAuth, const char* deviceId);
    
    // Initialize sensor manager
    bool begin();
    
    // Read sensor and send to GCP if interval has passed
    void update();
    
    // Set how often to send data (in milliseconds)
    void setSendInterval(unsigned long intervalMs);
    
private:
    DHT& _dht;
    WiFiManager* _wifiManager;
    GcpAuth* _gcpAuth;
    const char* _deviceId;
    
    unsigned long _sendInterval;
    unsigned long _lastSendTime;
    
    // Read sensor and send data to GCP
    bool readAndSendData();
};
