/*
 * Dynamic Arduino Configuration System
 * 
 * Receives JSON configuration from server and executes dynamically
 * Supports all devices from the visual designer
 */

#ifndef DYNAMIC_ARDUINO_H
#define DYNAMIC_ARDUINO_H

#include <Arduino.h>
#include <Servo.h>

// ArduinoJson for parsing configuration
// Install via: platformio lib install "bblanchon/ArduinoJson@^6.21.3"
#include <ArduinoJson.h>

// Maximum devices supported
#define MAX_DEVICES 20
#define MAX_PROGRAM_BLOCKS 50

// Device types
enum DeviceType {
    DEVICE_NONE = 0,
    DEVICE_LED,
    DEVICE_BUTTON,
    DEVICE_SERVO,
    DEVICE_ULTRASONIC,
    DEVICE_DHT11,
    DEVICE_IR_RECEIVER,
    DEVICE_BUZZER,
    DEVICE_RELAY,
    DEVICE_POTENTIOMETER,
    DEVICE_LDR,
    DEVICE_PIR,
    DEVICE_STEPPER,
    DEVICE_OLED
};

// Base device class
class Device {
protected:
    String id;
    DeviceType type;
    uint8_t pin;
    bool enabled;
    
public:
    Device(const String& deviceId, DeviceType deviceType, uint8_t devicePin) 
        : id(deviceId), type(deviceType), pin(devicePin), enabled(true) {}
    
    virtual ~Device() {}
    
    virtual void begin() = 0;
    virtual void update() = 0;
    virtual void execute(const String& action, const JsonObject& params) = 0;
    virtual JsonObject getState(JsonDocument& doc) = 0;
    
    String getId() const { return id; }
    DeviceType getType() const { return type; }
    uint8_t getPin() const { return pin; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool state) { enabled = state; }
};

// LED Device
class LEDDevice : public Device {
private:
    bool state;
    uint8_t brightness;
    bool blinking;
    unsigned long blinkInterval;
    unsigned long lastBlink;
    
public:
    LEDDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_LED, pin), state(false), brightness(255), 
          blinking(false), blinkInterval(500), lastBlink(0) {}
    
    void begin() override {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    
    void update() override {
        if (blinking && enabled) {
            if (millis() - lastBlink >= blinkInterval) {
                state = !state;
                analogWrite(pin, state ? brightness : 0);
                lastBlink = millis();
            }
        }
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "set") {
            state = params["state"] | false;
            blinking = false;
            digitalWrite(pin, state ? HIGH : LOW);
        } else if (action == "setBrightness") {
            brightness = params["brightness"] | 255;
            if (state) {
                analogWrite(pin, brightness);
            }
        } else if (action == "blink") {
            blinking = true;
            blinkInterval = params["interval"] | 500;
            lastBlink = millis();
        } else if (action == "stopBlink") {
            blinking = false;
            digitalWrite(pin, state ? HIGH : LOW);
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "LED";
        obj["state"] = state;
        obj["brightness"] = brightness;
        obj["blinking"] = blinking;
        return obj;
    }
};

// Button Device
class ButtonDevice : public Device {
private:
    bool currentState;
    bool lastState;
    bool pressed;
    unsigned long lastDebounce;
    const unsigned long debounceDelay = 50;
    
public:
    ButtonDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_BUTTON, pin), currentState(false), 
          lastState(false), pressed(false), lastDebounce(0) {}
    
    void begin() override {
        pinMode(pin, INPUT_PULLUP);
        currentState = digitalRead(pin) == LOW;
        lastState = currentState;
    }
    
    void update() override {
        if (!enabled) return;
        
        bool reading = digitalRead(pin) == LOW;
        
        if (reading != lastState) {
            lastDebounce = millis();
        }
        
        if ((millis() - lastDebounce) > debounceDelay) {
            if (reading != currentState) {
                currentState = reading;
                if (currentState) {
                    pressed = true;
                }
            }
        }
        
        lastState = reading;
    }
    
    void execute(const String& action, const JsonObject& params) override {
        // Buttons don't have actions, only state reading
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Button";
        obj["pressed"] = pressed;
        obj["state"] = currentState;
        
        // Clear pressed flag after reading
        pressed = false;
        
        return obj;
    }
    
    bool isPressed() { 
        bool p = pressed;
        pressed = false;
        return p;
    }
};

