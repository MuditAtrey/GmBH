/*
 * ESP8266 WiFi-to-Serial Bridge (Binary Protocol)
 * 
 * Hardware Connections:
 * - NodeMCU D1 (GPIO5) → Arduino RX (hardware serial)
 * - NodeMCU D2 (GPIO4) → Arduino TX (hardware serial)
 * - Common GND between NodeMCU and Arduino
 * 
 * USB Serial (115200) = Debug output to computer
 * Software Serial (57600) = Binary protocol to Arduino
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SoftwareSerial.h>
#include "ArduinoProtocol.h"

// WiFi credentials
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// Server settings
const char* serverUrl = "https://6fbx0j5c-5001.inc1.devtunnels.ms";

// Hardware connections
// D1 (GPIO5) = TX to Arduino RX
// D2 (GPIO4) = RX from Arduino TX
#define ARDUINO_RX_PIN 5  // D1 - connects to Arduino RX
#define ARDUINO_TX_PIN 4  // D2 - connects to Arduino TX

// Software serial for Arduino communication
SoftwareSerial arduinoSerial(ARDUINO_TX_PIN, ARDUINO_RX_PIN);  // RX, TX

// Protocol handler for Arduino communication
ProtocolHandler protocol(&arduinoSerial);

// WiFi client
WiFiClientSecure wifiClient;

// Stats
unsigned long lastPoll = 0;
unsigned long lastArduinoResponse = 0;
uint32_t commandsSent = 0;
uint32_t responsesReceived = 0;

void setup() {
    // USB serial for debug
    Serial.begin(115200);
    delay(500);
    
    // Arduino serial for protocol
    arduinoSerial.begin(57600);
    
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║  ESP8266 Binary Protocol Bridge       ║");
    Serial.println("╚════════════════════════════════════════╝");
    
    // Disable SSL verification
    wifiClient.setInsecure();
    
    // Connect to WiFi
    Serial.print("📡 Connecting to WiFi: ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi Connected!");
        Serial.print("   IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("   Server: ");
        Serial.println(serverUrl);
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        delay(1000);
        ESP.restart();
        return;
    }
    
    // Wait for Arduino to boot
    Serial.println("⏳ Waiting for Arduino...");
    delay(2000);
    
    // Send ping to Arduino
    Serial.println("🔌 Pinging Arduino...");
    protocol.sendCommand(CMD_PING);
    
    unsigned long pingStart = millis();
    ProtocolFrame response;
    bool gotPong = false;
    
    while (millis() - pingStart < 3000) {
        if (protocol.receiveFrame(response)) {
            if (response.commandId == CMD_PONG) {
                Serial.println("✅ Arduino is ready!");
                gotPong = true;
                lastArduinoResponse = millis();
                break;
            }
        }
        yield();
    }
    
    if (!gotPong) {
        Serial.println("⚠️  No response from Arduino (check wiring!)");
    }
    
    Serial.println("\n🔄 Bridge active. Polling for commands...\n");
}

void handleArduinoResponse(const ProtocolFrame& frame) {
    lastArduinoResponse = millis();
    responsesReceived++;
    
    Serial.print("⬅️  Arduino response: CMD=0x");
    Serial.print(frame.commandId, HEX);
    Serial.print(" LEN=");
    Serial.print(frame.length);
    
    // Parse response based on command type
    if (frame.commandId == CMD_PONG) {
        Serial.println(" [PONG]");
    }
    else if (frame.commandId == CMD_ACK) {
        Serial.println(" [ACK]");
    }
    else if (frame.commandId == CMD_ERROR) {
        Serial.print(" [ERROR=");
        if (frame.length > 0) {
            Serial.print(frame.payload[0], HEX);
        }
        Serial.println("]");
    }
    else if (frame.commandId == CMD_SENSOR_DATA) {
        // Parse sensor data example
        PayloadParser parser(frame.payload, frame.length);
        uint8_t sensorId;
        int16_t value;
        if (parser.readUint8(sensorId) && parser.readInt16(value)) {
            Serial.print(" [SENSOR ");
            Serial.print(sensorId);
            Serial.print("=");
            Serial.print(value);
            Serial.println("]");
        }
    }
    else if (frame.commandId == CMD_ENCODER_DATA) {
        // Parse encoder data
        PayloadParser parser(frame.payload, frame.length);
        int16_t position;
        uint8_t velocity;
        uint8_t buttonState;
        
        if (parser.readInt16(position)) {
            Serial.print(" [POS=");
            Serial.print(position);
            
            if (parser.readUint8(velocity)) {
                Serial.print(" VEL=");
                Serial.print((int8_t)velocity);
            }
            
            if (parser.readUint8(buttonState)) {
                Serial.print(" BTN=");
                Serial.print(buttonState ? "PRESS" : "RELEASE");
            }
            
            Serial.println("]");
        }
    }
    else {
        Serial.println();
    }
}

void pollServerForCommand() {
    HTTPClient http;
    
    String url = String(serverUrl) + "/api/command/get";
    http.begin(wifiClient, url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        
        // Server returns binary protocol data as hex string or base64
        // For now, parse simple JSON commands and convert to binary
        
        if (payload.length() > 10 && payload.indexOf("{") >= 0) {
            // Parse JSON command from server
            String cmd = payload;
            cmd.trim();
            
            Serial.print("📨 Server command: ");
            Serial.println(cmd);
            
            // Convert to binary protocol
            uint8_t framePayload[256];
            PayloadBuilder builder(framePayload, sizeof(framePayload));
            uint8_t cmdId = CMD_PING;
            
            // Parse command type
            if (cmd.indexOf("\"type\":\"led_blink\"") > 0) {
                cmdId = CMD_LED_BLINK;
                
                // Extract duration parameter
                int durStart = cmd.indexOf("\"duration\":") + 11;
                int durEnd = cmd.indexOf(",", durStart);
                if (durEnd == -1) durEnd = cmd.indexOf("}", durStart);
                String durStr = cmd.substring(durStart, durEnd);
                durStr.trim();
                uint16_t duration = durStr.toInt();
                
                builder.addUint16(duration);
                
                Serial.print("   → LED_BLINK duration=");
                Serial.println(duration);
            }
            else if (cmd.indexOf("\"type\":\"led_set\"") > 0) {
                cmdId = CMD_LED_SET;
                
                // Extract state parameter
                int stateStart = cmd.indexOf("\"state\":") + 8;
                String stateStr = cmd.substring(stateStart, stateStart + 10);
                stateStr.trim();
                uint8_t state = (stateStr.indexOf("true") >= 0) ? 1 : 0;
                
                builder.addUint8(state);
                Serial.print("   → LED_SET state=");
                Serial.println(state ? "ON" : "OFF");
            }
            else if (cmd.indexOf("\"type\":\"led_on\"") > 0) {
                cmdId = CMD_LED_SET;
                builder.addUint8(1);  // state = ON
                Serial.println("   → LED_ON");
            }
            else if (cmd.indexOf("\"type\":\"led_off\"") > 0) {
                cmdId = CMD_LED_SET;
                builder.addUint8(0);  // state = OFF
                Serial.println("   → LED_OFF");
            }
            else if (cmd.indexOf("\"type\":\"ping\"") > 0) {
                cmdId = CMD_PING;
                Serial.println("   → PING");
            }
            else if (cmd.indexOf("\"type\":\"oled_text\"") > 0) {
                cmdId = CMD_OLED_TEXT;
                
                // Extract text, x, y
                int textStart = cmd.indexOf("\"text\":\"") + 8;
                int textEnd = cmd.indexOf("\"", textStart);
                String text = cmd.substring(textStart, textEnd);
                
                int xStart = cmd.indexOf("\"x\":") + 4;
                int xEnd = cmd.indexOf(",", xStart);
                uint8_t x = cmd.substring(xStart, xEnd).toInt();
                
                int yStart = cmd.indexOf("\"y\":") + 4;
                int yEnd = cmd.indexOf("}", yStart);
                if (yEnd == -1) yEnd = cmd.indexOf(",", yStart);
                uint8_t y = cmd.substring(yStart, yEnd).toInt();
                
                builder.addUint8(x);
                builder.addUint8(y);
                builder.addString(text.c_str());
                
                Serial.print("   → OLED_TEXT x=");
                Serial.print(x);
                Serial.print(" y=");
                Serial.print(y);
                Serial.print(" text='");
                Serial.print(text);
                Serial.println("'");
            }
            else if (cmd.indexOf("\"type\":\"encoder_read\"") > 0) {
                cmdId = CMD_ENCODER_READ;
                Serial.println("   → ENCODER_READ");
            }
            else {
                Serial.println("   ⚠️  Unknown command type");
                http.end();
                return;
            }
            
            // Send binary frame to Arduino
            bool sent = protocol.sendFrame(cmdId, framePayload, builder.size());
            if (sent) {
                commandsSent++;
                Serial.println("✅ Sent to Arduino");
            } else {
                Serial.println("❌ Failed to send");
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

void loop() {
    // Check WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi lost! Reconnecting...");
        WiFi.reconnect();
        delay(1000);
        return;
    }
    
    // Handle Arduino responses
    ProtocolFrame response;
    if (protocol.receiveFrame(response)) {
        handleArduinoResponse(response);
    }
    
    // Poll server every 2 seconds
    if (millis() - lastPoll > 2000) {
        pollServerForCommand();
        lastPoll = millis();
        
        // Print stats every 10 polls
        static int pollCount = 0;
        if (++pollCount >= 10) {
            pollCount = 0;
            Serial.print("📊 Stats: ");
            Serial.print(commandsSent);
            Serial.print(" sent, ");
            Serial.print(responsesReceived);
            Serial.print(" received, ");
            Serial.print((millis() - lastArduinoResponse) / 1000);
            Serial.println("s since last response");
        }
    }
    
    yield();
}
