// Minimal ESP8266 WiFi Bridge - With Command Forwarding
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi credentials - YOUR ANDROID HOTSPOT
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// Server settings - PUBLIC URL via devtunnels
const char* serverUrl = "https://6fbx0j5c-5001.inc1.devtunnels.ms";
const bool useHttps = true;

// CRITICAL: WiFiClientSecure must be GLOBAL for HTTPS
WiFiClientSecure wifiClient;

void setup() {
    Serial.begin(115200);
    delay(500);
    
    // CRITICAL: Disable SSL certificate validation for HTTPS
    wifiClient.setInsecure();
    
    Serial.println("\n\n=== ESP8266 WiFi Bridge ===");
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    // Wait for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("ESP8266 IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Server: ");
        Serial.println(serverUrl);
        Serial.println("\n🔄 Polling for commands...\n");
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        delay(1000);
        ESP.restart();
        return;
    }
}

void loop() {
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi disconnected! Restarting...");
        delay(1000);
        ESP.restart();
        return;
    }
    
    // Poll server for commands
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/command/get";
    
    // Use global wifiClient for HTTPS
    http.begin(wifiClient, url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            const char* command = doc["command"];
            
            if (command != nullptr && strlen(command) > 0) {
                // Got a command! Parse and format as JSON for Arduino
                Serial.print("📨 Command received: ");
                Serial.println(command);
                
                // Create JSON command for Arduino
                String jsonCmd = "";
                String cmd = String(command);
                
                if (cmd == "BLINK_FAST") {
                    jsonCmd = "{\"type\":\"blink\",\"duration\":200}";
                } else if (cmd == "BLINK_SLOW") {
                    jsonCmd = "{\"type\":\"blink\",\"duration\":1000}";
                } else if (cmd == "LED_ON") {
                    jsonCmd = "{\"type\":\"led_on\"}";
                } else if (cmd == "LED_OFF") {
                    jsonCmd = "{\"type\":\"led_off\"}";
                } else if (cmd == "STOP") {
                    jsonCmd = "{\"type\":\"stop\"}";
                } else if (cmd == "STATUS") {
                    jsonCmd = "{\"type\":\"status\"}";
                } else {
                    // Forward raw command
                    jsonCmd = command;
                }
                
                // Forward to Arduino on Serial
                Serial.println(jsonCmd);
                Serial.print("✅ Forwarded: ");
                Serial.println(jsonCmd);
                Serial.println();
            }
            // If command is null, no command available (silent)
        } else {
            Serial.print("⚠️ JSON parse error: ");
            Serial.println(error.c_str());
        }
    } else if (httpCode > 0) {
        Serial.print("⚠️ HTTP error: ");
        Serial.println(httpCode);
    } else {
        Serial.print("❌ Connection failed: ");
        Serial.println(http.errorToString(httpCode));
    }
    
    http.end();
    
    delay(3000);  // Poll every 3 seconds
    yield();
}
