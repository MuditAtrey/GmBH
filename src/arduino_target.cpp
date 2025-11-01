// Arduino R4 Minima - Command Receiver
// Receives JSON commands via serial and executes them
#include <Arduino.h>

#define LED_PIN LED_BUILTIN

// Blink task state
struct {
    bool active;
    unsigned int duration;
    unsigned long lastToggle;
    bool state;
} blinkTask = {false, 500, 0, false};

// Command buffer
String commandBuffer = "";

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    delay(1000);
    
    // Send ready signal
    Serial.println("{\"status\":\"ready\",\"device\":\"Arduino R4 Minima\",\"version\":\"1.0\"}");
}

void processCommand(String cmd) {
    cmd.trim();
    
    // Simple JSON parsing (no library needed for simple commands)
    if (cmd.indexOf("\"type\":\"blink\"") > 0) {
        // Extract duration
        int durStart = cmd.indexOf("\"duration\":") + 11;
        int durEnd = cmd.indexOf(",", durStart);
        if (durEnd == -1) durEnd = cmd.indexOf("}", durStart);
        
        String durStr = cmd.substring(durStart, durEnd);
        unsigned int duration = durStr.toInt();
        
        if (duration >= 50 && duration <= 5000) {
            blinkTask.duration = duration;
            blinkTask.active = true;
            blinkTask.lastToggle = millis();
            blinkTask.state = true;
            digitalWrite(LED_PIN, HIGH);
            
            Serial.print("{\"status\":\"blink_started\",\"duration\":");
            Serial.print(duration);
            Serial.println("}");
        } else {
            Serial.println("{\"error\":\"Invalid duration (50-5000ms)\"}");
        }
    }
    else if (cmd.indexOf("\"type\":\"led_on\"") > 0) {
        blinkTask.active = false;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("{\"status\":\"led_on\"}");
    }
    else if (cmd.indexOf("\"type\":\"led_off\"") > 0) {
        blinkTask.active = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("{\"status\":\"led_off\"}");
    }
    else if (cmd.indexOf("\"type\":\"stop\"") > 0) {
        blinkTask.active = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("{\"status\":\"stopped\"}");
    }
    else if (cmd.indexOf("\"type\":\"ping\"") > 0) {
        Serial.print("{\"status\":\"pong\",\"uptime\":");
        Serial.print(millis());
        Serial.println("}");
    }
    else if (cmd.indexOf("\"type\":\"status\"") > 0) {
        Serial.print("{\"status\":\"ok\",\"uptime\":");
        Serial.print(millis());
        Serial.print(",\"blinking\":");
        Serial.print(blinkTask.active ? "true" : "false");
        Serial.print(",\"duration\":");
        Serial.print(blinkTask.duration);
        Serial.print(",\"led_state\":");
        Serial.print(digitalRead(LED_PIN) ? "true" : "false");
        Serial.println("}");
    }
    else {
        Serial.println("{\"error\":\"Unknown command\"}");
    }
}

void updateBlink() {
    if (!blinkTask.active) return;
    
    unsigned long now = millis();
    if (now - blinkTask.lastToggle >= blinkTask.duration) {
        blinkTask.state = !blinkTask.state;
        digitalWrite(LED_PIN, blinkTask.state ? HIGH : LOW);
        blinkTask.lastToggle = now;
    }
}

void loop() {
    // Read commands from serial
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (commandBuffer.length() > 0) {
                processCommand(commandBuffer);
                commandBuffer = "";
            }
        } else {
            commandBuffer += c;
        }
    }
    
    // Update blink task
    updateBlink();
}