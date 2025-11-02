/*
 * Arduino R4 Minima - Dynamic Configuration System
 * 
 * Receives JSON configuration from server via ESP8266
 * Dynamically instantiates and controls devices
 * Executes visual programming blocks
 * 
 * Hardware Connections:
 * - Arduino RX0 ← ESP8266 D1 (GPIO5) TX
 * - Arduino TX1 → ESP8266 D2 (GPIO4) RX
 * - Common GND
 */

#include <Arduino.h>
#include "ArduinoProtocol.h"
#include "DynamicArduino.h"

// Protocol handler (uses Serial1 for ESP8266, Serial for debug)
// Arduino R4 Minima has two UARTs:
// - Serial (USB) for debugging
// - Serial1 (RX0/TX1 pins) for ESP8266 communication
ProtocolHandler protocol(&Serial1);

// Dynamic device management
Device* devices[MAX_DEVICES];
uint8_t deviceCount = 0;

// Visual programming blocks
struct ProgramBlock {
    String type;          // "control", "logic", "sensor", "motor"
    String subtype;       // "if", "while", "and", etc.
    JsonObject params;    // Block parameters
};

ProgramBlock programBlocks[MAX_PROGRAM_BLOCKS];
uint8_t programBlockCount = 0;

// Variables for visual programming
struct ProgramVariable {
    String name;
    float value;
};

ProgramVariable variables[20];
uint8_t variableCount = 0;

// Helper functions
DeviceType getDeviceTypeFromString(const char* typeStr) {
    if (strcmp(typeStr, "LED") == 0) return DEVICE_LED;
    if (strcmp(typeStr, "Button") == 0) return DEVICE_BUTTON;
    if (strcmp(typeStr, "MG90S Servo") == 0) return DEVICE_SERVO;
    if (strcmp(typeStr, "HC-SR04 Ultrasonic") == 0) return DEVICE_ULTRASONIC;
    if (strcmp(typeStr, "Buzzer") == 0) return DEVICE_BUZZER;
    if (strcmp(typeStr, "Relay") == 0) return DEVICE_RELAY;
    if (strcmp(typeStr, "Potentiometer") == 0) return DEVICE_POTENTIOMETER;
    if (strcmp(typeStr, "LDR") == 0) return DEVICE_LDR;
    if (strcmp(typeStr, "PIR Sensor") == 0) return DEVICE_PIR;
    return DEVICE_NONE;
}

Device* findDevice(const String& deviceId) {
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i] && devices[i]->getId() == deviceId) {
            return devices[i];
        }
    }
    return nullptr;
}

float getVariable(const String& name) {
    for (uint8_t i = 0; i < variableCount; i++) {
        if (variables[i].name == name) {
            return variables[i].value;
        }
    }
    return 0;
}

void setVariable(const String& name, float value) {
    for (uint8_t i = 0; i < variableCount; i++) {
        if (variables[i].name == name) {
            variables[i].value = value;
            return;
        }
    }
    
    if (variableCount < 20) {
        variables[variableCount].name = name;
        variables[variableCount].value = value;
        variableCount++;
    }
}

void clearDevices() {
    Serial.println("Clearing all devices...");
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i]) {
            delete devices[i];
            devices[i] = nullptr;
        }
    }
    deviceCount = 0;
}

