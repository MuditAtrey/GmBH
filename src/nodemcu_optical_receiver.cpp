/*
 * NodeMCU v3 - Optical Configuration Receiver
 * 
 * Receives JSON configurations via photodiode optical transmission
 * from the web designer's "Blink to Send" feature.
 * 
 * Circuit:
 * - Photodiode CATHODE (shorter leg) → NodeMCU 3.3V
 * - Photodiode ANODE (longer leg) → NodeMCU A0 + 4.7kΩ resistor
 * - 4.7kΩ resistor other leg → GND
 * 
 * Protocol:
 * - Manchester encoding: 0→01, 1→10
 * - Bit duration: 2ms per half-bit
 * - Preamble: 16 alternating bits (11111111 00000000)
 * - Data: 8 bits per character
 * - Postamble: Same as preamble for sync verification
 * 
 * After receiving configuration via optical:
 * - Executes configuration locally
 * - Also connects to WiFi/MQTT for future updates
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// ========== WiFi Configuration ==========
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// ========== MQTT Configuration ==========
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_config = "arduino_designer/nodemcu/config";
const char* mqtt_topic_status = "arduino_designer/nodemcu/status";
const char* mqtt_client_id = "nodemcu_optical_001";

// ========== Photodiode Configuration ==========
#define PHOTODIODE_PIN A0
#define THRESHOLD_LIGHT 512       // ADC value above = LIGHT (1), below = DARK (0)
#define BIT_HALF_DURATION 2       // milliseconds - must match transmitter
#define PREAMBLE_PATTERN 0xFF00   // 11111111 00000000
#define SYNC_BITS 16              // Preamble/postamble length

// ========== Objects ==========
WiFiClient espClient;
PubSubClient mqttClient(espClient);
DHT* dhtSensor = nullptr;
Adafruit_PWMServoDriver* servoDriver = nullptr;

// ========== Servo Constants ==========
#define SERVOMIN  150
#define SERVOMAX  600
#define SERVO_FREQ 50

// ========== Device Configuration Storage ==========
struct DeviceConfig {
  // LED
  struct {
    int pin = LED_BUILTIN;
    String mode = "off";
    int interval = 1000;
    bool enabled = false;
    unsigned long lastToggle = 0;
    bool state = false;
  } led;
  
  // DHT22 Sensor
  struct {
    int pin = D2;
    String mode = "off";
    int interval = 2000;
    bool enabled = false;
    unsigned long lastRead = 0;
    float lastTemp = 0;
    float lastHum = 0;
  } dht;
  
  // OLED Display
  struct {
    String mode = "off";
    String text = "";
    String sensorType = "";
    int x = 0;
    int y = 0;
    bool enabled = false;
  } oled;
  
  // Buzzer/Tone
  struct {
    int pin = D1;
    String mode = "off";
    int frequency = 1000;
    int duration = 500;
    bool enabled = false;
  } buzzer;
  
  // Digital Output
  struct {
    int pin = D3;
    String mode = "off";
    bool enabled = false;
  } digital;
  
  // Servo Controller (PCA9685)
  struct {
    bool enabled = false;
    int channels[16];  // 0-180 degrees
    unsigned long lastUpdate = 0;
  } servo;
  
  String activeDevice = "none";
};

DeviceConfig config;

// ========== Optical Reception State ==========
String receivedData = "";
bool opticalReceptionMode = true;
unsigned long lastBitTime = 0;

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
void configureServo(JsonObject device);
void executeLED();
void executeDHT();
void initServoDriver();
void setServoAngle(int channel, int angle);
bool receiveOpticalData();
void processReceivedConfig();

// ========== Setup ==========
void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║  NodeMCU Optical Configuration Receiver   ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize built-in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off
  
  // Initialize photodiode pin
  pinMode(PHOTODIODE_PIN, INPUT);
  
  Serial.println("💡 OPTICAL RECEIVER MODE");
  Serial.println("   Place photodiode on blinking screen");
  Serial.println("   Circuit:");
  Serial.println("   • Cathode (−) → 3.3V");
  Serial.println("   • Anode (+)   → A0 + 4.7kΩ → GND");
  Serial.println();
  Serial.println("📊 Waiting for optical transmission...");
  Serial.println("   Threshold: " + String(THRESHOLD_LIGHT));
  Serial.println("   Bit duration: " + String(BIT_HALF_DURATION) + "ms");
  Serial.println();
  
  // Test photodiode reading
  int reading = analogRead(PHOTODIODE_PIN);
  Serial.print("   Current ADC: ");
  Serial.println(reading);
  
  if (reading < 100) {
    Serial.println("   ⚠️  Very dark - ensure photodiode is properly connected");
  } else if (reading > 900) {
    Serial.println("   ⚠️  Very bright - ensure proper lighting");
  } else {
    Serial.println("   ✅ ADC reading looks good");
  }
  Serial.println();
  Serial.println("────────────────────────────────────────────");
  Serial.println();
}

// ========== Main Loop ==========
void loop() {
  if (opticalReceptionMode) {
    // Try to receive optical data
    if (receiveOpticalData()) {
      Serial.println();
      Serial.println("✅ Optical transmission received!");
      Serial.print("   Data length: ");
      Serial.println(receivedData.length());
      Serial.println("   Data:");
      Serial.println(receivedData);
      Serial.println();
      
      // Process the configuration
      processReceivedConfig();
      
      // Switch to normal operation mode
      opticalReceptionMode = false;
      
      // Now connect to WiFi/MQTT for future updates
      Serial.println("📡 Connecting to WiFi for MQTT updates...");
      setupWiFi();
      mqttClient.setServer(mqtt_server, mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttClient.setBufferSize(512);
      
      Serial.println("✅ Ready for normal operation!");
      Serial.println();
    }
  } else {
    // Normal operation mode (after optical config received)
    
    // Ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
      setupWiFi();
    }
    
    // Ensure MQTT is connected
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
    
    // Execute current configuration
    executeLED();
    executeDHT();
  }
  
  delay(10);
}

// ========== Optical Data Reception ==========
bool receiveOpticalData() {
  static bool syncDetected = false;
  static String binaryData = "";
  static String manchesterBits = "";
  static int consecutiveBits = 0;
  static int lastBit = -1;
  static unsigned long syncStartTime = 0;
  
  // Read photodiode
  int reading = analogRead(PHOTODIODE_PIN);
  int currentBit = (reading >= THRESHOLD_LIGHT) ? 1 : 0;
  
  // Detect bit transitions (simple edge detection)
  unsigned long now = millis();
  
  if (!syncDetected) {
    // Look for preamble pattern (8 consecutive 1s followed by 8 consecutive 0s)
    if (currentBit == lastBit) {
      consecutiveBits++;
    } else {
      consecutiveBits = 1;
    }
    
    // Check for sync pattern
    if (consecutiveBits >= 8 && currentBit == 0 && lastBit == 0) {
      syncDetected = true;
      syncStartTime = now;
      manchesterBits = "";
      binaryData = "";
      Serial.println("🔄 SYNC DETECTED! Starting reception...");
      digitalWrite(LED_BUILTIN, LOW);  // LED on during reception
    }
    
    lastBit = currentBit;
    delay(BIT_HALF_DURATION);
    return false;
  }
  
  // Collect manchester encoded bits
  manchesterBits += String(currentBit);
  
  // Check if we have complete manchester pairs
  if (manchesterBits.length() >= 2) {
    String pair = manchesterBits.substring(0, 2);
    
    if (pair == "01") {
      binaryData += "0";
    } else if (pair == "10") {
      binaryData += "1";
    }
    // else invalid manchester code - ignore
    
    manchesterBits = manchesterBits.substring(2);
  }
  
  // Check if we have complete bytes (8 bits)
  if (binaryData.length() >= 8) {
    String byte = binaryData.substring(0, 8);
    int charCode = strtol(byte.c_str(), nullptr, 2);
    
    // Check for end of transmission (null or postamble)
    if (charCode == 0 || (now - syncStartTime > 30000)) {
      // End of transmission
      digitalWrite(LED_BUILTIN, HIGH);  // LED off
      syncDetected = false;
      
      if (receivedData.length() > 0) {
        return true;  // Success!
      }
      return false;
    }
    
    // Valid character
    receivedData += (char)charCode;
    binaryData = binaryData.substring(8);
    
    // Print progress
    if (receivedData.length() % 10 == 0) {
      Serial.print(".");
    }
  }
  
  // Timeout check
  if (now - syncStartTime > 60000) {
    Serial.println();
    Serial.println("⚠️  Reception timeout - resetting");
    syncDetected = false;
    receivedData = "";
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  delay(BIT_HALF_DURATION);
  return false;
}

// ========== Process Received Configuration ==========
void processReceivedConfig() {
  parseConfiguration(receivedData);
  receivedData = "";  // Clear for next reception
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
  }
}

// ========== MQTT Callback ==========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("📨 MQTT Configuration Received");
  parseConfiguration(message);
}

// ========== MQTT Reconnect ==========
void reconnectMQTT() {
  if (mqttClient.connect(mqtt_client_id)) {
    mqttClient.subscribe(mqtt_topic_config);
    String statusMsg = "{\"status\":\"online\",\"ip\":\"" + WiFi.localIP().toString() + "\",\"mode\":\"optical\"}";
    mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
  }
}

// ========== Configuration Parser ==========
void parseConfiguration(const String& jsonString) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("❌ JSON Parse Error: ");
    Serial.println(error.c_str());
    return;
  }
  
  Serial.println("✅ JSON parsed successfully");
  
  if (doc.containsKey("devices") && doc["devices"].is<JsonArray>()) {
    JsonArray devices = doc["devices"];
    for (JsonObject device : devices) {
      applyDeviceConfig(device);
    }
  } else {
    applyDeviceConfig(doc.as<JsonObject>());
  }
  
  Serial.println("✅ Configuration applied!");
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
  } else if (deviceType == "servo_angle") {
    configureServo(device);
  }
}

// ========== Device Configuration Functions ==========
void configureLED(JsonObject device) {
  config.led.pin = device["pin"] | LED_BUILTIN;
  config.led.mode = device["mode"] | "off";
  config.led.interval = device["interval"] | 1000;
  config.led.enabled = (config.led.mode != "off");
  
  pinMode(config.led.pin, OUTPUT);
  
  if (config.led.mode == "on") {
    digitalWrite(config.led.pin, config.led.pin == LED_BUILTIN ? LOW : HIGH);
  } else if (config.led.mode == "off") {
    digitalWrite(config.led.pin, config.led.pin == LED_BUILTIN ? HIGH : LOW);
  }
  
  config.led.lastToggle = millis();
}

void configureDHT(JsonObject device) {
  config.dht.pin = device["pin"] | D2;
  config.dht.mode = device["mode"] | "off";
  config.dht.interval = device["interval"] | 2000;
  config.dht.enabled = (config.dht.mode != "off");
  
  if (dhtSensor) {
    delete dhtSensor;
  }
  dhtSensor = new DHT(config.dht.pin, DHT22);
  dhtSensor->begin();
  
  Serial.println("     ✅ DHT22 sensor initialized");
}

void configureOLED(JsonObject device) {
  config.oled.mode = device["mode"] | "off";
  config.oled.text = device["text"] | "";
  config.oled.sensorType = device["sensor_type"] | "";
  config.oled.x = device["x"] | 0;
  config.oled.y = device["y"] | 0;
  config.oled.enabled = (config.oled.mode != "off");
  
  Serial.print("     Mode: ");
  Serial.println(config.oled.mode);
}

void configureBuzzer(JsonObject device) {
  config.buzzer.pin = device["pin"] | D1;
  config.buzzer.mode = device["mode"] | "off";
  config.buzzer.frequency = device["frequency"] | 1000;
  config.buzzer.duration = device["duration"] | 500;
  config.buzzer.enabled = (config.buzzer.mode != "off");
  
  pinMode(config.buzzer.pin, OUTPUT);
  
  if (config.buzzer.mode == "tone") {
    tone(config.buzzer.pin, config.buzzer.frequency, config.buzzer.duration);
  }
}

void configureDigital(JsonObject device) {
  config.digital.pin = device["pin"] | D3;
  config.digital.mode = device["mode"] | "off";
  config.digital.enabled = (config.digital.mode != "off");
  
  pinMode(config.digital.pin, OUTPUT);
  
  if (config.digital.mode == "high") {
    digitalWrite(config.digital.pin, HIGH);
  } else if (config.digital.mode == "low") {
    digitalWrite(config.digital.pin, LOW);
  }
}

void configureServo(JsonObject device) {
  int channel = device["channel"] | 0;
  int angle = device["angle"] | 90;
  
  if (channel < 0 || channel > 15) {
    Serial.println("     ⚠️  Invalid servo channel");
    return;
  }
  
  if (!servoDriver) {
    initServoDriver();
  }
  
  setServoAngle(channel, angle);
  config.servo.enabled = true;
  config.servo.channels[channel] = angle;
}

void initServoDriver() {
  servoDriver = new Adafruit_PWMServoDriver();
  Wire.begin(D2, D1);  // SDA, SCL
  servoDriver->begin();
  servoDriver->setPWMFreq(SERVO_FREQ);
  Serial.println("     ✅ PCA9685 servo driver initialized");
}

void setServoAngle(int channel, int angle) {
  if (!servoDriver) return;
  
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  servoDriver->setPWM(channel, 0, pulse);
  
  Serial.print("     Servo ");
  Serial.print(channel);
  Serial.print(" → ");
  Serial.print(angle);
  Serial.println("°");
}

// ========== Device Execution Functions ==========
void executeLED() {
  if (!config.led.enabled || config.led.mode != "blink") return;
  
  unsigned long currentMillis = millis();
  if (currentMillis - config.led.lastToggle >= config.led.interval) {
    config.led.lastToggle = currentMillis;
    config.led.state = !config.led.state;
    
    if (config.led.pin == LED_BUILTIN) {
      digitalWrite(config.led.pin, config.led.state ? LOW : HIGH);
    } else {
      digitalWrite(config.led.pin, config.led.state ? HIGH : LOW);
    }
  }
}

void executeDHT() {
  if (!config.dht.enabled || config.dht.mode != "read") return;
  
  unsigned long currentMillis = millis();
  if (currentMillis - config.dht.lastRead >= config.dht.interval) {
    config.dht.lastRead = currentMillis;
    
    if (dhtSensor) {
      float temp = dhtSensor->readTemperature();
      float hum = dhtSensor->readHumidity();
      
      if (!isnan(temp) && !isnan(hum)) {
        config.dht.lastTemp = temp;
        config.dht.lastHum = hum;
        Serial.print("🌡️ DHT22 (REAL): ");
        Serial.print(temp);
        Serial.print("°C, ");
        Serial.print(hum);
        Serial.println("%");
      }
    }
  }
}
