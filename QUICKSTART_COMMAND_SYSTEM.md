# Arduino R4 Command & Control System

## 🎯 What This Does

Send commands from your PC to control an Arduino R4 Minima over WiFi. Change LED blink speeds, turn LED on/off, and more - all remotely!

## 🚀 Quick Start (3 Steps)

### Step 1: Start the Server
```bash
cd server
python3 firmware_server.py
```

### Step 2: Upload to Arduino
```bash
~/.platformio/penv/bin/pio run -e uno_r4_minima -t upload
```

### Step 3: Upload to ESP8266
```bash
~/.platformio/penv/bin/pio run -e esp8266_programmer -t upload
```

## 🎮 Use It!

Open browser to: **http://localhost:5000**

Try these commands:
- **Very Fast** - LED blinks at 100ms
- **Normal** - LED blinks at 500ms  
- **Very Slow** - LED blinks at 2 seconds
- **Custom Speed** - Enter any duration (50-5000ms)
- **LED ON/OFF** - Direct control
- **Stop Blinking** - Stops current blink pattern

## 🔧 Hardware Connections

```
ESP8266 (NodeMCU)  →  Arduino R4 Minima
─────────────────────────────────────
TX (GPIO1)         →  RX (Pin 0)
RX (GPIO3)         →  TX (Pin 1)
GND                →  GND
```

## 📊 How It Works

```
PC Browser → Python Server → ESP8266 (WiFi) → Arduino (Serial)
                ↑                                    ↓
                └──────── Response logs ─────────────┘
```

1. Click button in web interface
2. Command queued on server
3. ESP8266 polls server every 3 seconds
4. ESP8266 forwards command to Arduino via serial
5. Arduino executes command (changes blink speed)
6. Arduino sends confirmation back
7. Web interface shows live logs

## 🎨 Available Commands

### Blink Speed
- **type**: "blink"
- **duration**: 50-5000 (milliseconds)

Example: `{"type":"blink","duration":250}`

### LED Control
- `{"type":"led_on"}` - Turn LED on
- `{"type":"led_off"}` - Turn LED off
- `{"type":"stop"}` - Stop blinking

### System
- `{"type":"ping"}` - Check Arduino is alive
- `{"type":"status"}` - Get current state

## 📝 Configuration

### WiFi Settings
Edit `src/esp8266_programmer.cpp`:
```cpp
const char* ssid = "muditatrey1234";
const char* password = "muditmudit";
```

### Server IP
Already set to: `10.147.66.33`

If your PC IP changes, update line 13:
```cpp
const char* serverHost = "YOUR_NEW_IP";
```

## 🔍 Monitoring

Watch ESP8266 serial output:
```bash
~/.platformio/penv/bin/pio device monitor -e esp8266_programmer
```

You'll see:
```
📨 Command from server: {"type":"blink","duration":250}
📤 Forwarding to Arduino...
📥 Arduino response: {"status":"blink_started","duration":250}
✅ Command acknowledged
```

## 🎯 Adding New Commands

### 1. Add to Arduino (`src/arduino_target.cpp`)
```cpp
else if (cmd.indexOf("\"type\":\"mycommand\"") > 0) {
    // Your code here
    Serial.println("{\"status\":\"done\"}");
}
```

### 2. Add to Web UI (`server/firmware_server.py`)

In HTML_TEMPLATE, add a button:
```html
<button onclick="sendCommand({type:'mycommand'})" class="button">
    My Command
</button>
```

### 3. Rebuild Arduino only
```bash
~/.platformio/penv/bin/pio run -e uno_r4_minima -t upload
```

No need to rebuild ESP8266 or restart server!

## ✅ System Status

| Component | Status |
|-----------|--------|
| PC Server | ✅ Ready |
| ESP8266 Bridge | ✅ Ready |
| Arduino R4 | ✅ Ready |
| Web Interface | ✅ Ready |

## 🎉 Features

✅ Real-time command execution
✅ Web-based control interface  
✅ Live logging and status
✅ Command queuing
✅ Auto-retry on network issues
✅ No firmware flashing needed
✅ Easy to add new commands
✅ Works with Arduino R4 Minima (ARM)

## 📚 Files

```
GmBH/
├── server/
│   ├── firmware_server.py       ← Python Flask server
│   └── requirements.txt          ← Dependencies
├── src/
│   ├── esp8266_programmer.cpp   ← WiFi bridge client
│   └── arduino_target.cpp        ← Command executor
└── platformio.ini                ← Build configuration
```

## 🎓 Why This Works Better Than Firmware Flashing

| Firmware Flashing | Command System |
|-------------------|----------------|
| ❌ Doesn't work (ARM bootloader) | ✅ Works perfectly |
| ❌ Requires restart | ✅ Instant execution |
| ❌ Fixed behavior | ✅ Dynamic control |
| ❌ 30+ second process | ✅ 3 second response |
| ❌ Complex protocol needed | ✅ Simple JSON over serial |

---

**You're all set!** 🚀

Open **http://localhost:5000** and start controlling your Arduino remotely!
