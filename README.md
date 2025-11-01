# Arduino R4 Minima Remote Programming System
## Complete Setup and Usage Guide

---

## 🎯 System Architecture

**NEW ARCHITECTURE:**
```
[Your PC] ← WiFi → [ESP8266] ← Serial → [Arduino R4 Minima]
    ↑                  ↓
  Web Server      HTTP Client
  (Flask)        (Polls for updates)
```

### Components:
1. **PC Server** (Flask) - Hosts firmware files and provides REST API
2. **ESP8266** (NodeMCU) - Polls server, downloads firmware, programs Arduino
3. **Arduino R4 Minima** - Target device to be programmed

---

## 📦 What You'll Need

### Hardware:
- NodeMCU ESP8266
- Arduino R4 Minima
- 1µF capacitor (for reset control)
- 4.6kΩ resistor (pull-down on Arduino RESET)
- Jumper wires
- USB cables for initial programming

### Software:
- Python 3.7+ (for PC server)
- PlatformIO (for firmware builds)
- This project repository

---

## 🔌 Hardware Connections

```
ESP8266 (NodeMCU)          Arduino R4 Minima
─────────────────          ─────────────────
    TX (GPIO1)     ──────→    RX (D0)
    RX (GPIO3)     ←──────    TX (D1)
    D1 (GPIO5)     ──┤├──→    RESET (via 1µF capacitor)
    GND            ────────    GND
    
    RESET          ─[4.6kΩ]─  GND (pull-down resistor)
```