void loadConfiguration(const JsonObject& config) {
    Serial.println("\n=== LOADING CONFIGURATION ===");
    
    // Clear existing devices
    clearDevices();
    
    // Load devices
    JsonArray devicesArray = config["devices"];
    if (devicesArray) {
        Serial.print("Creating ");
        Serial.print(devicesArray.size());
        Serial.println(" devices:");
        
        for (JsonObject deviceObj : devicesArray) {
            const char* id = deviceObj["id"];
            const char* type = deviceObj["type"];
            const char* pinStr = deviceObj["pin"];
            
            if (!id || !type || !pinStr) continue;
            
            // Parse pin number
            uint8_t pinNum = 0;
            if (pinStr[0] == 'D') {
                pinNum = atoi(pinStr + 1);
            } else if (pinStr[0] == 'A') {
                pinNum = A0 + atoi(pinStr + 1);
            } else {
                pinNum = atoi(pinStr);
            }
            
            Serial.print("  - ");
            Serial.print(type);
            Serial.print(" (");
            Serial.print(id);
            Serial.print(") on pin ");
            Serial.println(pinNum);
            
            Device* device = nullptr;
            DeviceType deviceType = getDeviceTypeFromString(type);
            
            switch (deviceType) {
                case DEVICE_LED:
                    device = new LEDDevice(id, pinNum);
                    break;
                    
                case DEVICE_BUTTON:
                    device = new ButtonDevice(id, pinNum);
                    break;
                    
                case DEVICE_SERVO:
                    device = new ServoDevice(id, pinNum);
                    break;
                    
                case DEVICE_ULTRASONIC: {
                    // Ultrasonic needs trigger and echo pins
                    const char* trigPinStr = deviceObj["trigPin"] | pinStr;
                    const char* echoPinStr = deviceObj["echoPin"] | pinStr;
                    
                    uint8_t trigPin = (trigPinStr[0] == 'D') ? atoi(trigPinStr + 1) : atoi(trigPinStr);
                    uint8_t echoPin = (echoPinStr[0] == 'D') ? atoi(echoPinStr + 1) : atoi(echoPinStr);
                    
                    device = new UltrasonicDevice(id, trigPin, echoPin);
                    break;
                }
                    
                case DEVICE_BUZZER:
                    device = new BuzzerDevice(id, pinNum);
                    break;
                    
                case DEVICE_RELAY:
                    device = new RelayDevice(id, pinNum);
                    break;
                    
                case DEVICE_POTENTIOMETER:
                    device = new PotentiometerDevice(id, pinNum);
                    break;
                    
                case DEVICE_LDR:
                    device = new LDRDevice(id, pinNum);
                    break;
                    
                case DEVICE_PIR:
                    device = new PIRDevice(id, pinNum);
                    break;
                    
                default:
                    Serial.println("    WARNING: Unknown device type");
                    continue;
            }
            
            if (device && deviceCount < MAX_DEVICES) {
                devices[deviceCount++] = device;
                device->begin();
                Serial.println("    ✓ Device initialized");
            }
        }
    }
    
    // Load visual program blocks
    JsonArray programArray = config["visualProgram"];
    if (programArray) {
        Serial.print("\nLoading ");
        Serial.print(programArray.size());
        Serial.println(" program blocks");
        
        programBlockCount = 0;
        // Note: Visual programming execution will be implemented later
        // For now, we just acknowledge the blocks are loaded
    }
    
    // Auto-start blinking on built-in LED if present
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i] && devices[i]->getId() == "builtin_led") {
            Serial.println("\n💡 Auto-starting built-in LED blink for testing...");
            StaticJsonDocument<64> params;
            params["interval"] = 1000;  // 1 second blink
            devices[i]->execute("blink", params.as<JsonObject>());
            Serial.println("   ✓ Built-in LED blinking");
            break;
        }
    }
    
    Serial.println("=== CONFIGURATION LOADED ===\n");
}

void handleConfigDeploy(const ProtocolFrame& frame) {
    Serial.println("\n>>> RECEIVED CONFIG DEPLOYMENT <<<");
    
    // Parse JSON from payload
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, frame.payload, frame.length);
    
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
        return;
    }
    
    // Load the configuration
    JsonObject config = doc.as<JsonObject>();
    loadConfiguration(config);
    
    // Send ACK
    protocol.sendAck();
    Serial.println("✓ Configuration deployed successfully\n");
}

void handleDeviceAction(const ProtocolFrame& frame) {
    // Execute action on a specific device
    // Payload format: deviceId (string), action (string), params (JSON)
    
    PayloadParser parser(frame.payload, frame.length);
    char deviceId[32];
    char action[32];
    
    if (!parser.readString(deviceId, sizeof(deviceId)) || 
        !parser.readString(action, sizeof(action))) {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
        return;
    }
    
    Device* device = findDevice(deviceId);
    if (!device) {
        protocol.sendError(PROTO_ERR_INVALID_PARAM);
        return;
    }
    
    // Parse remaining payload as JSON params
    StaticJsonDocument<512> doc;
    if (parser.remaining() > 0) {
        DeserializationError error = deserializeJson(doc, frame.payload + parser.position, parser.remaining());
        if (error) {
            protocol.sendError(PROTO_ERR_INVALID_PARAM);
            return;
        }
    }
    
    JsonObject params = doc.as<JsonObject>();
    device->execute(action, params);
    
    protocol.sendAck();
}

