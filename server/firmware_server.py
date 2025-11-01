#!/usr/bin/env python3
"""
Arduino Remote Command Server
Sends commands to Arduino via ESP8266 bridge
"""

from flask import Flask, jsonify, request, render_template_string
import json
from datetime import datetime
from queue import Queue

app = Flask(__name__)

# Command storage
saved_commands = {}  # key: command_name, value: command_dict
command_queue = Queue()
current_command = None
command_acknowledged = False

# Device logs
device_logs = []
MAX_LOGS = 100

# HTML Template for web interface
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Arduino Command Center</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1000px;
            margin: 20px auto;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            margin-bottom: 20px;
        }
        h1 {
            color: #00979D;
            margin-top: 0;
        }
        .status {
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
            background: #e3f2fd;
        }
        .button {
            background: #00979D;
            color: white;
            padding: 12px 30px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            margin: 5px;
        }
        .button:hover {
            background: #007c82;
        }
        .button.danger {
            background: #d32f2f;
        }
        .button.danger:hover {
            background: #b71c1c;
        }
        .log {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            max-height: 400px;
            overflow-y: auto;
        }
        .log-entry {
            margin: 5px 0;
            padding: 5px;
            border-left: 3px solid #00979D;
            padding-left: 10px;
        }
        .command-section {
            background: #f9f9f9;
            padding: 20px;
            margin: 15px 0;
            border-radius: 5px;
            border: 2px solid #e0e0e0;
        }
        input[type="text"], input[type="number"] {
            width: 200px;
            padding: 10px;
            margin: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        label {
            font-weight: bold;
            margin-right: 10px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
            background: white;
        }
        table th {
            background: #00979D;
            color: white;
            padding: 10px;
            text-align: left;
            font-weight: bold;
        }
        table td {
            padding: 8px;
            border-bottom: 1px solid #ddd;
        }
        table tr:hover {
            background: #f5f5f5;
        }
        code {
            background: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
            font-size: 11px;
        }
    </style>
</head>
<body>
    <h1 style="text-align: center;">🎛️ Arduino R4 Command Center</h1>
    
    <div class="container">
        <h2>💡 LED Blink Control</h2>
        
        <div class="command-section">
            <h3>Pre-set Speeds</h3>
            <button onclick="sendBlink(100)" class="button">Very Fast (100ms)</button>
            <button onclick="sendBlink(250)" class="button">Fast (250ms)</button>
            <button onclick="sendBlink(500)" class="button">Normal (500ms)</button>
            <button onclick="sendBlink(1000)" class="button">Slow (1 second)</button>
            <button onclick="sendBlink(2000)" class="button">Very Slow (2 seconds)</button>
        </div>
        
        <div class="command-section">
            <h3>Custom Speed</h3>
            <label>Blink Duration (ms):</label>
            <input type="number" id="customDuration" value="500" min="50" max="5000">
            <button onclick="sendCustomBlink()" class="button">Set Custom Speed</button>
        </div>
        
        <div class="command-section">
            <h3>LED Control</h3>
            <button onclick="sendCommand({type:'led_on'})" class="button">LED ON</button>
            <button onclick="sendCommand({type:'led_off'})" class="button">LED OFF</button>
            <button onclick="sendCommand({type:'stop'})" class="button danger">Stop Blinking</button>
        </div>
    </div>
    
    <div class="container">
        <h2>💾 Saved Commands</h2>
        <div class="command-section">
            <h3>Save Current Command</h3>
            <label>Command Name:</label>
            <input type="text" id="saveName" placeholder="e.g., fast_blink">
            <button onclick="saveCommand()" class="button">Save Last Command</button>
        </div>
        
        <div class="command-section">
            <h3>Load & Send Saved Commands</h3>
            <div id="savedCommandsList">
                <p>No saved commands yet...</p>
            </div>
        </div>
    </div>
    
    <div class="container">
        <h2>⚙️ System Control</h2>
        <button onclick="pingArduino()" class="button">Ping Arduino</button>
        <button onclick="getStatus()" class="button">Get Status</button>
        <button onclick="clearLogs()" class="button">Clear Logs</button>
    </div>
    
    <div class="container">
        <h2>📊 Device Log</h2>
        <div class="status">
            Queue: <span id="queueSize">0</span> commands | 
            Last Update: <span id="lastUpdate">-</span>
        </div>
        <div class="log" id="deviceLog">
            <div class="log-entry">Waiting for device connection...</div>
        </div>
    </div>

    <script>
        let lastCommand = null;
        
        function sendCommand(command) {
            lastCommand = command;
            fetch('/api/command/send', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(command)
            })
            .then(r => r.json())
            .then(data => {
                console.log('Command sent:', data);
                document.getElementById('queueSize').textContent = data.queue_size;
                addLog('✓ Command queued: ' + command.type);
            })
            .catch(err => {
                console.error('Error:', err);
                addLog('✗ Error: ' + err.message);
            });
        }
        
        function sendBlink(duration) {
            sendCommand({
                type: 'blink',
                duration: duration
            });
        }
        
        function sendCustomBlink() {
            const duration = parseInt(document.getElementById('customDuration').value);
            if (duration >= 50 && duration <= 5000) {
                sendBlink(duration);
            } else {
                alert('Duration must be between 50 and 5000 ms');
            }
        }
        
        function saveCommand() {
            const name = document.getElementById('saveName').value.trim();
            if (!name) {
                alert('Please enter a command name');
                return;
            }
            if (!lastCommand) {
                alert('No command to save. Send a command first!');
                return;
            }
            
            fetch('/api/command/save', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({name: name, command: lastCommand})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                document.getElementById('saveName').value = '';
                loadSavedCommands();
            })
            .catch(err => {
                addLog('✗ Error saving command: ' + err.message);
            });
        }
        
        function loadSavedCommands() {
            fetch('/api/command/list')
            .then(r => r.json())
            .then(data => {
                const listDiv = document.getElementById('savedCommandsList');
                if (Object.keys(data.commands).length === 0) {
                    listDiv.innerHTML = '<p>No saved commands yet...</p>';
                } else {
                    let html = '<table style="width:100%; border-collapse: collapse;">';
                    html += '<tr><th>Name</th><th>Command</th><th>Actions</th></tr>';
                    for (const [name, cmd] of Object.entries(data.commands)) {
                        html += `<tr>
                            <td><strong>${name}</strong></td>
                            <td><code>${JSON.stringify(cmd)}</code></td>
                            <td>
                                <button onclick="executeCommand('${name}')" class="button" style="margin:2px; font-size:12px;">Send</button>
                                <button onclick="deleteCommand('${name}')" class="button danger" style="margin:2px; font-size:12px;">Delete</button>
                            </td>
                        </tr>`;
                    }
                    html += '</table>';
                    listDiv.innerHTML = html;
                }
            })
            .catch(err => console.error('Failed to load saved commands:', err));
        }
        
        function executeCommand(name) {
            fetch('/api/command/execute', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({name: name})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                document.getElementById('queueSize').textContent = data.queue_size;
            })
            .catch(err => {
                addLog('✗ Error executing command: ' + err.message);
            });
        }
        
        function deleteCommand(name) {
            if (!confirm(`Delete command "${name}"?`)) return;
            
            fetch('/api/command/delete', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({name: name})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                loadSavedCommands();
            })
            .catch(err => {
                addLog('✗ Error deleting command: ' + err.message);
            });
        }
        
        function pingArduino() {
            sendCommand({type: 'ping'});
        }
        
        function getStatus() {
            sendCommand({type: 'status'});
        }
        
        function addLog(message) {
            const log = document.getElementById('deviceLog');
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.textContent = new Date().toLocaleTimeString() + ' - ' + message;
            log.insertBefore(entry, log.firstChild);
            
            while (log.children.length > 50) {
                log.removeChild(log.lastChild);
            }
            
            document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
        }
        
        function clearLogs() {
            document.getElementById('deviceLog').innerHTML = '<div class="log-entry">Logs cleared</div>';
        }
        
        // Auto-refresh saved commands every 5 seconds
        setInterval(loadSavedCommands, 5000);
        
        // Initial load
        loadSavedCommands();
        }
        
        // Poll for log updates
        setInterval(() => {
            fetch('/api/logs/get')
                .then(r => r.json())
                .then(data => {
                    if (data.logs && data.logs.length > 0) {
                        const log = document.getElementById('deviceLog');
                        log.innerHTML = '';
                        data.logs.forEach(entry => {
                            const div = document.createElement('div');
                            div.className = 'log-entry';
                            div.textContent = entry;
                            log.appendChild(div);
                        });
                    }
                    document.getElementById('queueSize').textContent = data.queue_size || 0;
                });
        }, 2000);
    </script>