// Servo Device
class ServoDevice : public Device {
private:
    Servo servo;
    uint8_t currentAngle;
    uint8_t targetAngle;
    bool sweeping;
    uint8_t sweepMin;
    uint8_t sweepMax;
    unsigned long lastSweepUpdate;
    int sweepDirection;
    
public:
    ServoDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_SERVO, pin), currentAngle(90), targetAngle(90),
          sweeping(false), sweepMin(0), sweepMax(180), lastSweepUpdate(0), sweepDirection(1) {}
    
    void begin() override {
        servo.attach(pin);
        servo.write(90);
    }
    
    void update() override {
        if (!enabled) return;
        
        if (sweeping) {
            if (millis() - lastSweepUpdate >= 15) {
                currentAngle += sweepDirection;
                
                if (currentAngle >= sweepMax) {
                    currentAngle = sweepMax;
                    sweepDirection = -1;
                } else if (currentAngle <= sweepMin) {
                    currentAngle = sweepMin;
                    sweepDirection = 1;
                }
                
                servo.write(currentAngle);
                lastSweepUpdate = millis();
            }
        } else if (currentAngle != targetAngle) {
            // Smooth movement
            if (currentAngle < targetAngle) {
                currentAngle++;
            } else {
                currentAngle--;
            }
            servo.write(currentAngle);
            delay(15);
        }
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "setAngle") {
            targetAngle = constrain(params["angle"] | 90, 0, 180);
            sweeping = false;
        } else if (action == "sweep") {
            sweeping = true;
            sweepMin = params["min"] | 0;
            sweepMax = params["max"] | 180;
            sweepDirection = 1;
            lastSweepUpdate = millis();
        } else if (action == "stopSweep") {
            sweeping = false;
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Servo";
        obj["angle"] = currentAngle;
        obj["sweeping"] = sweeping;
        return obj;
    }
};

// Ultrasonic Sensor Device
class UltrasonicDevice : public Device {
private:
    uint8_t trigPin;
    uint8_t echoPin;
    float lastDistance;
    unsigned long lastMeasurement;
    const unsigned long measurementInterval = 100;
    
public:
    UltrasonicDevice(const String& id, uint8_t trig, uint8_t echo) 
        : Device(id, DEVICE_ULTRASONIC, trig), trigPin(trig), echoPin(echo),
          lastDistance(0), lastMeasurement(0) {}
    
    void begin() override {
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(trigPin, LOW);
    }
    
    void update() override {
        if (!enabled) return;
        
        if (millis() - lastMeasurement >= measurementInterval) {
            lastDistance = measure();
            lastMeasurement = millis();
        }
    }
    
    float measure() {
        digitalWrite(trigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        long duration = pulseIn(echoPin, HIGH, 30000);
        float distance = duration * 0.034 / 2.0;
        
        return (distance > 0 && distance < 400) ? distance : lastDistance;
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "measure") {
            lastDistance = measure();
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Ultrasonic";
        obj["distance"] = lastDistance;
        return obj;
    }
    
    float getDistance() const { return lastDistance; }
};

// Buzzer Device
class BuzzerDevice : public Device {
private:
    bool active;
    unsigned int frequency;
    unsigned long duration;
    unsigned long startTime;
    
public:
    BuzzerDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_BUZZER, pin), active(false), frequency(1000), duration(0), startTime(0) {}
    
    void begin() override {
        pinMode(pin, OUTPUT);
        noTone(pin);
    }
    
    void update() override {
        if (!enabled) return;
        
        if (active && duration > 0) {
            if (millis() - startTime >= duration) {
                noTone(pin);
                active = false;
            }
        }
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "tone") {
            frequency = params["frequency"] | 1000;
            duration = params["duration"] | 0;
            
            tone(pin, frequency);
            active = true;
            startTime = millis();
        } else if (action == "stop") {
            noTone(pin);
            active = false;
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Buzzer";
        obj["active"] = active;
        obj["frequency"] = frequency;
        return obj;
    }
};

