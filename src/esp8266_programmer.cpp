/*
 * ESP8266 WiFi-to-Serial Bridge for Arduino Configuration
 * 
 * Receives JSON configurations from server and forwards to Arduino
 * via binary protocol
 * 
 * Hardware Connections:
 * - NodeMCU D1 (GPIO5) → Arduino RX0 (Serial1 RX)
 * - NodeMCU D2 (GPIO4) → Arduino TX1 (Serial1 TX)
 * - Common GND between NodeMCU and Arduino
 * 
 * Data Flow:
 * - Server (JSON) → HTTP POST → ESP8266 → Binary Protocol → Arduino
 * - Arduino → Binary Protocol → ESP8266 → HTTP Response → Server
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Include protocol definitions (shared with Arduino)
// We need to prefix error codes to avoid lwip conflicts
#define PROTO_START_BYTE 0xAA
#define PROTO_MAX_PAYLOAD 1024

enum CommandID : uint8_t {
    CMD_PING = 0x01,
    CMD_PONG = 0x02,
    CMD_ERROR = 0x03,
    CMD_ACK = 0x04,
    CMD_SENSOR_CONFIG = 0x22,  // Configuration deployment
    CMD_DATA_STRING = 0x54,    // String data
    CMD_SENSOR_READ = 0x20,    // State request
};

// WiFi credentials
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// Server settings
const char* serverUrl = "https://6fbx0j5c-5001.inc1.devtunnels.ms";

// Hardware connections
#define ARDUINO_RX_PIN 5  // D1 (GPIO5) - connects to Arduino RX
#define ARDUINO_TX_PIN 4  // D2 (GPIO4) - connects to Arduino TX

// Software serial for Arduino communication (transparent passthrough)
SoftwareSerial arduinoSerial(ARDUINO_TX_PIN, ARDUINO_RX_PIN);  // RX, TX

// WiFi client
WiFiClientSecure wifiClient;

// Response buffer for Arduino data
String responseBuffer = "";
unsigned long lastResponseTime = 0;

// Stats
unsigned long lastPoll = 0;
uint32_t bytesSent = 0;
uint32_t bytesReceived = 0;

// CRC-16 calculation
uint16_t calculateCRC16(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

// Send binary protocol frame to Arduino
bool sendProtocolFrame(uint8_t commandId, const uint8_t* payload, uint16_t length) {
    if (length > PROTO_MAX_PAYLOAD) {
        Serial.println("❌ Payload too large!");
        return false;
    }
    
    // Build CRC data: CMD + LEN_H + LEN_L + PAYLOAD
    uint8_t crcBuffer[3 + PROTO_MAX_PAYLOAD];
    crcBuffer[0] = commandId;
    crcBuffer[1] = (length >> 8) & 0xFF;
    crcBuffer[2] = length & 0xFF;
    if (length > 0) {
        memcpy(crcBuffer + 3, payload, length);
    }
    
    uint16_t crc = calculateCRC16(crcBuffer, 3 + length);
    
    // Send frame to Arduino
    arduinoSerial.write(PROTO_START_BYTE);
    arduinoSerial.write(commandId);
    arduinoSerial.write((length >> 8) & 0xFF);
    arduinoSerial.write(length & 0xFF);
    if (length > 0) {
        arduinoSerial.write(payload, length);
    }
    arduinoSerial.write((crc >> 8) & 0xFF);
    arduinoSerial.write(crc & 0xFF);
    arduinoSerial.flush();
    
    return true;
}

void setup() {
    // USB serial for debug
    Serial.begin(115200);
    delay(500);
    
    // Arduino serial for binary protocol
    arduinoSerial.begin(57600);
    
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║  ESP8266 Configuration Bridge v3      ║");
    Serial.println("╚════════════════════════════════════════╝");
    
#ifdef NODEMCU_V3
    Serial.println("Hardware: NodeMCU v3");
#else
    Serial.println("Hardware: NodeMCU v2");
#endif
    
    Serial.println("Mode: JSON to Binary Protocol");
    Serial.print("Serial: ");
    Serial.print(ARDUINO_TX_PIN);
    Serial.print("(RX) / ");
    Serial.print(ARDUINO_RX_PIN);
    Serial.println("(TX) @ 57600 baud");
    
    // Disable SSL verification
    wifiClient.setInsecure();
    
    // Better WiFi stability settings
    WiFi.persistent(false);  // Don't save WiFi config to flash
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    
    // Connect to WiFi
    Serial.print("📡 Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        // Force reconnect if stuck
        if (attempts == 15) {
            Serial.print("\n   Retrying... ");
            WiFi.disconnect();
            delay(100);
            WiFi.begin(ssid, password);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("   IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("   Signal: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("   Server: ");
        Serial.println(serverUrl);
        
        // Ping Arduino
        Serial.println("\n🔌 Testing Arduino connection...");
        bool pingSuccess = sendProtocolFrame(CMD_PING, nullptr, 0);
        if (pingSuccess) {
            Serial.println("   ✅ Ping sent to Arduino");
        } else {
            Serial.println("   ⚠️  Could not ping Arduino");
        }
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Serial.println("   Restarting in 5 seconds...");
        delay(5000);
        ESP.restart();
        return;
    }
    
    Serial.println("\n🔄 Bridge active. Polling server...\n");
}

void pollServerForCommand() {
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/command/get";
    http.begin(wifiClient, url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        if (payload.length() > 0) {
            Serial.print("📥 Server → ESP (");
            Serial.print(payload.length());
            Serial.println(" bytes)");
            
            // Parse JSON
            StaticJsonDocument<4096> doc;
            DeserializationError error = deserializeJson(doc, payload);
            
            if (error) {
                Serial.print("❌ JSON Parse Error: ");
                Serial.println(error.c_str());
                http.end();
                return;
            }
            
            // Check deployment type
            const char* type = doc["type"];
            
            if (type && strcmp(type, "deploy") == 0) {
                Serial.println("   Type: DEPLOYMENT");
                
                // Extract the config object
                JsonObject config = doc["config"];
                
                // Serialize config back to JSON string for Arduino
                String configJson;
                serializeJson(config, configJson);
                
                Serial.print("   Config size: ");
                Serial.print(configJson.length());
                Serial.println(" bytes");
                
                // Send as binary protocol frame
                bool success = sendProtocolFrame(
                    CMD_SENSOR_CONFIG, 
                    (uint8_t*)configJson.c_str(), 
                    configJson.length()
                );
                
                if (success) {
                    Serial.println("   ✅ Sent to Arduino via binary protocol");
                    bytesSent += configJson.length();
                } else {
                    Serial.println("   ❌ Failed to send to Arduino");
                }
            } else {
                Serial.println("   ⚠️  Unknown command type");
            }
        }
    }
    else if (httpCode == 204) {
        // No command available (silent)
    }
    else if (httpCode > 0) {
        Serial.print("⚠️  HTTP ");
        Serial.println(httpCode);
    }
    
    http.end();
}

void sendResponseToServer() {
    if (responseBuffer.length() == 0) return;
    
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/response";
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);
    
    // Wrap response in JSON
    StaticJsonDocument<2048> doc;
    doc["source"] = "arduino";
    doc["data"] = responseBuffer;
    doc["timestamp"] = millis();
    
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    
    int httpCode = http.POST(jsonOutput);
    
    if (httpCode == 200) {
        Serial.print("📤 ESP → Server (");
        Serial.print(responseBuffer.length());
        Serial.println(" bytes)");
        Serial.println("   ✅ Sent to server");
    } else {
        Serial.print("⚠️  Failed to send response: HTTP ");
        Serial.println(httpCode);
    }
    
    responseBuffer = "";
    http.end();
}

void loop() {
    // Check WiFi with auto-reconnect
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 5000) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("❌ WiFi lost! Reconnecting...");
            Serial.print("   Signal was: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            
            WiFi.disconnect();
            delay(100);
            WiFi.begin(ssid, password);
            
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20) {
                delay(500);
                Serial.print(".");
                attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n✅ WiFi reconnected!");
                Serial.print("   IP: ");
                Serial.println(WiFi.localIP());
                Serial.print("   Signal: ");
                Serial.print(WiFi.RSSI());
                Serial.println(" dBm");
            } else {
                Serial.println("\n❌ Reconnect failed! Restarting ESP...");
                delay(2000);
                ESP.restart();
            }
        }
        lastWifiCheck = millis();
    }
    
    // Read responses from Arduino (binary protocol frames)
    // For simplicity, we'll just forward any text responses
    while (arduinoSerial.available()) {
        char c = arduinoSerial.read();
        responseBuffer += c;
        bytesReceived++;
        lastResponseTime = millis();
    }
    
    // If we have data and it's been quiet for 100ms, send it
    if (responseBuffer.length() > 0 && (millis() - lastResponseTime > 100)) {
        sendResponseToServer();
    }
    
    // Poll server every 2 seconds
    if (millis() - lastPoll > 2000) {
        pollServerForCommand();
        lastPoll = millis();
        
        // Print stats every 60 seconds
        static unsigned long lastStats = 0;
        if (millis() - lastStats > 60000) {
            Serial.println("\n╔════════════════════════════════════════╗");
            Serial.println("║  Bridge Status Report                 ║");
            Serial.println("╚════════════════════════════════════════╝");
            Serial.print("Uptime: ");
            Serial.print(millis() / 1000);
            Serial.println(" seconds");
            Serial.print("WiFi Signal: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.print("Data: ");
            Serial.print(bytesSent);
            Serial.print(" sent → Arduino, ");
            Serial.print(bytesReceived);
            Serial.println(" received ← Arduino");
            Serial.print("Free Heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");
            Serial.println();
            lastStats = millis();
        }
    }
    
    yield();  // Feed watchdog
    delay(10);
}