**Important Notes:**
- The capacitor creates a pulse on RESET when D1 goes HIGH
- The pull-down resistor keeps RESET stable when D1 is LOW
- Both devices must share a common ground
- Power each device via its own USB connection (don't cross power!)

---

## 🚀 Setup Instructions

### Step 1: Install Python Dependencies

```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH/server
pip3 install -r requirements.txt
```

### Step 2: Configure ESP8266

Edit `src/esp8266_programmer.cpp` and update these lines:

```cpp
// Line 9-10: Your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Line 13: Your PC's IP address on the network
const char* serverHost = "192.168.1.100";  // Replace with your PC's IP
```

**To find your PC's IP address:**
- macOS: `ifconfig | grep "inet " | grep -v 127.0.0.1`
- Linux: `ip addr show`
- Windows: `ipconfig`

### Step 3: Build and Upload ESP8266 Firmware

```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH

# Build ESP8266 firmware
~/.platformio/penv/bin/pio run -e esp8266_programmer

# Upload to ESP8266 (connect via USB)
~/.platformio/penv/bin/pio run -e esp8266_programmer -t upload

# Monitor serial output to verify connection
~/.platformio/penv/bin/pio device monitor -e esp8266_programmer
```

You should see:
```
=== ESP8266 Arduino Remote Programmer ===
Device ID: AABBCCDDEEFF
WiFi connected!
IP Address: 192.168.1.150
Server: http://192.168.1.100:5000
Waiting for firmware updates...
```

### Step 4: Build Arduino Firmware

```bash
# Build Arduino R4 firmware (.bin file will be created)
~/.platformio/penv/bin/pio run -e uno_r4_minima
```

The firmware binary will be saved to:
`.pio/build/uno_r4_minima/firmware.bin`

### Step 5: Start PC Server

```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH/server
python3 firmware_server.py
```

You should see:
```
============================================================
🚀 Arduino R4 Minima Firmware Server
============================================================
📁 Firmware directory: /Users/muditatrey/Documents/PlatformIO/Projects/GmBH/server/firmware
🌐 Server starting on http://0.0.0.0:5000
💡 Web interface: http://localhost:5000
🔌 API endpoint: http://<your-pc-ip>:5000/api/firmware/check
============================================================
```

---

## 📤 Uploading Firmware Remotely

### Method 1: Web Interface

1. Open browser to `http://localhost:5000`
2. Click "Choose File" and select `.pio/build/uno_r4_minima/firmware.bin`
3. Enter version number (e.g., "1.0.0")
4. Click "Upload Firmware"
5. Wait 30 seconds - ESP8266 will automatically detect and download the update!

### Method 2: Command Line (curl)

```bash
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH

curl -X POST http://localhost:5000/api/firmware/upload \
  -F "firmware=@.pio/build/uno_r4_minima/firmware.bin" \
  -F "version=1.0.0" \
  -F "description=LED blink test"
```

---

## 🔄 How It Works

1. **ESP8266 polls PC server** every 30 seconds via `/api/firmware/check`
2. **Server responds** with firmware metadata (version, size, MD5 hash)
3. **ESP8266 compares** MD5 hash with current firmware
4. **If different**, ESP8266 downloads firmware via `/api/firmware/download`
5. **Firmware saved** to ESP8266's LittleFS filesystem
6. **ESP8266 enters bootloader mode** on Arduino (double-tap reset)
7. **Firmware sent** to Arduino over serial connection
8. **Arduino reset** to run new firmware
9. **Status reported** back to PC server

---

## 🔍 Monitoring

### Watch ESP8266 Serial Monitor
```bash
~/.platformio/penv/bin/pio device monitor -e esp8266_programmer
```

### Watch PC Server Logs
The Flask server shows real-time logs of ESP8266 activity:
```
📡 Device [AABBCCDDEEFF]: downloading - New firmware detected
📡 Device [AABBCCDDEEFF]: flashing - Starting flash process
📡 Device [AABBCCDDEEFF]: success - Firmware update complete
```

---

## ⚠️ IMPORTANT LIMITATIONS

### Arduino R4 Minima Programming Challenge

**The current implementation has a CRITICAL limitation:**

The Arduino R4 Minima uses a **Renesas RA4M1 ARM Cortex-M4** processor, NOT the traditional AVR architecture. This means:

❌ **STK500 protocol won't work** (traditional Arduino bootloader)
❌ **AVRdude won't work** (AVR programming tool)
❌ **Simple serial programming won't work** (requires specific bootloader)

✅ **What the current system CAN do:**
- Connect ESP8266 to WiFi
- Poll PC server for updates
- Download firmware files
- Store firmware on ESP8266
- Reset Arduino R4
- Send data over serial to Arduino

❌ **What it CANNOT do (yet):**
- Actually FLASH the firmware to Arduino R4's program memory
- Use the Arduino R4's built-in bootloader remotely

### Solutions for Actual Programming:

#### Option 1: Use Arduino R4's Built-in WiFi (RECOMMENDED)
The Arduino R4 Minima has a built-in Renesas DA16200 WiFi module. You can:
1. Add WiFi and OTA update capabilities directly to your Arduino code
2. Use Arduino's `WiFiS3` library and `OTAUpdate` library
3. Arduino updates itself without needing the ESP8266

**Example Arduino OTA code:**
```cpp
#include <WiFiS3.h>
#include <OTAUpdate.h>

void setup() {
  WiFi.begin("SSID", "PASSWORD");
  OTAUpdate.begin();  // Enable OTA updates
}

void loop() {
  OTAUpdate.poll();  // Check for updates
  // Your code here
}
```

#### Option 2: Switch to AVR Arduino (WORKS WITH CURRENT SYSTEM)
Use Arduino Uno, Nano, or Mega instead:
- These use AVR processors with STK500 bootloader
- Can be programmed over serial using standard protocols
- The ESP8266 programmer code would work with minimal modifications

#### Option 3: Implement bossac Protocol (ADVANCED)
The Arduino R4 uses the `bossac` tool for programming:
- Would require porting bossac to ESP8266 (very complex)
- Or using an external SWD programmer connected to ESP8266
- Beyond the scope of this project

---

## 🛠️ Troubleshooting

### ESP8266 Won't Connect to WiFi
- Check SSID and password in `esp8266_programmer.cpp`
- Ensure ESP8266 is in range of router
- Use 2.4GHz WiFi (ESP8266 doesn't support 5GHz)

### ESP8266 Can't Reach Server
- Verify PC's firewall allows port 5000
- Ensure PC and ESP8266 are on same network
- Test server with: `curl http://<pc-ip>:5000/api/firmware/check`
- Check serverHost IP address in ESP8266 code

### Firmware Upload Fails
- Check file size (must fit in ESP8266's LittleFS)
- Verify .bin file is correct format
- Try re-uploading via web interface

### Arduino Not Responding
- Check physical connections (TX↔RX, GND)
- Verify capacitor and resistor values
- Test reset by pressing Arduino's reset button
- Check baud rate matches (115200 for both)

---

## 📁 Project Structure

```
GmBH/
├── platformio.ini              # PlatformIO configuration
├── src/
│   ├── esp8266_programmer.cpp  # ESP8266 client code
│   └── arduino_target.cpp      # Arduino test firmware
├── server/
│   ├── firmware_server.py      # Python Flask server
│   ├── requirements.txt        # Python dependencies
│   └── firmware/               # (created automatically)
│       ├── firmware.bin        # Latest uploaded firmware
│       └── metadata.json       # Firmware metadata
```

---

## 🔧 Development Workflow

### Typical Update Cycle:

1. **Modify Arduino code** in `src/arduino_target.cpp`
2. **Build firmware:**
   ```bash
   ~/.platformio/penv/bin/pio run -e uno_r4_minima
   ```
3. **Upload to server** via web interface at `http://localhost:5000`
4. **Wait ~30 seconds** for ESP8266 to detect and download
5. **Monitor progress** in ESP8266 serial monitor
6. **Verify** new code is running on Arduino

---

## 🎓 Technical Details

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web interface |
| `/api/firmware/check` | GET | Check for firmware availability |
| `/api/firmware/download` | GET | Download firmware binary |
| `/api/firmware/upload` | POST | Upload new firmware |
| `/api/firmware/metadata` | GET | Get firmware details |
| `/api/firmware/delete` | DELETE | Delete current firmware |
| `/api/device/report` | POST | ESP8266 status reporting |

### Firmware Check Response
```json
{
  "available": true,
  "version": "1.0.0",
  "size": 33928,
  "md5": "a1b2c3d4e5f6...",
  "timestamp": "2025-11-01T10:30:00",
  "description": "LED blink test"
}
```

---

## 📝 Next Steps

### To Enable Actual Arduino R4 Programming:

1. **Research Renesas RA4M1 bootloader** protocol
2. **Implement bossac commands** for ESP8266
3. **Or add WiFi to Arduino** and use native OTA updates
4. **Or switch to AVR Arduino** for simpler programming

### Enhancements:
- [ ] Add firmware rollback capability
- [ ] Implement firmware signing/verification
- [ ] Add multiple device support
- [ ] Create mobile app for firmware upload
- [ ] Add scheduled update times
- [ ] Implement firmware A/B partitions

---

## 📚 References

- [Arduino R4 Minima Documentation](https://docs.arduino.cc/hardware/uno-r4-minima)
- [ESP8266 Arduino Core](https://arduino-esp8266.readthedocs.io/)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [bossac Tool](https://github.com/shumatech/BOSSA)
- [Flask Documentation](https://flask.palletsprojects.com/)

---

## 📄 License

This project is for educational purposes. Use at your own risk.

**⚠️ Warning:** Flashing firmware can brick your devices if done incorrectly. Always keep a working backup firmware and USB programmer available.
