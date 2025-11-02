# Arduino Dynamic Configuration System

## 🎯 Overview

Complete implementation of a **wireless Arduino configuration system** that allows you to:
1. **Design circuits visually** on a web interface
2. **Deploy configurations wirelessly** to Arduino R4 via ESP8266
3. **Control devices dynamically** without re-uploading firmware
4. **Execute visual programming blocks** on the Arduino

## 📡 System Architecture

```
┌─────────────────┐      HTTP/HTTPS       ┌──────────────┐
│  Web Browser    │ ◄──────────────────► │  Flask       │
│  (Designer UI)  │      WiFi/LAN         │  Server      │
└─────────────────┘                        └──────────────┘
                                                  │
                                                  │ HTTP Polling
                                                  │ (JSON)
                                                  ▼
                                           ┌──────────────┐
                                           │  ESP8266     │
                                           │  (Bridge)    │
                                           └──────────────┘
                                                  │
                                                  │ Binary Protocol
                                                  │ Serial @ 57600
                                                  ▼
                                           ┌──────────────┐
                                           │  Arduino R4  │
                                           │  Minima      │
                                           └──────────────┘
                                                  │
                                    ┌─────────────┼─────────────┐
                                    ▼             ▼             ▼
                                 [LED]        [Servo]      [Sensors...]
```

## 🔧 Hardware Setup

### Connections

**ESP8266 (NodeMCU) ↔ Arduino R4 Minima:**
```
NodeMCU D1 (GPIO5)  →  Arduino RX0 (Pin 0)
NodeMCU D2 (GPIO4)  ←  Arduino TX1 (Pin 1)
NodeMCU GND         →  Arduino GND
```

**Devices (Examples):**
- LED: Any digital pin + GND (with 220Ω-330Ω resistor)
- Servo (MG90S): Signal → Digital pin, VCC → 5V, GND → GND (+ 100µF-470µF capacitor)
- Button: One pin → Digital pin, Other → GND (internal pullup enabled)
- Ultrasonic: Trig → Digital pin, Echo → Digital pin, VCC → 5V, GND → GND
- Potentiometer: Center → Analog pin, Sides → 5V/GND
- PIR Sensor: Signal → Digital pin, VCC → 5V, GND → GND

## 💻 Software Components

### 1. Web Interface (`arduino_designer.html` + `arduino_designer.js`)

**Features:**
- Visual device library with 13+ supported devices
- Drag-and-drop device placement
- Pin assignment visualization
- Hardware suggestions (resistors, capacitors, wiring)
- Visual programming blocks (control, logic, sensor, motor)
- Touch and mouse support
- Optimized compact layout

**Supported Devices:**
1. LED
2. Button
3. MG90S Servo
4. HC-SR04 Ultrasonic
5. DHT11 Temperature/Humidity
6. IR Receiver
7. Buzzer
8. Relay Module
9. Potentiometer
10. LDR (Light Sensor)
11. PIR Motion Sensor
12. Stepper Motor (28BYJ-48)
13. OLED Display

### 2. Flask Server (`designer_server.py`)

**Endpoints:**
- `GET /` - Serve web interface
- `POST /api/config/save` - Save configuration to JSON file
- `GET /api/config/load` - Load saved configuration
- `POST /api/deploy` - Queue configuration for ESP8266
- `GET /api/command/get` - ESP8266 polls this for commands
- `POST /api/response` - Receive responses from Arduino
- `GET /api/status` - Server status and stats

**Deployment Flow:**
1. User clicks "Deploy to Arduino"
2. Server receives config + visual program
3. Server creates deployment package (JSON)
4. ESP8266 polls `/api/command/get`
5. Server sends deployment JSON
6. ESP8266 converts to binary protocol
7. Arduino receives and executes

### 3. ESP8266 Bridge (`esp8266_programmer.cpp`)

**Purpose:** WiFi-to-Serial bridge with protocol conversion

**Functions:**
- Polls server every 2 seconds for commands
- Parses JSON deployment packages
- Converts JSON to binary protocol frames
- Sends binary frames to Arduino via Serial
- Forwards Arduino responses back to server

**Binary Protocol:**
```
Frame Format:
[0xAA][CMD_ID][LEN_H][LEN_L][PAYLOAD...][CRC_H][CRC_L]

Commands:
- 0x01: PING
- 0x02: PONG
- 0x22: CONFIG_DEPLOY (CMD_SENSOR_CONFIG)
- 0x54: STRING_DATA (device actions)
- 0x20: STATE_REQUEST
```

### 4. Arduino Dynamic System (`arduino_target.cpp` + `DynamicArduino.h`)

**Core Classes:**

#### Base Device Class
```cpp
class Device {
    virtual void begin()    // Initialize hardware
    virtual void update()   // Called every 10ms
    virtual void execute()  // Execute actions
    virtual JsonObject getState()  // Return state
}
```

#### Implemented Devices
- **LEDDevice**: Set on/off, brightness, blink patterns
- **ButtonDevice**: Debounced input with press detection
- **ServoDevice**: Angle control with smooth movement and sweep
- **UltrasonicDevice**: Distance measurement (auto-polling)
- **BuzzerDevice**: Tone generation with duration
- **RelayDevice**: On/off control
- **PotentiometerDevice**: Analog reading with auto-polling
- **LDRDevice**: Light level sensing
- **PIRDevice**: Motion detection with trigger events

**Dynamic Loading:**
1. Arduino receives JSON configuration
2. Parses device list with ArduinoJson
3. Creates device objects dynamically
4. Calls `begin()` on each device
5. Updates all devices in main loop (10ms interval)