// Relay Device
class RelayDevice : public Device {
private:
    bool state;
    
public:
    RelayDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_RELAY, pin), state(false) {}
    
    void begin() override {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
    
    void update() override {
        // Relays are set and forget
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "set") {
            state = params["state"] | false;
            digitalWrite(pin, state ? HIGH : LOW);
        } else if (action == "toggle") {
            state = !state;
            digitalWrite(pin, state ? HIGH : LOW);
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Relay";
        obj["state"] = state;
        return obj;
    }
};

// Potentiometer Device
class PotentiometerDevice : public Device {
private:
    int rawValue;
    int lastValue;
    unsigned long lastRead;
    const unsigned long readInterval = 50;
    
public:
    PotentiometerDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_POTENTIOMETER, pin), rawValue(0), lastValue(0), lastRead(0) {}
    
    void begin() override {
        pinMode(pin, INPUT);
        rawValue = analogRead(pin);
        lastValue = rawValue;
    }
    
    void update() override {
        if (!enabled) return;
        
        if (millis() - lastRead >= readInterval) {
            rawValue = analogRead(pin);
            lastRead = millis();
        }
    }
    
    void execute(const String& action, const JsonObject& params) override {
        // Potentiometers are read-only
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "Potentiometer";
        obj["value"] = rawValue;
        obj["percent"] = map(rawValue, 0, 1023, 0, 100);
        return obj;
    }
    
    int getValue() const { return rawValue; }
    int getPercent() const { return map(rawValue, 0, 1023, 0, 100); }
};

// LDR (Light Sensor) Device
class LDRDevice : public Device {
private:
    int lightLevel;
    unsigned long lastRead;
    const unsigned long readInterval = 100;
    
public:
    LDRDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_LDR, pin), lightLevel(0), lastRead(0) {}
    
    void begin() override {
        pinMode(pin, INPUT);
        lightLevel = analogRead(pin);
    }
    
    void update() override {
        if (!enabled) return;
        
        if (millis() - lastRead >= readInterval) {
            lightLevel = analogRead(pin);
            lastRead = millis();
        }
    }
    
    void execute(const String& action, const JsonObject& params) override {
        // LDR is read-only
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "LDR";
        obj["light"] = lightLevel;
        obj["percent"] = map(lightLevel, 0, 1023, 0, 100);
        return obj;
    }
    
    int getLightLevel() const { return lightLevel; }
};

// PIR Motion Sensor Device
class PIRDevice : public Device {
private:
    bool motionDetected;
    bool lastState;
    unsigned long lastTrigger;
    
public:
    PIRDevice(const String& id, uint8_t pin) 
        : Device(id, DEVICE_PIR, pin), motionDetected(false), lastState(false), lastTrigger(0) {}
    
    void begin() override {
        pinMode(pin, INPUT);
        // PIR sensors need warm-up time
        delay(2000);
    }
    
    void update() override {
        if (!enabled) return;
        
        bool currentState = digitalRead(pin) == HIGH;
        
        if (currentState && !lastState) {
            motionDetected = true;
            lastTrigger = millis();
        }
        
        lastState = currentState;
    }
    
    void execute(const String& action, const JsonObject& params) override {
        if (action == "reset") {
            motionDetected = false;
        }
    }
    
    JsonObject getState(JsonDocument& doc) override {
        JsonObject obj = doc.createNestedObject();
        obj["type"] = "PIR";
        obj["motion"] = motionDetected;
        obj["state"] = lastState;
        
        // Clear motion flag after reading
        bool motion = motionDetected;
        motionDetected = false;
        obj["triggered"] = motion;
        
        return obj;
    }
    
    bool isMotionDetected() {
        bool motion = motionDetected;
        motionDetected = false;
        return motion;
    }
};

#endif // DYNAMIC_ARDUINO_H
