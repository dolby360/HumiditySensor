#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "WiFiManager.hpp"

class GcpAuth {
public:
    GcpAuth(WiFiManager* wifiManager, const char* serviceAccountEmail, const char* privateKeyPem, const char* functionUrl);
    
    // Get a fresh ID token for authentication
    String getIdToken();
    
    // Send authenticated POST request with JSON payload
    bool sendAuthenticatedRequest(const String& payload, int& statusCode, String& response);

private:
    WiFiManager* _wifiManager;
    const char* _serviceAccountEmail;
    const char* _privateKeyPem;
    const char* _functionUrl;
    String _idToken;
    unsigned long _tokenExpiry;
    
    // Create JWT token signed with private key
    String createJWT();
    
    // Exchange JWT for ID token
    String exchangeJWTForIdToken(const String& jwt);
    
    // Base64URL encoding
    String base64UrlEncode(const uint8_t* data, size_t length);
    String base64UrlEncode(const String& str);
};