**Example Configuration JSON:**
```json
{
  "devices": [
    {
      "id": "led_1",
      "type": "LED",
      "pin": "D13"
    },
    {
      "id": "servo_1",
      "type": "MG90S Servo",
      "pin": "D9"
    },
    {
      "id": "button_1",
      "type": "Button",
      "pin": "D2"
    }
  ],
  "visualProgram": [
    // Visual programming blocks (to be executed)
  ]
}
```

## 🚀 Usage Instructions

### Step 1: Upload Firmware

**Upload ESP8266:**
```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH
~/.platformio/penv/bin/platformio run -e esp8266_programmer -t upload
```

**Upload Arduino R4:**
```bash
~/.platformio/penv/bin/platformio run -e uno_r4_minima -t upload
```

### Step 2: Start Server

```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH/server
python3 designer_server.py
```

Server will start at: `http://localhost:5001`

### Step 3: Design Circuit

1. Open browser: `http://localhost:5001/arduino_designer.html`
2. Drag devices from library to Arduino board
3. Assign pins by dropping on pin slots
4. View hardware suggestions (resistors, capacitors)
5. Add visual programming blocks (optional)
6. Click "Save Configuration"

### Step 4: Deploy

1. Click "Deploy to Arduino"
2. Monitor ESP8266 Serial (115200 baud):
   - Should show "📥 Server → ESP"
   - Then "✅ Sent to Arduino via binary protocol"
3. Monitor Arduino Serial (115200 baud):
   - Should show ">>> RECEIVED CONFIG DEPLOYMENT <<<"
   - Lists all devices being created
   - Shows "✓ Device initialized" for each

### Step 5: Verify

Arduino Serial output should look like:
```
═══════════════════════════════════════
   ARDUINO R4 DYNAMIC SYSTEM
═══════════════════════════════════════
Build: Jan 1 2025 12:00:00
Features:
  ✓ Dynamic device loading
  ✓ Visual programming support
  ✓ JSON configuration over ESP8266
═══════════════════════════════════════

✓ Arduino R4 Dynamic System Ready
✓ Waiting for configuration from ESP8266...

>>> RECEIVED CONFIG DEPLOYMENT <<<

=== LOADING CONFIGURATION ===
Creating 3 devices:
  - LED (led_1) on pin 13
    ✓ Device initialized
  - MG90S Servo (servo_1) on pin 9
    ✓ Device initialized
  - Button (button_1) on pin 2
    ✓ Device initialized
=== CONFIGURATION LOADED ===

✓ Configuration deployed successfully
```

## 📊 Device Control API

Once configured, you can send device actions:

**LED Control:**
```json
{
  "deviceId": "led_1",
  "action": "set",
  "params": { "state": true }
}
```

**Servo Control:**
```json
{
  "deviceId": "servo_1",
  "action": "setAngle",
  "params": { "angle": 90 }
}
```

**Servo Sweep:**
```json
{
  "deviceId": "servo_1",
  "action": "sweep",
  "params": { "min": 0, "max": 180 }
}
```

## 🔍 Debugging

**ESP8266 Debug Output:**
```
╔════════════════════════════════════════╗
║  ESP8266 Configuration Bridge         ║
╚════════════════════════════════════════╝
Mode: JSON to Binary Protocol
📡 Connecting to WiFi: muditatrey12345
..........
✅ WiFi Connected!
   IP: 192.168.1.100
   Server: https://6fbx0j5c-5001.inc1.devtunnels.ms
🔄 Bridge active. Polling server...
```

**Arduino Debug Output:**
Monitor on Serial (115200 baud) to see:
- Configuration reception
- Device initialization
- Command processing
- State updates

## 📦 Dependencies

**platformio.ini:**
```ini
[env:uno_r4_minima]
lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
    arduino-libraries/Servo@^1.2.1

[env:esp8266_programmer]
lib_deps = 
    bblanchon/ArduinoJson@^6.21.3
```

## ⚡ Performance

- Configuration deployment: ~500ms
- Device update cycle: 10ms
- ESP8266 polling: 2 seconds
- Maximum devices: 20
- Maximum payload: 4KB JSON

## 🎨 Visual Programming (Future)

Visual blocks are parsed but not yet executed. Planned features:
- If/else conditions
- Loops (while, for)
- Logic gates (AND, OR, NOT)
- Sensor reads
- Motor control
- Delays and timing

Example block structure:
```json
{
  "type": "control",
  "subtype": "if",
  "condition": "button_1.pressed",
  "action": "led_1.set(true)"
}
```

## 🛠️ Troubleshooting

**Problem:** ESP8266 can't connect to WiFi
- Check SSID and password in `esp8266_programmer.cpp`
- Ensure 2.4GHz WiFi (ESP8266 doesn't support 5GHz)

**Problem:** Arduino not receiving config
- Verify wiring: D1→RX0, D2→TX1, GND→GND
- Check baud rate (57600) matches on both sides
- Monitor both serial ports simultaneously

**Problem:** Device not working
- Check pin assignment matches hardware
- Verify device is in supported list
- Check hardware connections (resistors, power)
- Monitor Serial for initialization errors

**Problem:** Server not deploying
- Check Flask server is running on port 5001
- Verify ESP8266 serverUrl points to correct address
- Check network connectivity

## 📝 License

This is a custom educational project for Arduino R4 Minima with ESP8266 WiFi bridge.

## 🎉 Credits

Built with:
- Arduino R4 Minima (Renesas RA4M1)
- ESP8266 NodeMCU
- Flask (Python)
- ArduinoJson
- HTML5/CSS3/JavaScript
