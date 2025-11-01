/*
 * Arduino R4 Minima - Binary Protocol Receiver
 * 
 * Receives binary protocol commands via hardware serial and controls peripherals
 * 
 * Hardware Connections:
 * - Arduino RX0 ← ESP8266 D1 (GPIO5) TX
 * - Arduino TX1 → ESP8266 D2 (GPIO4) RX
 * - Common GND
 * 
 * Example Peripherals (optional, for demonstration):
 * - Rotary Encoder: CLK=2, DT=3, SW=4
 * - OLED Display: SDA=A4, SCL=A5 (I2C)
 * - LED: Built-in LED_BUILTIN
 */

#include <Arduino.h>
#include "ArduinoProtocol.h"

// Protocol handler (uses Serial1 for ESP8266, Serial for debug)
// Arduino R4 Minima has two UARTs:
// - Serial (USB) for debugging
// - Serial1 (RX0/TX1 pins) for ESP8266 communication
ProtocolHandler protocol(&Serial1);

// LED state
#define LED_PIN LED_BUILTIN
struct {
    bool blinking;
    uint16_t duration;
    unsigned long lastToggle;
    bool state;
} ledState = {false, 500, 0, false};

// Simulated rotary encoder (replace with real encoder library if needed)
struct {
    int16_t position;
    int8_t velocity;
    bool buttonPressed;
    unsigned long lastUpdate;
} encoder = {0, 0, false, 0};

// OLED simulation (replace with real library like Adafruit_SSD1306)
struct {
    bool initialized;
    char lastText[64];
    uint8_t lastX;
    uint8_t lastY;
} oled = {false, "", 0, 0};

void setup() {
    // USB serial for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 5000); // Wait for Serial or timeout after 5s
    
    Serial.println("=== ARDUINO R4 STARTING ===");
    Serial.println("Build time: " __DATE__ " " __TIME__);
    
    // Hardware serial for ESP8266 at 57600 (must match ESP8266)
    Serial1.begin(57600);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // Turn on LED to show we're alive
    delay(500);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("Arduino R4 Binary Protocol Ready");
    Serial.println("Waiting for ESP8266...");
    
    delay(500);
}

void handleLedSet(const ProtocolFrame& frame) {
    PayloadParser parser(frame.payload, frame.length);
    uint8_t state;
    
    if (parser.readUint8(state)) {
        ledState.blinking = false;
        digitalWrite(LED_PIN, state ? HIGH : LOW);
        ledState.state = state;
        
        protocol.sendAck();
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleLedBlink(const ProtocolFrame& frame) {
    PayloadParser parser(frame.payload, frame.length);
    uint16_t duration;
    
    if (parser.readUint16(duration)) {
        if (duration >= 50 && duration <= 5000) {
            ledState.blinking = true;
            ledState.duration = duration;
            ledState.lastToggle = millis();
            ledState.state = true;
            digitalWrite(LED_PIN, HIGH);
            
            protocol.sendAck();
        } else {
            protocol.sendError(PROTO_ERR_INVALID_PARAM);
        }
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleEncoderRead(const ProtocolFrame& frame) {
    // Read simulated encoder (in real app, read from hardware)
    // For demo, we'll just send current position
    
    uint8_t payload[4];
    PayloadBuilder builder(payload, sizeof(payload));
    
    builder.addInt16(encoder.position);
    builder.addUint8((uint8_t)encoder.velocity);
    builder.addUint8(encoder.buttonPressed ? 1 : 0);
    
    protocol.sendFrame(CMD_ENCODER_DATA, payload, builder.size());
}

void handleOledText(const ProtocolFrame& frame) {
    PayloadParser parser(frame.payload, frame.length);
    uint8_t x, y;
    char text[64];
    
    if (parser.readUint8(x) && parser.readUint8(y) && parser.readString(text, sizeof(text))) {
        // In real implementation, would call display.setCursor(x, y); display.print(text);
        oled.lastX = x;
        oled.lastY = y;
        strncpy(oled.lastText, text, sizeof(oled.lastText) - 1);
        oled.initialized = true;
        
        protocol.sendAck();
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleOledClear(const ProtocolFrame& frame) {
    // In real implementation: display.clearDisplay(); display.display();
    oled.lastText[0] = '\0';
    protocol.sendAck();
}

void processCommand(const ProtocolFrame& frame) {
    Serial.print("CMD: 0x");
    Serial.println(frame.commandId, HEX);
    
    switch (frame.commandId) {
        case CMD_PING:
            Serial.println("→ PONG");
            protocol.sendCommand(CMD_PONG);
            break;
            
        case CMD_LED_SET:
            Serial.println("→ LED_SET");
            handleLedSet(frame);
            break;
            
        case CMD_LED_BLINK:
            Serial.println("→ LED_BLINK");
            handleLedBlink(frame);
            break;
            
        case CMD_ENCODER_READ:
            Serial.println("→ ENCODER_READ");
            handleEncoderRead(frame);
            break;
            
        case CMD_OLED_TEXT:
            Serial.println("→ OLED_TEXT");
            handleOledText(frame);
            break;
            
        case CMD_OLED_CLEAR:
            Serial.println("→ OLED_CLEAR");
            handleOledClear(frame);
            break;
            
        default:
            Serial.println("→ ERROR: Unknown command");
            protocol.sendError(PROTO_ERR_INVALID_CMD);
            break;
    }
}

void updateLedBlink() {
    if (!ledState.blinking) return;
    
    unsigned long now = millis();
    if (now - ledState.lastToggle >= ledState.duration) {
        ledState.state = !ledState.state;
        digitalWrite(LED_PIN, ledState.state ? HIGH : LOW);
        ledState.lastToggle = now;
    }
}

void updateSimulatedEncoder() {
    // Simulate encoder changes (for testing without hardware)
    // In real app, this would read actual encoder pins
    unsigned long now = millis();
    if (now - encoder.lastUpdate > 1000) {
        encoder.position += random(-5, 6);
        encoder.velocity = random(-3, 4);
        encoder.lastUpdate = now;
    }
}

void loop() {
    static unsigned long lastDebug = 0;
    
    // Debug heartbeat every 2 seconds
    if (millis() - lastDebug > 2000) {
        Serial.print(".");
        lastDebug = millis();
    }
    
    // Receive and process protocol frames
    ProtocolFrame frame;
    if (protocol.receiveFrame(frame)) {
        processCommand(frame);
    }
    
    // Update LED blink task
    updateLedBlink();
    
    // Update simulated encoder
    updateSimulatedEncoder();
    
    // Small delay to prevent tight loop
    delay(1);
}