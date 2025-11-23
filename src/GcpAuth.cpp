#include "GcpAuth.hpp"
#include "Secrets.hpp"
#include <WiFi.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <time.h>

GcpAuth::GcpAuth(WiFiManager* wifiManager, const char* serviceAccountEmail, const char* privateKeyPem, 
    const char* functionUrl):
        _wifiManager(wifiManager),
        _serviceAccountEmail(serviceAccountEmail),
        _privateKeyPem(privateKeyPem),
        _functionUrl(functionUrl),
        _tokenExpiry(0)
{
}

String GcpAuth::base64UrlEncode(const uint8_t* data, size_t length) {
    size_t outLen = 0;
    mbedtls_base64_encode(NULL, 0, &outLen, data, length);
    
    uint8_t* output = new uint8_t[outLen];
    mbedtls_base64_encode(output, outLen, &outLen, data, length);
    
    String result = String((char*)output);
    delete[] output;
    
    // Convert to base64url format
    result.replace("+", "-");
    result.replace("/", "_");
    result.replace("=", "");
    
    return result;
}

String GcpAuth::base64UrlEncode(const String& str) {
    return base64UrlEncode((const uint8_t*)str.c_str(), str.length());
}

String GcpAuth::createJWT() {
    time_t now = time(nullptr);
    time_t exp = now + 3600; // Token valid for 1 hour
    
    // JWT Header
    JsonDocument headerDoc;
    headerDoc["alg"] = "RS256";
    headerDoc["typ"] = "JWT";
    
    String headerJson;
    serializeJson(headerDoc, headerJson);
    String headerEncoded = base64UrlEncode(headerJson);
    
    // JWT Payload
    JsonDocument payloadDoc;
    payloadDoc["iss"] = _serviceAccountEmail;
    payloadDoc["sub"] = _serviceAccountEmail;
    payloadDoc["aud"] = GCP_TOKEN_URL;
    payloadDoc["iat"] = now;
    payloadDoc["exp"] = exp;
    payloadDoc["target_audience"] = _functionUrl;
    
    String payloadJson;
    serializeJson(payloadDoc, payloadJson);
    String payloadEncoded = base64UrlEncode(payloadJson);
    
    // Create signature base
    String signatureBase = headerEncoded + "." + payloadEncoded;
    
    // Sign with private key
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    
    int ret = mbedtls_pk_parse_key(&pk, 
                                    (const unsigned char*)_privateKeyPem, 
                                    strlen(_privateKeyPem) + 1,
                                    NULL, 
                                    0);
    
    if (ret != 0) {
        Serial.printf("Failed to parse private key: -0x%04x\n", -ret);
        mbedtls_pk_free(&pk);
        return "";
    }
    
    // Hash the signature base with SHA256
    uint8_t hash[32];
    mbedtls_md_context_t md_ctx;
    mbedtls_md_init(&md_ctx);
    mbedtls_md_setup(&md_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&md_ctx);
    mbedtls_md_update(&md_ctx, (const unsigned char*)signatureBase.c_str(), signatureBase.length());
    mbedtls_md_finish(&md_ctx, hash);
    mbedtls_md_free(&md_ctx);
    
    // Sign the hash
    uint8_t signature[256];
    size_t sig_len = 0;
    ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, 32, signature, &sig_len, NULL, NULL);
    
    mbedtls_pk_free(&pk);
    
    if (ret != 0) {
        Serial.printf("Failed to sign JWT: -0x%04x\n", -ret);
        return "";
    }
    
    String signatureEncoded = base64UrlEncode(signature, sig_len);
    String jwt = signatureBase + "." + signatureEncoded;
    
    return jwt;
}

String GcpAuth::exchangeJWTForIdToken(const String& jwt) {
    WiFiClientSecure client;
    client.setInsecure(); // For simplicity - in production, use proper certificate validation
    
    HTTPClient http;
    http.begin(client, GCP_TOKEN_URL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String postData = "grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=" + jwt;
    
    int httpCode = http.POST(postData);
    String idToken = "";
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            idToken = doc["id_token"].as<String>();
            Serial.println("Successfully obtained ID token");
        } else {
            Serial.println("Failed to parse token response");
        }
    } else {
        Serial.printf("Token exchange failed: %d\n", httpCode);
        Serial.println(http.getString());
    }
    
    http.end();
    return idToken;
}

String GcpAuth::getIdToken() {
    // Check if we have a valid cached token
    if (_idToken.length() > 0 && millis() < _tokenExpiry) {
        return _idToken;
    }
    
    Serial.println("Creating JWT...");
    String jwt = createJWT();
    if (jwt.length() == 0) {
        Serial.println("Failed to create JWT");
        return "";
    }
    
    Serial.println("Exchanging JWT for ID token...");
    _idToken = exchangeJWTForIdToken(jwt);
    
    if (_idToken.length() > 0) {
        // Cache token for 50 minutes (tokens are valid for 1 hour)
        _tokenExpiry = millis() + (50 * 60 * 1000);
    }
    
    return _idToken;
}

bool GcpAuth::sendAuthenticatedRequest(const String& payload, int& statusCode, String& response) {
    if (!_wifiManager->isConnected()) {
        Serial.println("WiFi not connected");
        return false;
    }
    
    String idToken = getIdToken();
    if (idToken.length() == 0) {
        Serial.println("Failed to get ID token");
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure(); // For simplicity - in production, use proper certificate validation
    
    HTTPClient http;
    http.begin(client, _functionUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + idToken);
    
    Serial.println("Sending request to GCP...");
    statusCode = http.POST(payload);
    response = http.getString();
    
    Serial.printf("Response code: %d\n", statusCode);
    Serial.println("Response: " + response);
    
    http.end();
    
    return statusCode == HTTP_CODE_OK || statusCode == HTTP_CODE_CREATED;
}
