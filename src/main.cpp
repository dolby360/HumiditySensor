#include <Arduino.h>
#include <DHT.h>
#include "WiFiManager.hpp"
#include "GcpAuth.hpp"
#include "SystemManager.hpp"
#include "Secrets.hpp"

// WiFi credentials - replace with your WiFi SSID and password
const char* WIFI_SSID = WIFI_SSID_SECRET;
const char* WIFI_PASSWORD = WIFI_PASSWORD_SECRET;

// Device configuration
const char* DEVICE_ID = "esp32_garage";

#define DHTPIN 10
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiManager* wifiManager = nullptr;
GcpAuth* gcpAuth = nullptr;
SystemManager* systemManager = nullptr;

void setup() {
  Serial.begin(115200);
  delay(2000); // Increased delay for USB CDC
  
  Serial.println("\n\nESP32 Humidity Sensor with GCP Integration");
  Serial.println("==========================================");
  
  // Print chip info
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT sensor initialized");
  
  // Initialize WiFi Manager
  Serial.println("\nInitializing WiFi...");
  wifiManager = new WiFiManager(WIFI_SSID, WIFI_PASSWORD);
  if (!wifiManager->connect()) {
    Serial.println("Failed to connect to WiFi. Restarting...");
    delay(5000);
    ESP.restart();
  }
  
  // Initialize GCP authentication
  gcpAuth = new GcpAuth(wifiManager, SERVICE_ACCOUNT_EMAIL, PRIVATE_KEY_PEM, function_url);
  
  // Initialize sensor manager
  systemManager = new SystemManager(dht, wifiManager, gcpAuth, DEVICE_ID);
  
  if (!systemManager->begin()) {
    Serial.println("Failed to initialize Sensor Manager. Restarting...");
    delay(5000);
    ESP.restart();
  }
  
  // Optional: Set custom send interval (default is 5 minutes)
  // systemManager->setSendInterval(60000); // 1 minute
  
  Serial.println("Setup complete. Starting measurements...");
}

void loop() {
  // All WiFi, authentication, and sensor logic handled by SystemManager
  systemManager->update();
  
  // Small delay to prevent watchdog issues
  delay(100);
}