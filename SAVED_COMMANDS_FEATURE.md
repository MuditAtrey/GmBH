# 💾 Saved Commands Feature

## Overview
The server now supports **server-side command storage**, giving you complete freedom over how and when commands are sent to the Arduino. Instead of directly queuing commands, you can now:
- Save frequently-used commands with custom names
- Build a library of commands
- Execute saved commands at any time with one click
- Manage (view, send, delete) all saved commands from the web UI

## What Changed

### 1. **Server-Side Storage**
- Added `saved_commands` dictionary to `firmware_server.py`
- Commands are stored on the PC server (not on the NodeMCU)
- Persistent during server runtime (in-memory storage)
- Full control: save, list, execute, delete

### 2. **New API Endpoints**

#### `POST /api/command/save`
Save a command with a custom name.
```json
Request:
{
  "name": "fast_blink",
  "command": {"type": "blink", "duration": 100}
}

Response:
{
  "success": true,
  "message": "Command 'fast_blink' saved successfully"
}
```

#### `GET /api/command/list`
Get all saved commands.
```json
Response:
{
  "commands": {
    "fast_blink": {"type": "blink", "duration": 100},
    "slow_blink": {"type": "blink", "duration": 2000},
    "led_on": {"type": "led_on"}
  }
}
```

#### `POST /api/command/execute`
Execute a saved command by name (adds to queue).
```json
Request:
{
  "name": "fast_blink"
}

Response:
{
  "success": true,
  "message": "Command 'fast_blink' added to queue",
  "queue_size": 1
}
```

#### `POST /api/command/delete`
Delete a saved command.
```json
Request:
{
  "name": "fast_blink"
}

Response:
{
  "success": true,
  "message": "Command 'fast_blink' deleted successfully"
}
```

### 3. **Enhanced Web UI**

The web interface now has a new section: **💾 Saved Commands**

**Features:**
- **Save Current Command**: After sending any command (e.g., blink, LED on/off), you can save it with a custom name
- **Saved Commands List**: Shows all saved commands in a table with:
  - Command name
  - Full command JSON
  - Action buttons: Send | Delete
- **Auto-refresh**: The saved commands list refreshes every 5 seconds
- **Instant Execution**: Click "Send" on any saved command to add it to the queue immediately

## How to Use

### Step 1: Start the Server
```bash
cd server
python3 firmware_server.py
```

### Step 2: Open Web Interface
Open your browser to: `http://localhost:5000`

### Step 3: Save Commands

**Method A - Save after sending:**
1. Use any button to send a command (e.g., "Very Fast (100ms)")
2. Enter a name in "Command Name" field (e.g., `very_fast`)
3. Click "Save Last Command"
4. ✅ Command is now saved!

**Method B - Save via API:**
```bash
curl -X POST http://localhost:5000/api/command/save \
  -H "Content-Type: application/json" \
  -d '{"name":"my_pattern","command":{"type":"blink","duration":750}}'
```

### Step 4: Execute Saved Commands
- Click the **"Send"** button next to any saved command in the list
- The command will be added to the queue and sent to Arduino
- You'll see the confirmation in the logs

### Step 5: Delete Saved Commands
- Click the **"Delete"** button next to any saved command
- Confirm the deletion
- Command is removed from the library

## Example Workflow

```
1. Test different blink speeds using preset buttons
2. Find the perfect timing (e.g., 350ms)
3. Use Custom Speed to set 350ms, click "Set Custom Speed"
4. Name it "perfect_blink" and save
5. Later: click "Send" on "perfect_blink" to run it again
6. Build a library: "startup_sequence", "alert_pattern", "idle_blink", etc.
```

## Architecture

```
┌─────────────────────────────────────────────┐
│           PC (Flask Server)                 │
│  ┌──────────────┐    ┌──────────────┐      │
│  │ Command      │    │ Saved        │      │
│  │ Queue        │◄───│ Commands     │      │
│  │ (immediate)  │    │ (library)    │      │
│  └──────┬───────┘    └──────▲───────┘      │
│         │                   │              │
│         │                   │              │
└─────────┼───────────────────┼──────────────┘
          │                   │
          │ (poll)            │ (web UI)
          ▼                   │
    ┌──────────┐              │
    │ ESP8266  │              │
    │ (bridge) │◄─────────────┘
    └────┬─────┘
         │ (serial)
         ▼
    ┌──────────┐
    │ Arduino  │
    │   R4     │
    └──────────┘
```

**Key Points:**
- **Saved Commands** = Your command library (stored on PC)
- **Command Queue** = Commands waiting to be sent to Arduino
- Execute a saved command → Copies it to the queue → ESP8266 polls and gets it → Arduino executes

## Use Cases

1. **Testing & Development**
   - Save different configurations
   - Quickly test variations
   - Build a test suite

2. **Complex Sequences**
   - Save individual steps
   - Execute in sequence manually or via scripts
   - Mix and match saved commands

3. **Remote Control**
   - Build a command library
   - Use from any device on the network
   - API-driven automation

4. **Production Patterns**
   - Save validated commands
   - Ensure consistency
   - Quick deployment

## Technical Details

- **Storage**: In-memory Python dictionary (runtime only)
- **Persistence**: Commands lost when server restarts (add file save if needed)
- **Capacity**: Limited only by RAM
- **Thread Safety**: Flask handles requests sequentially by default
- **Auto-refresh**: UI polls saved commands list every 5 seconds

## Future Enhancements (Ideas)

- [ ] Persist saved commands to JSON file
- [ ] Import/export command libraries
- [ ] Command categories/tags
- [ ] Macro support (execute multiple commands in sequence)
- [ ] Scheduled command execution
- [ ] Command versioning

## API Usage Examples

### Python
```python
import requests

# Save a command
requests.post('http://localhost:5000/api/command/save',
    json={'name': 'alert', 'command': {'type': 'blink', 'duration': 100}})

# List all commands
resp = requests.get('http://localhost:5000/api/command/list')
print(resp.json())

# Execute a saved command
requests.post('http://localhost:5000/api/command/execute',
    json={'name': 'alert'})

# Delete a command
requests.post('http://localhost:5000/api/command/delete',
    json={'name': 'alert'})
```

### JavaScript (from web console)
```javascript
// Save command
fetch('/api/command/save', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({
    name: 'test_blink',
    command: {type: 'blink', duration: 500}
  })
});

// Execute
fetch('/api/command/execute', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({name: 'test_blink'})
});
```

### cURL
```bash
# Save
curl -X POST http://localhost:5000/api/command/save \
  -H "Content-Type: application/json" \
  -d '{"name":"my_cmd","command":{"type":"led_on"}}'

# List
curl http://localhost:5000/api/command/list

# Execute
curl -X POST http://localhost:5000/api/command/execute \
  -H "Content-Type: application/json" \
  -d '{"name":"my_cmd"}'

# Delete
curl -X POST http://localhost:5000/api/command/delete \
  -H "Content-Type: application/json" \
  -d '{"name":"my_cmd"}'
```

## Summary

✅ **Complete server-side control** - Commands stored on PC, not ESP8266  
✅ **Full CRUD operations** - Save, List, Execute, Delete  
✅ **User-friendly web UI** - Visual command management  
✅ **REST API** - Programmable access  
✅ **Auto-refresh** - Real-time updates  
✅ **Logging** - All operations logged  

You now have **complete freedom** over how commands are stored and transmitted to your Arduino! 🎉
