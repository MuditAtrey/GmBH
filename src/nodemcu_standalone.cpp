/*
 * NodeMCU v3 (ESP8266) - Standalone Interpreter with MQTT
 * 
 * This firmware does EVERYTHING:
 * 1. Connects to WiFi
 * 2. Subscribes to MQTT for JSON configurations
 * 3. Executes the configurations directly on NodeMCU
 * 
 * No Arduino needed! No level shifter needed!
 * All sensors run at 3.3V natively.
 * 
 * Supported Devices:
 * - LED (blink, static on/off)
 * - DHT22 (temperature/humidity) - 3.3V compatible
 * - OLED SSD1306 (SPI 7-pin) - 3.3V logic
 * - Buzzer/Tone
 * - Digital I/O
 * 
 * MQTT Topic: arduino_designer/nodemcu/config
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Uncomment these as you add sensors:
// #include <DHT.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
// #include <SPI.h>

// ========== WiFi Configuration ==========
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// ========== MQTT Configuration ==========
const char* mqtt_server = "broker.hivemq.com";  // Free public broker
const int mqtt_port = 1883;
const char* mqtt_topic_config = "arduino_designer/nodemcu/config";
const char* mqtt_topic_status = "arduino_designer/nodemcu/status";
const char* mqtt_client_id = "nodemcu_standalone_001";

// ========== Objects ==========
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========== Device Configuration Storage ==========
struct DeviceConfig {
  // LED
  struct {
    int pin = LED_BUILTIN;  // D4 on NodeMCU (GPIO2)
    String mode = "off";    // "off", "on", "blink"
    int interval = 1000;
    bool enabled = false;
    unsigned long lastToggle = 0;
    bool state = false;
  } led;
  
  // DHT22 Sensor
  struct {
    int pin = D2;           // GPIO4
    String mode = "off";    // "off", "read"
    int interval = 2000;
    bool enabled = false;
    unsigned long lastRead = 0;
  } dht;
  
  // OLED Display (SPI 7-pin)
  struct {
    int pin_mosi = D7;      // GPIO13 - MOSI/SDA (hardware SPI)
    int pin_clk = D5;       // GPIO14 - CLK/SCK (hardware SPI)
    int pin_dc = D3;        // GPIO0 - Data/Command
    int pin_rst = D0;       // GPIO16 - Reset
    int pin_cs = D8;        // GPIO15 - Chip Select
    // VCC → 3.3V, GND → GND (additional 2 pins)
    String mode = "off";    // "off", "text", "clear"
    String text = "";
    int x = 0;
    int y = 0;
    bool enabled = false;
  } oled;
  
  // Buzzer/Tone
  struct {
    int pin = D1;           // GPIO5
    String mode = "off";    // "off", "tone", "melody"
    int frequency = 1000;
    int duration = 500;
    bool enabled = false;
  } buzzer;
  
  // Generic Digital Output
  struct {
    int pin = D3;           // GPIO0
    String mode = "off";    // "off", "high", "low"
    bool enabled = false;
  } digital;
  
  // Button Input
  struct {
    int pin = D5;           // GPIO14
    String mode = "off";    // "off", "pullup", "pulldown"
    bool enabled = false;
    bool lastState = false;
    unsigned long lastDebounce = 0;
    const unsigned long debounceDelay = 50;
  } button;
  
  // Logic - Loop
  struct {
    int count = 0;
    bool enabled = false;
  } loop;
  
  // Logic - Delay
  struct {
    unsigned long time = 0;
    unsigned long startTime = 0;
    bool enabled = false;
    bool waiting = false;
  } delayBlock;
  
  String activeDevice = "none";
};

DeviceConfig config;

// ========== Statistics ==========
unsigned long configsReceived = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long statusInterval = 30000;  // 30 seconds

// ========== Function Declarations ==========
void setupWiFi();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void parseConfiguration(const String& jsonString);
void applyDeviceConfig(JsonObject device);
void configureLED(JsonObject device);
void configureDHT(JsonObject device);
void configureOLED(JsonObject device);
void configureBuzzer(JsonObject device);
void configureDigital(JsonObject device);
void configureButton(JsonObject device);
void executeLED();
void executeDHT();
void executeButton();

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║  NodeMCU Standalone Interpreter           ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off (inverted on NodeMCU)
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);  // Increase buffer for larger JSON
  
  Serial.println("✅ Setup complete!");
  Serial.println("📡 Waiting for JSON configurations via MQTT...");
  Serial.println();
  Serial.println("Supported devices (all 3.3V compatible):");
  Serial.println("  • LED (blink, static)");
  Serial.println("  • DHT22 (temperature/humidity)");
  Serial.println("  • OLED Display (SSD1306)");
  Serial.println("  • Buzzer/Tone");
  Serial.println("  • Digital I/O");
  Serial.println();
  Serial.println("MQTT Topic: " + String(mqtt_topic_config));
  Serial.println("----------------------------------------");
  Serial.println();
}

// ========== WiFi Setup ==========
void setupWiFi() {
  Serial.print("📡 Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi Connected!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println();
  } else {
    Serial.println("❌ WiFi connection failed!");
    Serial.println("   Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
}

// ========== MQTT Callback ==========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  configsReceived++;
  
  Serial.println();
  Serial.println("📨 MQTT Configuration Received:");
  Serial.print("   Topic: ");
  Serial.println(topic);
  Serial.print("   Length: ");
  Serial.print(length);
  Serial.println(" bytes");
  
  // Convert payload to String
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("   Payload:");
  Serial.println("   " + message);
  Serial.println();
  
  // Parse and apply configuration
  parseConfiguration(message);
  
  // Blink LED to show activity
  digitalWrite(LED_BUILTIN, LOW);   // LED on
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off
}

// ========== MQTT Reconnect ==========
void reconnectMQTT() {
  int attempts = 0;
  while (!mqttClient.connected() && attempts < 3) {
    Serial.print("🔄 Connecting to MQTT broker... ");
    
    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println("✅ Connected!");
      
      // Subscribe to config topic
      mqttClient.subscribe(mqtt_topic_config);
      Serial.print("📡 Subscribed to: ");
      Serial.println(mqtt_topic_config);
      
      // Publish status
      String statusMsg = "{\"status\":\"online\",\"ip\":\"" + WiFi.localIP().toString() + 
                        "\",\"device\":\"" + config.activeDevice + "\"}";
      mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
      
      Serial.println();
      return;
    } else {
      Serial.print("❌ Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println();
      attempts++;
      delay(2000);
    }
  }
}

// ========== JSON Configuration Parser ==========
void parseConfiguration(const String& jsonString) {
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
    Serial.println("📋 Multi-device configuration");
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
  Serial.println("   Active device: " + config.activeDevice);
  Serial.println("   Configs received: " + String(configsReceived));
  Serial.println("----------------------------------------");
  Serial.println();
}

void applyDeviceConfig(JsonObject device) {
  String deviceType = device["device"] | "unknown";
  config.activeDevice = deviceType;
  
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
  } else if (deviceType == "digital") {
    configureDigital(device);
  } else if (deviceType == "button") {
    configureButton(device);
  } else if (deviceType == "loop") {
    // Handle loop - execute child actions multiple times
    int loopCount = device["count"] | 1;
    Serial.print("     Loop count: ");
    Serial.println(loopCount);
    
    if (device.containsKey("actions") && device["actions"].is<JsonArray>()) {
      JsonArray actions = device["actions"];
      for (int i = 0; i < loopCount; i++) {
        Serial.print("     Iteration ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.println(loopCount);
        for (JsonObject action : actions) {
          applyDeviceConfig(action);
        }
      }
    }
  } else if (deviceType == "delay") {
    // Handle delay
    unsigned long delayTime = device["time"] | 1000;
    Serial.print("     Delay: ");
    Serial.print(delayTime);
    Serial.println(" ms");
    delay(delayTime);
  } else if (deviceType == "if") {
    // Handle if condition (simplified - would need actual sensor reading)
    String sensor = device["sensor"] | "temp";
    String op = device["operator"] | "gt";
    float value = device["value"] | 0;
    
    Serial.print("     If ");
    Serial.print(sensor);
    Serial.print(" ");
    Serial.print(op);
    Serial.print(" ");
    Serial.println(value);
    Serial.println("     (Condition blocks need sensor data - not fully implemented)");
    
    // Execute THEN branch by default for demo
    if (device.containsKey("then") && device["then"].is<JsonArray>()) {
      JsonArray thenActions = device["then"];
      for (JsonObject action : thenActions) {
        applyDeviceConfig(action);
      }
    }
  } else if (deviceType == "variable") {
    // Handle variable assignment
    String varName = device["name"] | "var";
    float varValue = device["value"] | 0;
    Serial.print("     Set ");
    Serial.print(varName);
    Serial.print(" = ");
    Serial.println(varValue);
    Serial.println("     (Variables stored in memory - not persistent)");
  } else {
    Serial.print("  ⚠️  Unknown device type: ");
    Serial.println(deviceType);
  }
}

// ========== Device Configuration Functions ==========

void configureLED(JsonObject device) {
  config.led.pin = device["pin"] | LED_BUILTIN;
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
    digitalWrite(config.led.pin, config.led.pin == LED_BUILTIN ? LOW : HIGH);
  } else if (config.led.mode == "off") {
    digitalWrite(config.led.pin, config.led.pin == LED_BUILTIN ? HIGH : LOW);
  }
  
  config.led.lastToggle = millis();
  config.led.state = false;
}

void configureDHT(JsonObject device) {
  config.dht.pin = device["pin"] | D2;
  config.dht.mode = device["mode"] | "off";
  config.dht.interval = device["interval"] | 2000;
  config.dht.enabled = (config.dht.mode != "off");
  
  Serial.print("     Pin: ");
  Serial.print(config.dht.pin);
  Serial.print(", Mode: ");
  Serial.print(config.dht.mode);
  Serial.print(", Interval: ");
  Serial.println(config.dht.interval);
  
  Serial.println("     ⚠️  DHT22 library not included - uncomment in code to enable");
  Serial.println("     💡 DHT22 works on 3.3V - safe to connect directly!");
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
  Serial.println("     SPI Pins: MOSI=D7, CLK=D5, DC=D3, RST=D0, CS=D8");
  
  Serial.println("     ⚠️  OLED library not included - uncomment in code to enable");
  Serial.println("     💡 SSD1306 OLED is 3.3V logic - safe to connect directly!");
  Serial.println("     📌 7-Pin SPI: VCC→3.3V, GND→GND, D7→MOSI, D5→CLK, D8→CS, D3→DC, D0→RST");
}

void configureBuzzer(JsonObject device) {
  config.buzzer.pin = device["pin"] | D1;
  config.buzzer.mode = device["mode"] | "off";
  config.buzzer.frequency = device["frequency"] | 1000;
  config.buzzer.duration = device["duration"] | 500;
  config.buzzer.enabled = (config.buzzer.mode != "off");
  
  pinMode(config.buzzer.pin, OUTPUT);
  
  Serial.print("     Pin: ");
  Serial.print(config.buzzer.pin);
  Serial.print(", Frequency: ");
  Serial.print(config.buzzer.frequency);
  Serial.println(" Hz");
  
  if (config.buzzer.mode == "tone") {
    tone(config.buzzer.pin, config.buzzer.frequency, config.buzzer.duration);
  }
}

void configureDigital(JsonObject device) {
  config.digital.pin = device["pin"] | D3;
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

void configureButton(JsonObject device) {
  config.button.pin = device["pin"] | D5;
  config.button.mode = device["mode"] | "pullup";
  config.button.enabled = (config.button.mode != "off");
  
  // Configure pin based on mode
  if (config.button.mode == "pullup") {
    pinMode(config.button.pin, INPUT_PULLUP);
  } else if (config.button.mode == "pulldown") {
    pinMode(config.button.pin, INPUT);
  } else {
    pinMode(config.button.pin, INPUT);
  }
  
  config.button.lastState = digitalRead(config.button.pin);
  
  Serial.print("     Pin: ");
  Serial.print(config.button.pin);
  Serial.print(", Mode: ");
  Serial.print(config.button.mode);
  Serial.print(", Initial State: ");
  Serial.println(config.button.lastState ? "HIGH" : "LOW");
}

// ========== Device Execution Functions ==========

void executeLED() {
  if (!config.led.enabled || config.led.mode != "blink") {
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - config.led.lastToggle >= config.led.interval) {
    config.led.lastToggle = currentMillis;
    config.led.state = !config.led.state;
    
    // Handle inverted LED_BUILTIN
    if (config.led.pin == LED_BUILTIN) {
      digitalWrite(config.led.pin, config.led.state ? LOW : HIGH);
    } else {
      digitalWrite(config.led.pin, config.led.state ? HIGH : LOW);
    }
  }
}

void executeDHT() {
  if (!config.dht.enabled || config.dht.mode != "read") {
    return;
  }
  
  unsigned long currentMillis = millis();
  if (currentMillis - config.dht.lastRead >= config.dht.interval) {
    config.dht.lastRead = currentMillis;
    
    // Placeholder - uncomment when DHT library is included
    Serial.println("🌡️  DHT22: (Enable library to see real data)");
  }
}

void executeButton() {
  if (!config.button.enabled) {
    return;
  }
  
  unsigned long currentMillis = millis();
  bool currentState = digitalRead(config.button.pin);
  
  // Debounce logic
  if (currentState != config.button.lastState) {
    if (currentMillis - config.button.lastDebounce > config.button.debounceDelay) {
      config.button.lastDebounce = currentMillis;
      config.button.lastState = currentState;
      
      // Report button state change
      Serial.print("🔘 Button ");
      Serial.print(config.button.pin);
      Serial.print(": ");
      Serial.println(currentState ? "PRESSED" : "RELEASED");
      
      // Publish button state via MQTT if connected
      if (mqttClient.connected()) {
        String statusMsg = "{\"button\":" + String(config.button.pin) + 
                          ",\"state\":" + String(currentState ? 1 : 0) + "}";
        mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
      }
    }
  }
}

// ========== Main Loop ==========
void loop() {
  // Ensure WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi disconnected! Reconnecting...");
    setupWiFi();
  }
  
  // Ensure MQTT is connected
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();  // Process MQTT messages
  
  // Execute current configuration
  executeLED();
  executeDHT();
  executeButton();
  
  // Periodic status update
  unsigned long now = millis();
  if (now - lastStatusUpdate > statusInterval) {
    lastStatusUpdate = now;
    
    if (mqttClient.connected()) {
      String statusMsg = "{\"status\":\"online\",\"ip\":\"" + WiFi.localIP().toString() + 
                        "\",\"device\":\"" + config.activeDevice + 
                        "\",\"configs\":" + String(configsReceived) + 
                        ",\"uptime\":" + String(millis()/1000) + "}";
      mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
    }
  }
  
  delay(10);  // Small delay for stability
}
