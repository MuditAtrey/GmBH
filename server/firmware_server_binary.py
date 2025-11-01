#!/usr/bin/env python3
"""
Arduino Remote Command Server - Binary Protocol Edition
High-performance binary protocol for complex peripheral control
"""

from flask import Flask, jsonify, request, render_template_string
import json
from datetime import datetime
from queue import Queue
import binary_protocol as proto

app = Flask(__name__)

# Command storage
saved_commands = {}
command_queue = Queue()
device_logs = []
MAX_LOGS = 100

# HTML Template
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Arduino Binary Protocol Command Center</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            margin-bottom: 20px;
        }
        h1 {
            color: #667eea;
            margin-bottom: 10px;
            font-size: 2em;
            text-align: center;
        }
        h2 {
            color: #444;
            margin: 20px 0 15px 0;
            border-bottom: 2px solid #667eea;
            padding-bottom: 8px;
        }
        h3 {
            color: #666;
            margin: 15px 0 10px 0;
            font-size: 1.1em;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 20px;
            font-size: 0.9em;
        }
        .section {
            background: #f8f9fa;
            padding: 20px;
            margin: 15px 0;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        .button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 12px 24px;
            border: none;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            margin: 5px;
            transition: transform 0.2s, box-shadow 0.2s;
            font-weight: 500;
        }
        .button:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        .button:active {
            transform: translateY(0);
        }
        .button.secondary {
            background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
        }
        .button.danger {
            background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
        }
        .button.success {
            background: linear-gradient(135deg, #30cfd0 0%, #330867 100%);
        }
        .input-group {
            display: inline-flex;
            align-items: center;
            margin: 5px;
        }
        .input-group label {
            margin-right: 10px;
            font-weight: 500;
            color: #555;
        }
        input[type="text"], input[type="number"] {
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        input[type="text"]:focus, input[type="number"]:focus {
            outline: none;
            border-color: #667eea;
        }
        .log {
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 15px;
            border-radius: 8px;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            max-height: 400px;
            overflow-y: auto;
            line-height: 1.6;
        }
        .log-entry {
            margin: 3px 0;
            padding: 6px;
            border-left: 3px solid #667eea;
            padding-left: 10px;
        }
        .status-bar {
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border-radius: 8px;
            margin: 15px 0;
            display: flex;
            justify-content: space-around;
            flex-wrap: wrap;
        }
        .status-item {
            text-align: center;
            padding: 10px;
        }
        .status-label {
            font-size: 0.9em;
            opacity: 0.9;
        }
        .status-value {
            font-size: 1.5em;
            font-weight: bold;
            margin-top: 5px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
            background: white;
        }
        table th {
            background: #667eea;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: 500;
        }
        table td {
            padding: 10px;
            border-bottom: 1px solid #eee;
        }
        table tr:hover {
            background: #f8f9fa;
        }
        code {
            background: #f4f4f4;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            color: #d63384;
        }
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>⚡ Arduino Binary Protocol Command Center</h1>
        <div class="subtitle">High-performance binary communication for complex peripherals</div>
        
        <div class="status-bar">
            <div class="status-item">
                <div class="status-label">Commands Queued</div>
                <div class="status-value" id="queueSize">0</div>
            </div>
            <div class="status-item">
                <div class="status-label">Protocol Version</div>
                <div class="status-value">Binary 1.0</div>
            </div>
            <div class="status-item">
                <div class="status-label">Last Update</div>
                <div class="status-value" id="lastUpdate">-</div>
            </div>
        </div>
    </div>
    
    <div class="container">
        <h2>💡 LED Control</h2>
        
        <div class="section">
            <h3>Basic Control</h3>
            <button onclick="sendLedOn()" class="button success">LED ON</button>
            <button onclick="sendLedOff()" class="button danger">LED OFF</button>
        </div>
        
        <div class="section">
            <h3>Blink Patterns (Binary Protocol)</h3>
            <div class="grid">
                <button onclick="sendBlink(100)" class="button">Ultra Fast (100ms)</button>
                <button onclick="sendBlink(250)" class="button">Fast (250ms)</button>
                <button onclick="sendBlink(500)" class="button">Normal (500ms)</button>
                <button onclick="sendBlink(1000)" class="button">Slow (1s)</button>
                <button onclick="sendBlink(2000)" class="button">Very Slow (2s)</button>
            </div>
        </div>
        
        <div class="section">
            <h3>Custom Blink</h3>
            <div class="input-group">
                <label>Duration (ms):</label>
                <input type="number" id="customDuration" value="500" min="50" max="5000" style="width: 100px;">
                <button onclick="sendCustomBlink()" class="button secondary">Apply</button>
            </div>
        </div>
    </div>
    
    <div class="container">
        <h2>📺 OLED Display Commands</h2>
        
        <div class="section">
            <h3>Display Text</h3>
            <div class="input-group">
                <label>X:</label>
                <input type="number" id="oledX" value="0" min="0" max="127" style="width: 70px;">
                <label>Y:</label>
                <input type="number" id="oledY" value="0" min="0" max="63" style="width: 70px;">
                <label>Text:</label>
                <input type="text" id="oledText" value="Hello!" style="width: 200px;">
                <button onclick="sendOledText()" class="button secondary">Display</button>
            </div>
        </div>
        
        <div class="section">
            <h3>Quick Actions</h3>
            <button onclick="sendOledClear()" class="button danger">Clear Display</button>
            <button onclick="sendOledPreset('Arduino R4')" class="button">Show 'Arduino R4'</button>
            <button onclick="sendOledPreset('Binary Protocol')" class="button">Show 'Binary Protocol'</button>
        </div>
    </div>
    
    <div class="container">
        <h2>🎛️ Rotary Encoder</h2>
        
        <div class="section">
            <h3>Read Encoder State</h3>
            <button onclick="sendEncoderRead()" class="button secondary">Read Position & Button</button>
            <div id="encoderData" style="margin-top: 15px; padding: 15px; background: white; border-radius: 6px; border: 2px solid #ddd;">
                <strong>Last Reading:</strong> No data yet
            </div>
        </div>
    </div>
    
    <div class="container">
        <h2>🔧 System Commands</h2>
        
        <div class="section">
            <button onclick="sendPing()" class="button">Ping Arduino</button>
            <button onclick="clearLogs()" class="button danger">Clear Logs</button>
        </div>
    </div>
    
    <div class="container">
        <h2>💾 Saved Commands</h2>
        
        <div class="section">
            <h3>Save Current Command</h3>
            <div class="input-group">
                <label>Name:</label>
                <input type="text" id="saveName" placeholder="e.g., fast_blink" style="width: 200px;">
                <button onclick="saveCommand()" class="button success">Save Last Command</button>
            </div>
        </div>
        
        <div class="section">
            <h3>Saved Commands List</h3>
            <div id="savedCommandsList">
                <p style="color: #999;">No saved commands yet...</p>
            </div>
        </div>
    </div>
    
    <div class="container">
        <h2>📊 Device Log</h2>
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
                updateQueue(data.queue_size);
                addLog('✓ ' + (command.description || 'Command queued'));
            })
            .catch(err => addLog('✗ Error: ' + err.message));
        }
        
        function sendLedOn() {
            sendCommand({type: 'led_set', state: true, description: 'LED ON'});
        }
        
        function sendLedOff() {
            sendCommand({type: 'led_set', state: false, description: 'LED OFF'});
        }
        
        function sendBlink(duration) {
            sendCommand({type: 'led_blink', duration: duration, description: `Blink ${duration}ms`});
        }
        
        function sendCustomBlink() {
            const duration = parseInt(document.getElementById('customDuration').value);
            if (duration >= 50 && duration <= 5000) {
                sendBlink(duration);
            } else {
                alert('Duration must be 50-5000 ms');
            }
        }
        
        function sendOledText() {
            const x = parseInt(document.getElementById('oledX').value);
            const y = parseInt(document.getElementById('oledY').value);
            const text = document.getElementById('oledText').value;
            sendCommand({type: 'oled_text', x: x, y: y, text: text, description: `OLED: "${text}"`});
        }
        
        function sendOledClear() {
            sendCommand({type: 'oled_clear', description: 'Clear OLED'});
        }
        
        function sendOledPreset(text) {
            document.getElementById('oledX').value = 0;
            document.getElementById('oledY').value = 0;
            document.getElementById('oledText').value = text;
            sendOledText();
        }
        
        function sendEncoderRead() {
            sendCommand({type: 'encoder_read', description: 'Read encoder'});
        }
        
        function sendPing() {
            sendCommand({type: 'ping', description: 'Ping Arduino'});
        }
        
        function saveCommand() {
            const name = document.getElementById('saveName').value.trim();
            if (!name) {
                alert('Enter a command name');
                return;
            }
            if (!lastCommand) {
                alert('No command to save. Send a command first!');
                return;
            }
            
            fetch('/api/command/save', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: name, command: lastCommand})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                document.getElementById('saveName').value = '';
                loadSavedCommands();
            });
        }
        
        function loadSavedCommands() {
            fetch('/api/command/list')
            .then(r => r.json())
            .then(data => {
                const list = document.getElementById('savedCommandsList');
                if (Object.keys(data.commands).length === 0) {
                    list.innerHTML = '<p style="color: #999;">No saved commands yet...</p>';
                } else {
                    let html = '<table><tr><th>Name</th><th>Command</th><th>Actions</th></tr>';
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
                    list.innerHTML = html;
                }
            });
        }
        
        function executeCommand(name) {
            fetch('/api/command/execute', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: name})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                updateQueue(data.queue_size);
            });
        }
        
        function deleteCommand(name) {
            if (!confirm(`Delete "${name}"?`)) return;
            fetch('/api/command/delete', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: name})
            })
            .then(r => r.json())
            .then(data => {
                addLog('✓ ' + data.message);
                loadSavedCommands();
            });
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
        
        function updateQueue(size) {
            document.getElementById('queueSize').textContent = size || 0;
        }
        
        // Auto-refresh
        setInterval(loadSavedCommands, 5000);
        setInterval(() => {
            fetch('/api/logs/get')
                .then(r => r.json())
                .then(data => {
                    updateQueue(data.queue_size);
                    if (data.encoder_data) {
                        const enc = data.encoder_data;
                        document.getElementById('encoderData').innerHTML = 
                            `<strong>Position:</strong> ${enc.position} | ` +
                            `<strong>Velocity:</strong> ${enc.velocity} | ` +
                            `<strong>Button:</strong> ${enc.button_pressed ? 'PRESSED' : 'Released'}`;
                    }
                });
        }, 2000);
        
        // Initial load
        loadSavedCommands();
    </script>
