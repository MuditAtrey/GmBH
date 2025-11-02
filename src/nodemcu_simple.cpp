/*
 * NodeMCU Simple HTTP Bridge
 * 
 * This is a clean, minimal version that:
 * - Connects to WiFi
 * - Pings a local HTTP server every 2 seconds
 * - No HTTPS, no SSL, no complex features
 * 
 * Hardware: NodeMCU v3 (ESP8266)
 * Server: Local HTTP server at http://192.168.x.x:5000
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// WiFi credentials
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// Server settings (AUTO-CONFIGURED)
// Your computer's local IP address
String serverIP = "10.147.66.174";  // Your Mac's IP
int serverPort = 5001;
String serverUrl = "http://" + serverIP + ":" + String(serverPort);

// WiFi client for HTTP (no SSL/TLS)
WiFiClient client;

// Stats
unsigned long lastPing = 0;
unsigned long pingInterval = 2000;  // Ping every 2 seconds
int successCount = 0;
int failCount = 0;

void setup() {
    // Start serial
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║   NodeMCU Simple HTTP Bridge          ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("\nMode: Local HTTP Only (No SSL)");
    
    // Connect to WiFi
    Serial.print("\n📡 Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("   IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("   Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("   Server: ");
        Serial.println(serverUrl);
        Serial.println("\n🔄 Starting ping loop...\n");
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Serial.println("   Check your credentials and restart.");
        while(1) { delay(1000); }
    }
}

void pingServer() {
    HTTPClient http;
    
    String url = serverUrl + "/api/nodemcu/ping";
    
    // Begin HTTP request
    http.begin(client, url);
    http.setTimeout(3000);
    http.addHeader("Content-Type", "application/json");
    
    Serial.print("📤 Pinging server... ");
    
    // Send POST request
    int httpCode = http.POST("{}");
    
    if (httpCode > 0) {
        if (httpCode == 200) {
            String response = http.getString();
            Serial.print("✅ Success! (");
            Serial.print(httpCode);
            Serial.println(")");
            Serial.print("   Response: ");
            Serial.println(response);
            successCount++;
        } else {
            Serial.print("⚠️  HTTP ");
            Serial.println(httpCode);
            failCount++;
        }
    } else {
        Serial.print("❌ Failed: ");
        Serial.println(http.errorToString(httpCode));
        failCount++;
    }
    
    http.end();
    
    // Print stats every 10 pings
    static int pingCount = 0;
    pingCount++;
    if (pingCount % 10 == 0) {
        Serial.println("\n╔════════════════════════════════════════╗");
        Serial.println("║  Statistics                            ║");
        Serial.println("╚════════════════════════════════════════╝");
        Serial.print("Total Pings: ");
        Serial.println(pingCount);
        Serial.print("Success: ");
        Serial.print(successCount);
        Serial.print(" (");
        Serial.print((successCount * 100) / pingCount);
        Serial.println("%)");
        Serial.print("Failed: ");
        Serial.println(failCount);
        Serial.print("Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
        Serial.print("Free Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        Serial.println();
    }
}

void loop() {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi disconnected! Reconnecting...");
        WiFi.reconnect();
        delay(5000);
        return;
    }
    
    // Ping server every interval
    if (millis() - lastPing > pingInterval) {
        pingServer();
        lastPing = millis();
    }
    
    delay(10);
}
