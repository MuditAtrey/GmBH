/*
 * Arduino R4 Minima - Programmable Device via Binary Protocol
 * 
 * This Arduino contains ALL the actual code for peripherals.
 * The web server just sends data and the Arduino decides what to do with it.
 * 
 * Hardware:
 * - Serial1 (RX0/TX1) connects to ESP8266 at 57600 baud
 * - Serial (USB) for debugging at 115200 baud
 * - Built-in LED on LED_BUILTIN
 * - Optional: OLED on I2C (SDA=A4, SCL=A5)
 * - Optional: Rotary encoder (CLK=2, DT=3, SW=4)
 * - Optional: Servo on pin 9
 * 
 * Protocol: Same binary protocol, but Arduino has the smarts
 */

#include <Arduino.h>
#include "ArduinoProtocol.h"

// Serial connections
#define DEBUG_SERIAL Serial      // USB for debugging
#define ESP_SERIAL   Serial1     // Hardware serial to ESP8266

ProtocolHandler protocol(&ESP_SERIAL);

// ============================================================================
// PERIPHERAL STATE - Add your own peripherals here!
// ============================================================================

// LED state
struct {
    bool blinking;
    uint16_t interval;
    unsigned long lastToggle;
    bool state;
} led = {false, 500, 0, false};

// Rotary encoder (simulated for now - add real encoder library if needed)
struct {
    int16_t position;
    int8_t velocity;
    bool buttonPressed;
    unsigned long lastUpdate;
    bool enabled;
} encoder = {0, 0, false, 0, false};

// Servo (add Servo.h library if needed)
struct {
    uint8_t pin;
    uint16_t position;  // 0-180 degrees
    bool attached;
} servo = {9, 90, false};

// OLED display (add display library if needed)
struct {
    char textBuffer[128];
    uint8_t cursorX;
    uint8_t cursorY;
    bool initialized;
} oled = {""  , 0, 0, false};

// Sensor readings
struct {
    int16_t values[8];  // Up to 8 analog sensors
    unsigned long lastRead;
} sensors = {{0}, 0};

// ============================================================================
// COMMAND HANDLERS - The actual functionality lives here!
// ============================================================================

void handlePing(const ProtocolFrame& frame) {
    DEBUG_SERIAL.println("→ PING received, sending PONG");
    protocol.sendCommand(CMD_PONG);
}