void handleDeviceStateRequest(const ProtocolFrame& frame) {
    // Send state of all devices or specific device
    PayloadParser parser(frame.payload, frame.length);
    char deviceId[32] = "";
    
    if (parser.remaining() > 0) {
        parser.readString(deviceId, sizeof(deviceId));
    }
    
    // Build JSON response
    StaticJsonDocument<2048> doc;
    JsonArray devicesArray = doc.createNestedArray("devices");
    
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i]) {
            if (strlen(deviceId) == 0 || devices[i]->getId() == deviceId) {
                JsonObject deviceObj = devicesArray.createNestedObject();
                deviceObj["id"] = devices[i]->getId();
                JsonObject state = devices[i]->getState(doc);
                deviceObj["state"] = state;
            }
        }
    }
    
    // Serialize and send
    String output;
    serializeJson(doc, output);
    
    protocol.sendFrame(CMD_DATA_STRING, (uint8_t*)output.c_str(), output.length());
}

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
    
    Serial.println("\n\n");
    Serial.println("═══════════════════════════════════════");
    Serial.println("   ARDUINO R4 DYNAMIC SYSTEM");
    Serial.println("═══════════════════════════════════════");
    Serial.println("Build: " __DATE__ " " __TIME__);
    Serial.println("Features:");
    Serial.println("  ✓ Dynamic device loading");
    Serial.println("  ✓ Visual programming support");
    Serial.println("  ✓ JSON configuration over ESP8266");
    Serial.println("═══════════════════════════════════════\n");
    
    // Hardware serial for ESP8266 at 57600 (must match ESP8266)
    Serial1.begin(57600);
    
    // Built-in LED for status
    pinMode(LED_BUILTIN, OUTPUT);
    
    // Startup blink pattern
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }
    
    Serial.println("✓ Arduino R4 Dynamic System Ready");
    Serial.println("✓ Waiting for configuration from ESP8266...\n");
    
    delay(500);
}

void processCommand(const ProtocolFrame& frame) {
    Serial.print("⚡ CMD: 0x");
    Serial.print(frame.commandId, HEX);
    Serial.print(" (");
    Serial.print(frame.length);
    Serial.println(" bytes)");
    
    switch (frame.commandId) {
        case CMD_PING:
            Serial.println("→ PONG");
            protocol.sendCommand(CMD_PONG);
            break;
            
        case CMD_SENSOR_CONFIG:
            // This is our configuration deployment command
            Serial.println("→ CONFIG_DEPLOY");
            handleConfigDeploy(frame);
            break;
            
        case CMD_DATA_STRING:
            // Device action command
            Serial.println("→ DEVICE_ACTION");
            handleDeviceAction(frame);
            break;
            
        case CMD_SENSOR_READ:
            // Request device state
            Serial.println("→ STATE_REQUEST");
            handleDeviceStateRequest(frame);
            break;
            
        default:
            Serial.println("→ ERROR: Unknown command");
            protocol.sendError(PROTO_ERR_INVALID_CMD);
            break;
    }
}

void updateAllDevices() {
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i] && devices[i]->isEnabled()) {
            devices[i]->update();
        }
    }
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    static unsigned long lastDeviceUpdate = 0;
    static bool ledState = false;
    
    // Heartbeat LED (slow blink when idle)
    if (millis() - lastHeartbeat > 2000) {
        if (deviceCount == 0) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState);
        } else {
            // Quick blink if devices are loaded
            digitalWrite(LED_BUILTIN, HIGH);
            delay(50);
            digitalWrite(LED_BUILTIN, LOW);
        }
        lastHeartbeat = millis();
    }
    
    // Receive and process protocol frames
    ProtocolFrame frame;
    if (protocol.receiveFrame(frame)) {
        processCommand(frame);
    }
    
    // Update all devices (every 10ms)
    if (millis() - lastDeviceUpdate >= 10) {
        updateAllDevices();
        lastDeviceUpdate = millis();
    }
    
    // Small delay to prevent tight loop
    delay(1);
}