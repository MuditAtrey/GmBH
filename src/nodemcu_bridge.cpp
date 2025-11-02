/*
 * NodeMCU v3 - MQTT to Serial Bridge
 * 
 * This firmware connects to WiFi and MQTT, then forwards any received
 * configurations to the Arduino R4 Minima over Serial (UART).
 * 
 * CRITICAL: Use a logic level shifter between NodeMCU (3.3V) and R4 (5V)!
 * 
 * Connections (with level shifter):
 *   NodeMCU TX → LV Side → HV Side → R4 RX
 *   NodeMCU RX ← LV Side ← HV Side ← R4 TX
 *   NodeMCU GND ────────────────────→ R4 GND
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ========== WiFi Configuration ==========
const char* ssid = "muditatrey12345";
const char* password = "muditmudit";

// ========== MQTT Configuration ==========
const char* mqtt_server = "10.147.66.174";  // Your local IP or broker.hivemq.com
const int mqtt_port = 1883;
const char* mqtt_topic_config = "arduino_designer/r4/config";
const char* mqtt_topic_status = "arduino_designer/nodemcu/status";
const char* mqtt_client_id = "nodemcu_bridge_001";

// ========== Objects ==========
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ========== Timing ==========
unsigned long lastReconnectAttempt = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long reconnectInterval = 5000;    // Try reconnect every 5 seconds
const unsigned long statusInterval = 30000;      // Status update every 30 seconds

// ========== Statistics ==========
unsigned long messagesReceived = 0;
unsigned long messagesForwarded = 0;

// ========== Function Declarations ==========
void setupWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);
boolean reconnectMQTT();

// ========== Setup ==========
void setup() {
  // Initialize Serial for communication with R4
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║  NodeMCU MQTT Bridge - Starting...        ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize built-in LED for status indication
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off (inverted on NodeMCU)
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);  // Increase buffer for larger JSON messages
  
  Serial.println("✅ Setup complete!");
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
    Serial.println("   Will retry in loop...");
    Serial.println();
  }
}

// ========== MQTT Callback ==========
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  messagesReceived++;
  
  Serial.println();
  Serial.println("📨 MQTT Message Received:");
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
  
  // Forward to R4 over Serial
  Serial.println();
  Serial.println("📤 Forwarding to Arduino R4...");
  Serial.println(message);  // This line goes to R4!
  
  messagesForwarded++;
  
  // Blink LED to show activity
  digitalWrite(LED_BUILTIN, LOW);   // LED on
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off
  
  Serial.println("✅ Message forwarded successfully");
  Serial.print("   Total received: ");
  Serial.print(messagesReceived);
  Serial.print(", Total forwarded: ");
  Serial.println(messagesForwarded);
  Serial.println("----------------------------------------");
  Serial.println();
}

// ========== MQTT Connect ==========
boolean reconnectMQTT() {
  Serial.print("🔄 Attempting MQTT connection to ");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.print(mqtt_port);
  Serial.print("... ");
  
  if (mqttClient.connect(mqtt_client_id)) {
    Serial.println("✅ Connected!");
    
    // Subscribe to config topic
    mqttClient.subscribe(mqtt_topic_config);
    Serial.print("📡 Subscribed to: ");
    Serial.println(mqtt_topic_config);
    
    // Publish status
    String statusMsg = "{\"status\":\"online\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
    
    Serial.println();
    return true;
  } else {
    Serial.print("❌ Failed, rc=");
    Serial.println(mqttClient.state());
    Serial.println("   Will retry in 5 seconds...");
    Serial.println();
    return false;
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
    unsigned long now = millis();
    if (now - lastReconnectAttempt > reconnectInterval) {
      lastReconnectAttempt = now;
      if (reconnectMQTT()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // MQTT is connected, process incoming messages
    mqttClient.loop();
  }
  
  // Periodic status update
  unsigned long now = millis();
  if (now - lastStatusUpdate > statusInterval) {
    lastStatusUpdate = now;
    
    if (mqttClient.connected()) {
      String statusMsg = "{\"status\":\"online\",\"ip\":\"" + WiFi.localIP().toString() + 
                        "\",\"uptime\":" + String(millis()/1000) + 
                        ",\"messages\":" + String(messagesReceived) + "}";
      mqttClient.publish(mqtt_topic_status, statusMsg.c_str());
      
      Serial.println("📊 Status update sent to MQTT");
      Serial.print("   Uptime: ");
      Serial.print(millis() / 1000);
      Serial.println(" seconds");
      Serial.print("   Messages: ");
      Serial.println(messagesReceived);
      Serial.println();
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}