void handleLedSet(const ProtocolFrame& frame) {
    PayloadParser parser(frame.payload, frame.length);
    uint8_t state;
    
    if (parser.readUint8(state)) {
        led.blinking = false;
        led.state = (state != 0);
        digitalWrite(LED_BUILTIN, led.state ? HIGH : LOW);
        
        DEBUG_SERIAL.print("→ LED ");
        DEBUG_SERIAL.println(led.state ? "ON" : "OFF");
        
        protocol.sendAck();
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleLedBlink(const ProtocolFrame& frame) {
    PayloadParser parser(frame.payload, frame.length);
    uint16_t interval;
    
    if (parser.readUint16(interval)) {
        if (interval >= 50 && interval <= 5000) {
            led.blinking = true;
            led.interval = interval;
            led.lastToggle = millis();
            led.state = true;
            digitalWrite(LED_BUILTIN, HIGH);
            
            DEBUG_SERIAL.print("→ LED BLINK ");
            DEBUG_SERIAL.print(interval);
            DEBUG_SERIAL.println("ms");
            
            protocol.sendAck();
        } else {
            DEBUG_SERIAL.println("→ LED BLINK: invalid interval");
            protocol.sendError(PROTO_ERR_INVALID_PARAM);
        }
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleEncoderConfig(const ProtocolFrame& frame) {
    // Configure encoder: [enabled:1, pinCLK:1, pinDT:1, pinSW:1]
    PayloadParser parser(frame.payload, frame.length);
    uint8_t enabled;
    
    if (parser.readUint8(enabled)) {
        encoder.enabled = (enabled != 0);
        encoder.position = 0;
        encoder.velocity = 0;
        
        DEBUG_SERIAL.print("→ ENCODER ");
        DEBUG_SERIAL.println(encoder.enabled ? "ENABLED" : "DISABLED");
        
        protocol.sendAck();
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleEncoderRead(const ProtocolFrame& frame) {
    // Send back encoder data
    uint8_t payload[4];
    PayloadBuilder builder(payload, sizeof(payload));
    
    builder.addInt16(encoder.position);
    builder.addUint8((uint8_t)encoder.velocity);
    builder.addUint8(encoder.buttonPressed ? 1 : 0);
    
    protocol.sendFrame(CMD_ENCODER_DATA, payload, builder.size());
    
    DEBUG_SERIAL.print("→ ENCODER DATA: pos=");
    DEBUG_SERIAL.print(encoder.position);
    DEBUG_SERIAL.print(" vel=");
    DEBUG_SERIAL.print(encoder.velocity);
    DEBUG_SERIAL.print(" btn=");
    DEBUG_SERIAL.println(encoder.buttonPressed);
}

void handleServoControl(const ProtocolFrame& frame) {
    // Servo control: [pin:1, angle:2]
    PayloadParser parser(frame.payload, frame.length);
    uint8_t pin;
    uint16_t angle;
    
    if (parser.readUint8(pin) && parser.readUint16(angle)) {
        if (angle <= 180) {
            servo.pin = pin;
            servo.position = angle;
            
            // Add actual servo library code here:
            // if (!servo.attached) {
            //     servoObj.attach(pin);
            //     servo.attached = true;
            // }
            // servoObj.write(angle);
            
            DEBUG_SERIAL.print("→ SERVO pin=");
            DEBUG_SERIAL.print(pin);
            DEBUG_SERIAL.print(" angle=");
            DEBUG_SERIAL.println(angle);
            
            protocol.sendAck();
        } else {
            protocol.sendError(PROTO_ERR_INVALID_PARAM);
        }
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleOledText(const ProtocolFrame& frame) {
    // OLED text: [x:1, y:1, text:string]
    PayloadParser parser(frame.payload, frame.length);
    uint8_t x, y;
    char text[64];
    
    if (parser.readUint8(x) && parser.readUint8(y) && parser.readString(text, sizeof(text))) {
        oled.cursorX = x;
        oled.cursorY = y;
        strncpy(oled.textBuffer, text, sizeof(oled.textBuffer) - 1);
        
        // Add actual OLED library code here:
        // display.setCursor(x, y);
        // display.print(text);
        // display.display();
        
        DEBUG_SERIAL.print("→ OLED (");
        DEBUG_SERIAL.print(x);
        DEBUG_SERIAL.print(",");
        DEBUG_SERIAL.print(y);
        DEBUG_SERIAL.print("): ");
        DEBUG_SERIAL.println(text);
        
        protocol.sendAck();
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleOledClear(const ProtocolFrame& frame) {
    oled.textBuffer[0] = '\0';
    
    // Add actual OLED library code:
    // display.clearDisplay();
    // display.display();
    
    DEBUG_SERIAL.println("→ OLED CLEAR");
    protocol.sendAck();
}

void handleSensorConfig(const ProtocolFrame& frame) {
    // Sensor config: [sensorId:1, pin:1, type:1]
    PayloadParser parser(frame.payload, frame.length);
    uint8_t sensorId, pin, type;
    
    if (parser.readUint8(sensorId) && parser.readUint8(pin) && parser.readUint8(type)) {
        if (sensorId < 8) {
            // Configure analog pin
            pinMode(pin, INPUT);
            
            DEBUG_SERIAL.print("→ SENSOR ");
            DEBUG_SERIAL.print(sensorId);
            DEBUG_SERIAL.print(" on pin ");
            DEBUG_SERIAL.println(pin);
            
            protocol.sendAck();
        } else {
            protocol.sendError(PROTO_ERR_INVALID_PARAM);
        }
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleSensorRead(const ProtocolFrame& frame) {
    // Read sensor: [sensorId:1]
    PayloadParser parser(frame.payload, frame.length);
    uint8_t sensorId;
    
    if (parser.readUint8(sensorId) && sensorId < 8) {
        // Read analog value (you would read from configured pin in real implementation)
        int16_t value = analogRead(A0 + sensorId);
        sensors.values[sensorId] = value;
        
        // Send response
        uint8_t payload[3];
        PayloadBuilder builder(payload, sizeof(payload));
        builder.addUint8(sensorId);
        builder.addInt16(value);
        
        protocol.sendFrame(CMD_SENSOR_DATA, payload, builder.size());
        
        DEBUG_SERIAL.print("→ SENSOR ");
        DEBUG_SERIAL.print(sensorId);
        DEBUG_SERIAL.print(" = ");
        DEBUG_SERIAL.println(value);
    } else {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
    }
}

void handleGenericData(const ProtocolFrame& frame) {
    // Generic data handler - Arduino decides what to do with raw data
    // This makes the system EXTREMELY flexible!
    
    DEBUG_SERIAL.print("→ GENERIC DATA: ");
    DEBUG_SERIAL.print(frame.length);
    DEBUG_SERIAL.println(" bytes");
    
    // Example: First byte could be a sub-command
    if (frame.length > 0) {
        uint8_t subCmd = frame.payload[0];
        DEBUG_SERIAL.print("   SubCmd: 0x");
        DEBUG_SERIAL.println(subCmd, HEX);
        
        // You can add any custom logic here!
        // Parse the payload however you want
        // Call any function you want
        // This is where the "programmability" comes in!
    }
    
    protocol.sendAck();
}

// ============================================================================
// MAIN COMMAND DISPATCHER
// ============================================================================

void processCommand(const ProtocolFrame& frame) {
    DEBUG_SERIAL.print("📨 CMD: 0x");
    DEBUG_SERIAL.print(frame.commandId, HEX);
    DEBUG_SERIAL.print(" LEN: ");
    DEBUG_SERIAL.println(frame.length);
    
    switch (frame.commandId) {
        case CMD_PING:
            handlePing(frame);
            break;
            
        case CMD_LED_SET:
            handleLedSet(frame);
            break;
            
        case CMD_LED_BLINK:
            handleLedBlink(frame);
            break;
            
        case CMD_ENCODER_CONFIG:
            handleEncoderConfig(frame);
            break;
            
        case CMD_ENCODER_READ:
            handleEncoderRead(frame);
            break;
            
        case CMD_ENCODER_RESET:
            encoder.position = 0;
            encoder.velocity = 0;
            protocol.sendAck();
            break;
            
        case CMD_OLED_TEXT:
            handleOledText(frame);
            break;
            
        case CMD_OLED_CLEAR:
            handleOledClear(frame);
            break;
            
        case CMD_SENSOR_CONFIG:
            handleSensorConfig(frame);
            break;
            
        case CMD_SENSOR_READ:
            handleSensorRead(frame);
            break;
            
        case CMD_DATA_UINT8:
        case CMD_DATA_INT16:
        case CMD_DATA_INT32:
        case CMD_DATA_FLOAT:
        case CMD_DATA_STRING:
        case CMD_DATA_ARRAY:
            handleGenericData(frame);
            break;
            
        default:
            DEBUG_SERIAL.print("❌ Unknown command: 0x");
            DEBUG_SERIAL.println(frame.commandId, HEX);
            protocol.sendError(PROTO_ERR_INVALID_CMD);
            break;
    }
}

// ============================================================================
// BACKGROUND TASKS
// ============================================================================

void updateLedBlink() {
    if (!led.blinking) return;
    
    if (millis() - led.lastToggle >= led.interval) {
        led.state = !led.state;
        digitalWrite(LED_BUILTIN, led.state ? HIGH : LOW);
        led.lastToggle = millis();
    }
}

void updateEncoder() {
    if (!encoder.enabled) return;
    
    // Simulate encoder changes (replace with real encoder library)
    if (millis() - encoder.lastUpdate > 1000) {
        encoder.position += random(-3, 4);
        encoder.velocity = random(-2, 3);
        encoder.lastUpdate = millis();
    }
}

void updateSensors() {
    // Auto-read sensors every second (optional)
    if (millis() - sensors.lastRead > 1000) {
        sensors.lastRead = millis();
        // Could auto-report sensor changes here
    }
}

// ============================================================================
// SETUP & LOOP
// ============================================================================

void setup() {
    // USB debugging
    DEBUG_SERIAL.begin(115200);
    while (!DEBUG_SERIAL && millis() < 3000);  // Wait up to 3s for USB
    
    DEBUG_SERIAL.println("\n\n╔═══════════════════════════════════════╗");
    DEBUG_SERIAL.println("║  Arduino R4 Programmable Device      ║");
    DEBUG_SERIAL.println("║  Remote Controlled via Binary Proto  ║");
    DEBUG_SERIAL.println("╚═══════════════════════════════════════╝");
    DEBUG_SERIAL.print("\n✅ Build: ");
    DEBUG_SERIAL.print(__DATE__);
    DEBUG_SERIAL.print(" ");
    DEBUG_SERIAL.println(__TIME__);
    
    // ESP8266 serial
    ESP_SERIAL.begin(57600);
    DEBUG_SERIAL.println("✅ ESP8266 serial ready (57600 baud)");
    
    // LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    DEBUG_SERIAL.println("✅ LED initialized");
    
    // Ready for commands
    DEBUG_SERIAL.println("\n🎯 Ready for commands!\n");
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    
    // Heartbeat every 5 seconds
    if (millis() - lastHeartbeat > 5000) {
        DEBUG_SERIAL.print("💓 ");
        DEBUG_SERIAL.print(millis() / 1000);
        DEBUG_SERIAL.println("s uptime");
        lastHeartbeat = millis();
    }
    
    // Process incoming commands
    ProtocolFrame frame;
    if (protocol.receiveFrame(frame)) {
        processCommand(frame);
    }
    
    // Background tasks
    updateLedBlink();
    updateEncoder();
    updateSensors();
    
    delay(1);
}
