# Quick Reference: Saved Commands System

## 🚀 Quick Start

1. **Start Server:**
   ```bash
   cd server
   python3 firmware_server.py
   ```

2. **Open Browser:** http://localhost:5000

3. **Workflow:**
   - Send a command (e.g., click "Fast (250ms)")
   - Enter name in "Command Name" field
   - Click "Save Last Command"
   - See it appear in "Saved Commands" list
   - Click "Send" to execute it again anytime

## 📋 API Endpoints

### Existing (unchanged):
- `POST /api/command/send` - Queue a command immediately
- `GET /api/command/get` - ESP8266 polls for next command
- `POST /api/command/ack` - ESP8266 acknowledges command
- `POST /api/device/report` - Device status reports
- `GET /api/logs/get` - Get recent logs

### New (saved commands):
- `POST /api/command/save` - Save a command with name
- `GET /api/command/list` - List all saved commands
- `POST /api/command/execute` - Execute saved command (add to queue)
- `POST /api/command/delete` - Delete a saved command

## 🎯 Command Examples

### Blink Commands
```json
{"type": "blink", "duration": 100}   // Very fast
{"type": "blink", "duration": 500}   // Normal
{"type": "blink", "duration": 2000}  // Slow
```

### LED Control
```json
{"type": "led_on"}   // Turn LED on
{"type": "led_off"}  // Turn LED off
{"type": "stop"}     // Stop blinking
```

### System Commands
```json
{"type": "ping"}     // Test communication
{"type": "status"}   // Get Arduino status
```

## 💡 Typical Use Cases

### 1. Testing Different Speeds
```bash
# From web UI:
1. Click different preset speeds
2. Find the one you like
3. Save it with a memorable name
4. Execute it later with one click
```

### 2. Building a Command Library
```python
import requests
SERVER = 'http://localhost:5000'

# Save multiple commands
commands = {
    'startup': {'type': 'blink', 'duration': 200},
    'idle': {'type': 'blink', 'duration': 1000},
    'alert': {'type': 'blink', 'duration': 50},
    'off': {'type': 'led_off'}
}

for name, cmd in commands.items():
    requests.post(f'{SERVER}/api/command/save',
        json={'name': name, 'command': cmd})
```

### 3. Automated Sequence
```python
import requests
import time

SERVER = 'http://localhost:5000'

# Execute saved commands in sequence
sequence = ['startup', 'idle', 'alert', 'off']

for cmd_name in sequence:
    requests.post(f'{SERVER}/api/command/execute',
        json={'name': cmd_name})
    time.sleep(5)  # Wait 5 seconds between commands
```

### 4. Remote Control via API
```bash
# Save from command line
curl -X POST http://192.168.1.100:5000/api/command/save \
  -H "Content-Type: application/json" \
  -d '{"name":"remote_blink","command":{"type":"blink","duration":300}}'

# Execute from anywhere on network
curl -X POST http://192.168.1.100:5000/api/command/execute \
  -H "Content-Type: application/json" \
  -d '{"name":"remote_blink"}'
```

## 🔧 Differences: Direct Send vs Saved Commands

### Direct Send (`/api/command/send`)
- Immediately adds to queue
- ESP8266 gets it on next poll (~3 seconds)
- Use for: One-time commands, testing, manual control

### Saved Commands (`/api/command/save` → `/api/command/execute`)
- Stored on server (reusable)
- Execute when needed (not automatic)
- Use for: Frequently-used commands, automation, sequences

## ⚙️ Configuration

### Change Poll Interval (ESP8266)
Edit `src/esp8266_programmer.cpp`:
```cpp
const unsigned long POLL_INTERVAL = 3000;  // Change to desired ms
```

### Add Persistence (save to file)
Add to `server/firmware_server.py`:
```python
import json

def save_to_file():
    with open('saved_commands.json', 'w') as f:
        json.dump(saved_commands, f)

def load_from_file():
    try:
        with open('saved_commands.json', 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        return {}

# At startup:
saved_commands = load_from_file()

# After save/delete:
save_to_file()
```

## 📊 Monitoring

### View All Saved Commands
```bash
curl http://localhost:5000/api/command/list | python3 -m json.tool
```

### Check Queue Size
```bash
curl http://localhost:5000/api/logs/get | python3 -m json.tool
```

### Server Logs
The terminal running `firmware_server.py` shows:
- 💾 Command saved: [name] -> [command]
- ▶️ Executing saved command: [name] ([type])
- 🗑️ Deleted saved command: [name]

## 🎨 Web UI Features

### Saved Commands Section
- **Real-time list** - Auto-refreshes every 5 seconds
- **Styled table** - Easy to read, hover effects
- **Action buttons** - Send and Delete for each command
- **Save interface** - Name input + save button
- **Integrated logs** - All operations logged below

### Color Coding
- 🔵 Blue buttons - Normal actions (Send, Set)
- 🔴 Red buttons - Destructive actions (Delete, Stop)
- 🟢 Green accents - Success messages
- 🟡 Hover effects - Interactive feedback

## 🐛 Troubleshooting

### Command not executing?
1. Check server logs for errors
2. Verify command name exists: `GET /api/command/list`
3. Check queue size (might be backlogged)
4. Ensure ESP8266 is connected and polling

### Can't save command?
1. Send a command first (click any button)
2. Enter a valid name (no spaces recommended)
3. Check browser console for errors
4. Try via API directly (see curl examples above)

### Commands disappeared?
- In-memory storage: restarting server clears saved commands
- Add file persistence (see Configuration section)

## 📝 Notes

- **Storage**: In-memory only (runtime persistence)
- **Thread-safe**: Flask default mode (single-threaded)
- **Capacity**: Unlimited (RAM is limit)
- **Naming**: Use alphanumeric + underscore (e.g., `fast_blink_1`)
- **Overwrite**: Saving with same name overwrites previous command

## 🎓 Next Steps

1. ✅ **Basic Usage** - Save and execute a few commands
2. ✅ **API Testing** - Try curl commands
3. ✅ **Automation** - Write a Python script to control Arduino
4. 📦 **Persistence** - Add file saving (optional)
5. 🔄 **Sequences** - Build command macros (future feature)

---

**Have fun with your Arduino R4 Command Center!** 🎉
