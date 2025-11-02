/*
 * Arduino R4 Minima - Universal Interpreter Firmware
 * 
 * This firmware receives JSON configurations over Serial and executes them.
 * No re-flashing needed - just send new JSON to change behavior!
 * 
 * Communication: Serial (115200 baud) connected to NodeMCU via level shifter
 * Format: JSON strings terminated with newline
 * 
 * Example: {"device":"led","pin":13,"mode":"blink","interval":500}
 */

#include <Arduino.h>
#include <ArduinoJson.h>

// Include libraries for all supported devices
// Note: Comment out if you don't have these libraries installed
// #include <DHT.h>
// #include <Adafruit_SSD1306.h>
// #include <Servo.h>

// ========== Configuration Storage ==========
struct DeviceConfig {
  // LED
  struct {
    int pin = 13;
    String mode = "off";  // "off", "on", "blink"
    int interval = 1000;
    bool enabled = false;
  } led;
  
  // DHT22 Sensor
  struct {
    int pin = 2;
    String mode = "off";  // "off", "read"
    int interval = 2000;
    bool enabled = false;
  } dht;
  
  // OLED Display
  struct {
    String mode = "off";  // "off", "text", "clear"
    String text = "";
    int x = 0;
    int y = 0;
    bool enabled = false;
  } oled;
  
  // Buzzer
  struct {
    int pin = 8;
    String mode = "off";  // "off", "tone", "melody"
    int frequency = 1000;
    int duration = 500;
    bool enabled = false;
  } buzzer;
  
  // Servo
  struct {
    int pin = 9;
    String mode = "off";  // "off", "position"
    int angle = 90;
    bool enabled = false;
  } servo;
  
  // Generic Digital Output
  struct {
    int pin = 7;
    String mode = "off";  // "off", "high", "low"
    bool enabled = false;
  } digital;
};

DeviceConfig config;

// ========== Timing Variables ==========
unsigned long lastLedToggle = 0;
unsigned long lastDhtRead = 0;
bool ledState = false;

// ========== Device Objects ==========
// DHT *dhtSensor = nullptr;
// Adafruit_SSD1306 *oledDisplay = nullptr;
// Servo *servoMotor = nullptr;

// ========== Function Declarations ==========
void parseConfiguration(const String& jsonString);
void applyDeviceConfig(JsonObject device);
void configureLED(JsonObject device);
void configureDHT(JsonObject device);
void configureOLED(JsonObject device);
void configureBuzzer(JsonObject device);
void configureServo(JsonObject device);
void configureDigital(JsonObject device);
void executeLED();
void executeDHT();

// ========== Setup ==========
void setup() {
  Serial.begin(115200);  // Communication with NodeMCU
  
  // Wait for Serial to be ready
  while (!Serial) {
    delay(10);
  }
  
  delay(1000);  // Give time for Serial to stabilize
  
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║  Arduino R4 Minima - Interpreter Mode     ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  Serial.println("✅ Serial initialized at 115200 baud");
  Serial.println("📡 Waiting for JSON configuration...");
  Serial.println();
  Serial.println("Supported devices:");
  Serial.println("  • LED (blink, static)");
  Serial.println("  • DHT22 (temperature/humidity)");
  Serial.println("  • OLED Display (SSD1306)");
  Serial.println("  • Servo Motor");
  Serial.println("  • Buzzer/Tone");
  Serial.println("  • Digital I/O");
  Serial.println();
  Serial.println("Send JSON config to update behavior.");
  Serial.println("Example: {\"device\":\"led\",\"pin\":13,\"mode\":\"blink\",\"interval\":500}");
  Serial.println();
  Serial.println("----------------------------------------");
  
  // Initialize built-in LED for basic testing
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

// ========== JSON Configuration Parser ==========
void parseConfiguration(const String& jsonString) {
  Serial.println();
  Serial.println("📥 Received new configuration:");
  Serial.println(jsonString);
  Serial.println();
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("❌ JSON Parse Error: ");
    Serial.println(error.c_str());
    Serial.println();
    return;
  }
  
  Serial.println("✅ JSON parsed successfully");
  
  // Check if it's a multi-device config
  if (doc.containsKey("devices") && doc["devices"].is<JsonArray>()) {
    Serial.println("📋 Multi-device configuration detected");
    JsonArray devices = doc["devices"];
    for (JsonObject device : devices) {
      applyDeviceConfig(device);
    }
  } else {
    // Single device config
    applyDeviceConfig(doc.as<JsonObject>());
  }
  
  Serial.println();
  Serial.println("✅ Configuration applied successfully!");
  Serial.println("----------------------------------------");
  Serial.println();
}

void applyDeviceConfig(JsonObject device) {
  String deviceType = device["device"] | "unknown";
  
  Serial.print("  🔧 Configuring: ");
  Serial.println(deviceType);
  
  if (deviceType == "led") {
    configureLED(device);
  } else if (deviceType == "dht22") {
    configureDHT(device);
  } else if (deviceType == "oled") {
    configureOLED(device);
  } else if (deviceType == "buzzer") {
    configureBuzzer(device);
  } else if (deviceType == "servo") {
    configureServo(device);
  } else if (deviceType == "digital") {
    configureDigital(device);
  } else {
    Serial.print("  ⚠️  Unknown device type: ");
    Serial.println(deviceType);
  }
}

// ========== Device Configuration Functions ==========

