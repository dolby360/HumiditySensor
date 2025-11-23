#include "WiFiManager.hpp"
#include <time.h>

WiFiManager::WiFiManager(const char* ssid, const char* password)
    : _ssid(ssid),
      _password(password),
      _lastReconnectAttempt(0) {
}

bool WiFiManager::connect(unsigned long timeout) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi already connected");
        return true;
    }
    
    Serial.println("Connecting to WiFi");
    
    // Aggressive WiFi reset for ESP32-C3
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(500);
    
    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    delay(100);
    
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    
    Serial.print("SSID: '");
    Serial.print(_ssid);
    Serial.println("'");
    Serial.print("SSID length: ");
    Serial.println(strlen(_ssid));
    Serial.print("Password length: ");
    Serial.println(strlen(_password));
    
    // Debug: Print password character by character (for debugging only)
    Serial.print("Password (hex): ");
    for(int i = 0; i < strlen(_password); i++) {
        Serial.print(_password[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Scan for networks to verify SSID is visible
    Serial.println("Scanning for WiFi networks...");
    int n = WiFi.scanNetworks();
    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" networks:");
    bool ssidFound = false;
    for (int i = 0; i < n; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm) ");
        Serial.println(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
        
        if (WiFi.SSID(i) == String(_ssid)) {
            ssidFound = true;
            Serial.println("  ^^ TARGET NETWORK FOUND!");
        }
    }
    
    if (!ssidFound) {
        Serial.println("WARNING: Target SSID not found in scan!");
    }
    
    // Try connecting with explicit credentials
    Serial.println("Attempting connection with WiFi.begin()...");
    WiFi.begin(_ssid, _password);
    
    unsigned long start = millis();
    int lastStatus = -1;
    int attempts = 0;
    
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
        delay(500);
        Serial.print(".");
        
        int currentStatus = WiFi.status();
        if (currentStatus != lastStatus) {
            Serial.print(" [Status: ");
            Serial.print(currentStatus);
            switch(currentStatus) {
                case WL_NO_SSID_AVAIL: Serial.print(" - SSID not available"); break;
                case WL_CONNECT_FAILED: Serial.print(" - AUTH FAILED/Wrong Password"); break;
                case WL_CONNECTION_LOST: Serial.print(" - Connection lost"); break;
                case WL_DISCONNECTED: Serial.print(" - Disconnected"); break;
                case WL_IDLE_STATUS: Serial.print(" - Idle"); break;
            }
            Serial.print("] ");
            lastStatus = currentStatus;
        }
        
        // If stuck on disconnected for too long, try reconnecting
        attempts++;
        if (attempts == 10 && currentStatus == WL_DISCONNECTED) {
            Serial.println("\nRetrying connection...");
            WiFi.disconnect();
            delay(100);
            WiFi.begin(_ssid, _password);
            attempts = 0;
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // Synchronize time for JWT and timestamps
        syncTime();
        
        return true;
    }
    
    Serial.println("\nWiFi connection failed");
    return false;
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManager::ensureConnected() {
    if (isConnected()) {
        return true;
    }

    // Throttle reconnection attempts
    unsigned long currentTime = millis();
    if (currentTime - _lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }

    _lastReconnectAttempt = currentTime;
    Serial.println("WiFi disconnected. Attempting reconnection...");

    return connect();
}

String WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "Not connected";
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    Serial.println("WiFi disconnected");
}

bool WiFiManager::syncTime() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync");
    
    time_t now = time(nullptr);
    int retries = 0;
    while (now < 8 * 3600 * 2 && retries++ < 20) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    
    if (now >= 8 * 3600 * 2) {
        Serial.println("\nTime synchronized");
        return true;
    }
    
    Serial.println("\nTime synchronization failed");
    return false;
}