</body>
</html>
"""


# Global state for encoder data
last_encoder_data = None


@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)


@app.route('/api/command/send', methods=['POST'])
def send_command():
    """Queue a command (JSON from web UI, will be converted to binary)"""
    command = request.json
    command_queue.put(command)
    
    log_msg = f"Queued: {command.get('type', 'unknown')}"
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_msg}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"📨 {log_msg}: {command}")
    
    return jsonify({
        'success': True,
        'message': 'Command queued',
        'queue_size': command_queue.qsize()
    })


@app.route('/api/command/get', methods=['GET'])
def get_command():
    """ESP8266 polls this to get next command (returns JSON for compatibility)"""
    if not command_queue.empty():
        cmd = command_queue.get()
        print(f"📤 Sending to ESP8266: {cmd}")
        return jsonify(cmd)
    
    return jsonify({'has_command': False}), 204


@app.route('/api/response/encoder', methods=['POST'])
def receive_encoder_data():
    """Receive encoder data from ESP8266"""
    global last_encoder_data
    data = request.json
    last_encoder_data = data
    
    log_msg = f"Encoder: pos={data.get('position')}, vel={data.get('velocity')}, btn={data.get('button_pressed')}"
    device_logs.insert(0, f"{datetime.now().strftime('%H:%M:%S')} - {log_msg}")
    if len(device_logs) > MAX_LOGS:
        device_logs.pop()
    
    print(f"📡 {log_msg}")
    
    return jsonify({'success': True})


@app.route('/api/logs/get', methods=['GET'])
def get_logs():
    """Get recent logs and status"""
    return jsonify({
        'logs': device_logs[:50],
        'queue_size': command_queue.qsize(),
        'encoder_data': last_encoder_data
    })


@app.route('/api/command/save', methods=['POST'])
def save_command():
    """Save a command"""
    data = request.json
    name = data.get('name', '').strip()
    command = data.get('command')
    
    if not name or not command:
        return jsonify({'success': False, 'message': 'Invalid request'}), 400
    
    saved_commands[name] = command
    print(f"💾 Saved command: {name}")
    
    return jsonify({'success': True, 'message': f'Command "{name}" saved'})


@app.route('/api/command/list', methods=['GET'])
def list_commands():
    """List saved commands"""
    return jsonify({'commands': saved_commands})


@app.route('/api/command/execute', methods=['POST'])
def execute_command():
    """Execute a saved command"""
    data = request.json
    name = data.get('name', '').strip()
    
    if name not in saved_commands:
        return jsonify({'success': False, 'message': f'Command "{name}" not found'}), 404
    
    command = saved_commands[name]
    command_queue.put(command)
    
    print(f"▶️  Executing saved command: {name}")
    
    return jsonify({
        'success': True,
        'message': f'Command "{name}" queued',
        'queue_size': command_queue.qsize()
    })


@app.route('/api/command/delete', methods=['POST'])
def delete_command():
    """Delete a saved command"""
    data = request.json
    name = data.get('name', '').strip()
    
    if name not in saved_commands:
        return jsonify({'success': False, 'message': f'Command "{name}" not found'}), 404
    
    saved_commands.pop(name)
    print(f"🗑️  Deleted command: {name}")
    
    return jsonify({'success': True, 'message': f'Command "{name}" deleted'})


if __name__ == '__main__':
    print("=" * 70)
    print("⚡ Arduino Binary Protocol Command Center")
    print("=" * 70)
    print(f"🌐 Server: http://0.0.0.0:5001")
    print(f"💡 Web UI: http://localhost:5001")
    print(f"🔌 ESP8266 endpoint: http://<your-ip>:5001/api/command/get")
    print("=" * 70)
    print("")
    
    # Test protocol encoding
    print("Testing binary protocol...")
    frame = proto.build_led_blink(500)
    print(f"✅ LED Blink frame: {frame.hex()}")
    print("")
    
    app.run(host='0.0.0.0', port=5001, debug=True)
