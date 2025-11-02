/*
 * ESP8266 Transparent WiFi-to-Serial Bridge
 * 
 * This is a DUMB PIPE - it just forwards data between WiFi and Serial.
 * No parsing, no protocol handling, no logic - just pure data passthrough.
 * 
 * The Arduino is the smart device that understands commands.
 * The server builds binary protocol frames and sends them as hex strings.
 * 
 * Hardware:
 * - NodeMCU D1 (GPIO5) → Arduino RX1
 * - NodeMCU D2 (GPIO4) → Arduino TX1  
 * - GND → GND
 * 
 * Serial1 (57600 baud) connects to Arduino
 * USB Serial (115200 baud) for debugging
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";
const char* serverUrl = "https://6fbx0j5c-5001.inc1.devtunnels.ms";

// Hardware serial to Arduino
// D1 (GPIO5) = TX to Arduino RX
// D2 (GPIO4) = RX from Arduino TX
#define ARDUINO_TX_PIN 5  // D1 - Our TX, Arduino RX
#define ARDUINO_RX_PIN 4  // D2 - Our RX, Arduino TX

SoftwareSerial arduinoSerial(ARDUINO_RX_PIN, ARDUINO_TX_PIN);  // RX, TX
WiFiClientSecure wifiClient;

// Stats
unsigned long lastPoll = 0;
uint32_t bytesSentToArduino = 0;
uint32_t bytesReceivedFromArduino = 0;

// Buffer for hex-to-binary conversion
uint8_t binaryBuffer[2048];

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n╔═══════════════════════════════════════╗");
    Serial.println("║   ESP8266 Transparent Serial Bridge  ║");
    Serial.println("║   WiFi ↔ Arduino (Dumb Pipe Mode)    ║");
    Serial.println("╚═══════════════════════════════════════╝\n");
    
    // Start Arduino serial
    arduinoSerial.begin(57600);
    
    // Connect WiFi
    wifiClient.setInsecure();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("📡 WiFi: ");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(300);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ✅");
        Serial.print("   IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("   Server: ");
        Serial.println(serverUrl);
    } else {
        Serial.println(" ❌ FAILED");
        Serial.println("Restarting in 5s...");
        delay(5000);
        ESP.restart();
    }
    
    Serial.println("\n🔄 Bridge active - forwarding all data\n");
}

// Convert hex string to binary
// "AA0100" → {0xAA, 0x01, 0x00}
uint16_t hexToBinary(const String& hex, uint8_t* output, uint16_t maxLen) {
    uint16_t len = hex.length() / 2;
    if (len > maxLen) len = maxLen;
    
    for (uint16_t i = 0; i < len; i++) {
        String byteStr = hex.substring(i * 2, i * 2 + 2);
        output[i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
    }
    
    return len;
}

// Convert binary to hex string
String binaryToHex(const uint8_t* data, uint16_t len) {
    String result = "";
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] < 0x10) result += "0";
        result += String(data[i], HEX);
    }
    result.toUpperCase();
    return result;
}

void pollServer() {
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/bridge/get";
    http.begin(wifiClient, url);
    http.setTimeout(3000);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        payload.trim();
        
        if (payload.length() > 0) {
            Serial.print("⬇️  Server→Arduino: ");
            Serial.print(payload.length() / 2);
            Serial.println(" bytes");
            
            // Convert hex string to binary and send to Arduino
            uint16_t len = hexToBinary(payload, binaryBuffer, sizeof(binaryBuffer));
            if (len > 0) {
                arduinoSerial.write(binaryBuffer, len);
                arduinoSerial.flush();
                bytesSentToArduino += len;
                
                Serial.print("   Raw: ");
                Serial.println(payload);
            }
        }
    } else if (httpCode == 204) {
        // No data - this is normal, stay silent
    } else if (httpCode > 0) {
        Serial.print("⚠️  HTTP ");
        Serial.println(httpCode);
    }
    
    http.end();
}

void sendArduinoDataToServer(const uint8_t* data, uint16_t len) {
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/bridge/post";
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "text/plain");
    http.setTimeout(3000);
    
    String hexData = binaryToHex(data, len);
    int httpCode = http.POST(hexData);
    
    if (httpCode > 0) {
        Serial.print("⬆️  Arduino→Server: ");
        Serial.print(len);
        Serial.print(" bytes (HTTP ");
        Serial.print(httpCode);
        Serial.println(")");
    }
    
    http.end();
}

void loop() {
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi lost!");
        WiFi.reconnect();
        delay(1000);
        return;
    }
    
    // Forward Arduino → Server (any data Arduino sends)
    if (arduinoSerial.available()) {
        uint16_t len = 0;
        while (arduinoSerial.available() && len < sizeof(binaryBuffer)) {
            binaryBuffer[len++] = arduinoSerial.read();
            delay(1);  // Small delay for bytes to arrive
        }
        
        if (len > 0) {
            bytesReceivedFromArduino += len;
            sendArduinoDataToServer(binaryBuffer, len);
        }
    }
    
    // Poll server every 500ms for commands
    if (millis() - lastPoll > 500) {
        pollServer();
        lastPoll = millis();
        
        // Stats every 20 seconds
        static unsigned long lastStats = 0;
        if (millis() - lastStats > 20000) {
            lastStats = millis();
            Serial.print("📊 Forwarded: ");
            Serial.print(bytesSentToArduino);
            Serial.print("↓ ");
            Serial.print(bytesReceivedFromArduino);
            Serial.println("↑ bytes");
        }
    }
    
    yield();
}
