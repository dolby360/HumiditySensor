#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager(const char* ssid, const char* password);
    
    // Initialize WiFi connection with timeout
    bool connect(unsigned long timeout = 10000);
    
    // Check if WiFi is connected
    bool isConnected();
    
    // Ensure WiFi is connected, attempt reconnection if needed
    bool ensureConnected();
    
    // Get current IP address
    String getIPAddress();
    
    // Disconnect from WiFi
    void disconnect();

private:
    const char* _ssid;
    const char* _password;
    unsigned long _lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds between reconnect attempts
    
    // Configure NTP time synchronization
    bool syncTime();
};