void configureLED(JsonObject device) {
  config.led.pin = device["pin"] | 13;
  config.led.mode = device["mode"] | "off";
  config.led.interval = device["interval"] | 1000;
  config.led.enabled = (config.led.mode != "off");
  
  pinMode(config.led.pin, OUTPUT);
  
  Serial.print("     Pin: ");
  Serial.print(config.led.pin);
  Serial.print(", Mode: ");
  Serial.print(config.led.mode);
  Serial.print(", Interval: ");
  Serial.println(config.led.interval);
  
  // Immediate action for static modes
  if (config.led.mode == "on") {
    digitalWrite(config.led.pin, HIGH);
  } else if (config.led.mode == "off") {
    digitalWrite(config.led.pin, LOW);
  }
  
  lastLedToggle = millis();
  ledState = false;
}

void configureDHT(JsonObject device) {
  config.dht.pin = device["pin"] | 2;
  config.dht.mode = device["mode"] | "off";
  config.dht.interval = device["interval"] | 2000;
  config.dht.enabled = (config.dht.mode != "off");
  
  Serial.print("     Pin: ");
  Serial.print(config.dht.pin);
  Serial.print(", Mode: ");
  Serial.print(config.dht.mode);
  Serial.print(", Interval: ");
  Serial.println(config.dht.interval);
  
  // Initialize DHT sensor if library is available
  /*
  if (dhtSensor != nullptr) {
    delete dhtSensor;
  }
  dhtSensor = new DHT(config.dht.pin, DHT22);
  dhtSensor->begin();
  */
  
  Serial.println("     ⚠️  DHT22 library not included - uncomment in code to enable");
}

void configureOLED(JsonObject device) {
  config.oled.mode = device["mode"] | "off";
  config.oled.text = device["text"] | "";
  config.oled.x = device["x"] | 0;
  config.oled.y = device["y"] | 0;
  config.oled.enabled = (config.oled.mode != "off");
  
  Serial.print("     Mode: ");
  Serial.print(config.oled.mode);
  Serial.print(", Text: \"");
  Serial.print(config.oled.text);
  Serial.println("\"");
  
  Serial.println("     ⚠️  OLED library not included - uncomment in code to enable");
}

void configureBuzzer(JsonObject device) {
  config.buzzer.pin = device["pin"] | 8;
  config.buzzer.mode = device["mode"] | "off";
  config.buzzer.frequency = device["frequency"] | 1000;
  config.buzzer.duration = device["duration"] | 500;
  config.buzzer.enabled = (config.buzzer.mode != "off");
  
  pinMode(config.buzzer.pin, OUTPUT);
  
  Serial.print("     Pin: ");
  Serial.print(config.buzzer.pin);
  Serial.print(", Mode: ");
  Serial.println(config.buzzer.mode);
  
  if (config.buzzer.mode == "tone") {
    tone(config.buzzer.pin, config.buzzer.frequency, config.buzzer.duration);
  }
}

void configureServo(JsonObject device) {
  config.servo.pin = device["pin"] | 9;
  config.servo.mode = device["mode"] | "off";
  config.servo.angle = device["angle"] | 90;
  config.servo.enabled = (config.servo.mode != "off");
  
  Serial.print("     Pin: ");
  Serial.print(config.servo.pin);
  Serial.print(", Angle: ");
  Serial.println(config.servo.angle);
  
  Serial.println("     ⚠️  Servo library not included - uncomment in code to enable");
}

void configureDigital(JsonObject device) {
  config.digital.pin = device["pin"] | 7;
  config.digital.mode = device["mode"] | "off";
  config.digital.enabled = (config.digital.mode != "off");
  
  pinMode(config.digital.pin, OUTPUT);
  
  Serial.print("     Pin: ");
  Serial.print(config.digital.pin);
  Serial.print(", Mode: ");
  Serial.println(config.digital.mode);
  
  if (config.digital.mode == "high") {
    digitalWrite(config.digital.pin, HIGH);
  } else if (config.digital.mode == "low") {
    digitalWrite(config.digital.pin, LOW);
  }
}

// ========== Device Execution Functions ==========

void executeLED() {
  if (!config.led.enabled || config.led.mode != "blink") {
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastLedToggle >= config.led.interval) {
    lastLedToggle = currentMillis;
    ledState = !ledState;
    digitalWrite(config.led.pin, ledState ? HIGH : LOW);
  }
}

void executeDHT() {
  if (!config.dht.enabled || config.dht.mode != "read") {
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - lastDhtRead >= config.dht.interval) {
    lastDhtRead = currentMillis;
    
    // Read DHT sensor if library is available
    /*
    if (dhtSensor != nullptr) {
      float humidity = dhtSensor->readHumidity();
      float temperature = dhtSensor->readTemperature();
      
      if (!isnan(humidity) && !isnan(temperature)) {
        Serial.print("🌡️  Temperature: ");
        Serial.print(temperature);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
      }
    }
    */
    
    Serial.println("📊 DHT22: (Enable library to see real data)");
  }
}

// ========== Main Loop ==========
void loop() {
  // Job A: Check for new configuration from NodeMCU
  if (Serial.available() > 0) {
    String incomingJson = Serial.readStringUntil('\n');
    incomingJson.trim();
    
    if (incomingJson.length() > 0) {
      parseConfiguration(incomingJson);
    }
  }
  
  // Job B: Execute current configuration
  executeLED();
  executeDHT();
  
  // Add other device execution functions here as needed
}