</body>
</html>
"""


@app.route('/')
def index():
    """Web interface"""
    return render_template_string(HTML_TEMPLATE)


@app.route('/api/command/send', methods=['POST'])
def send_command():
    """Queue a command to be sent to Arduino"""
    command = request.json
    
    # Add to queue
    command_queue.put(command)
    
    log_message = f"Command queued: {command.get('type', 'unknown')}"
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_message}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"📨 {log_message}")
    
    return jsonify({
        'success': True,
        'message': 'Command queued',
        'queue_size': command_queue.qsize()
    })


@app.route('/api/command/get', methods=['GET'])
def get_command():
    """ESP8266 polls this to get the next command"""
    global current_command, command_acknowledged
    
    # If there's a current command that hasn't been acknowledged, return it again
    if current_command and not command_acknowledged:
        return jsonify({
            'has_command': True,
            'command': json.dumps(current_command)
        })
    
    # Get next command from queue
    if not command_queue.empty():
        current_command = command_queue.get()
        command_acknowledged = False
        
        print(f"📤 Sending command to ESP8266: {current_command.get('type', 'unknown')}")
        
        return jsonify({
            'has_command': True,
            'command': json.dumps(current_command)
        })
    
    return jsonify({'has_command': False})


@app.route('/api/command/ack', methods=['POST'])
def acknowledge_command():
    """ESP8266 acknowledges command was sent to Arduino"""
    global command_acknowledged
    
    command_acknowledged = True
    print("✅ Command acknowledged by ESP8266")
    
    return jsonify({'success': True})


@app.route('/api/device/report', methods=['POST'])
def device_report():
    """ESP8266 reports status and Arduino responses"""
    data = request.json
    device_id = data.get('device_id', 'Unknown')
    status = data.get('status', 'Unknown')
    message = data.get('message', '')
    
    log_message = f"[{device_id}] {status}: {message}"
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_message}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"📡 {log_message}")
    
    return jsonify({'success': True})


@app.route('/api/logs/get', methods=['GET'])
def get_logs():
    """Get recent device logs"""
    return jsonify({
        'logs': device_logs[:50],
        'queue_size': command_queue.qsize()
    })


@app.route('/api/command/save', methods=['POST'])
def save_command():
    """Save a command for later use"""
    data = request.json
    name = data.get('name', '').strip()
    command = data.get('command')
    
    if not name:
        return jsonify({'success': False, 'message': 'Command name is required'}), 400
    
    if not command:
        return jsonify({'success': False, 'message': 'Command data is required'}), 400
    
    saved_commands[name] = command
    
    log_message = f"Command saved: {name}"
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_message}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"💾 {log_message} -> {command}")
    
    return jsonify({
        'success': True,
        'message': f'Command "{name}" saved successfully'
    })


@app.route('/api/command/list', methods=['GET'])
def list_commands():
    """List all saved commands"""
    return jsonify({
        'commands': saved_commands
    })


@app.route('/api/command/execute', methods=['POST'])
def execute_command():
    """Execute a saved command by name (adds to queue)"""
    data = request.json
    name = data.get('name', '').strip()
    
    if not name:
        return jsonify({'success': False, 'message': 'Command name is required'}), 400
    
    if name not in saved_commands:
        return jsonify({'success': False, 'message': f'Command "{name}" not found'}), 404
    
    command = saved_commands[name]
    command_queue.put(command)
    
    log_message = f'Executing saved command: {name} ({command.get("type", "unknown")})'
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_message}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"▶️  {log_message}")
    
    return jsonify({
        'success': True,
        'message': f'Command "{name}" added to queue',
        'queue_size': command_queue.qsize()
    })


@app.route('/api/command/delete', methods=['POST'])
def delete_command():
    """Delete a saved command"""
    data = request.json
    name = data.get('name', '').strip()
    
    if not name:
        return jsonify({'success': False, 'message': 'Command name is required'}), 400
    
    if name not in saved_commands:
        return jsonify({'success': False, 'message': f'Command "{name}" not found'}), 404
    
    deleted_cmd = saved_commands.pop(name)
    
    log_message = f'Deleted saved command: {name}'
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_message}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"🗑️  {log_message}")
    
    return jsonify({
        'success': True,
        'message': f'Command "{name}" deleted successfully'
    })


if __name__ == '__main__':
    print("=" * 60)
    print("🎛️  Arduino R4 Command Center")
    print("=" * 60)
    print(f"🌐 Server starting on http://0.0.0.0:3000")
    print(f"💡 Web interface: http://localhost:3000")
    print(f"🔌 ESP8266 polls: http://<your-pc-ip>:3000/api/command/get")
    print("=" * 60)
    print("")
    
    app.run(host='0.0.0.0', port=3000, debug=True)
