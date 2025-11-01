#!/usr/bin/env python3
"""
Arduino Command Center - Server with Command Queue
Stores commands to be sent to Arduino via ESP8266
"""

from flask import Flask, jsonify, request, render_template_string
import socket
from datetime import datetime

app = Flask(__name__)

# Command queue - stores commands to be sent to Arduino
command_queue = []

# Saved commands library
saved_commands = {}

@app.route('/ping')
def ping():
    """Simple ping endpoint to test connectivity"""
    return jsonify({
        'status': 'ok',
        'message': 'pong'
    })

@app.route('/')
def index():
    """Root endpoint with web interface"""
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/command/get', methods=['GET'])
def get_command():
    """ESP8266 polls this to get next command for Arduino"""
    if command_queue:
        cmd = command_queue.pop(0)
        return jsonify({
            'command': cmd['command'],
            'timestamp': cmd['timestamp']
        })
    return jsonify({'command': None})

@app.route('/api/command/send', methods=['POST'])
def send_command():
    """Web UI sends commands here to queue for Arduino"""
    data = request.json
    command = data.get('command', '')
    
    if command:
        command_queue.append({
            'command': command,
            'timestamp': datetime.now().isoformat()
        })
        return jsonify({'status': 'queued', 'command': command})
    return jsonify({'status': 'error', 'message': 'No command provided'}), 400

@app.route('/api/command/save', methods=['POST'])
def save_command():
    """Save a command to the library"""
    data = request.json
    name = data.get('name', '')
    command = data.get('command', '')
    
    if name and command:
        saved_commands[name] = {
            'command': command,
            'saved_at': datetime.now().isoformat()
        }
        return jsonify({'status': 'saved', 'name': name})
    return jsonify({'status': 'error', 'message': 'Name and command required'}), 400

@app.route('/api/command/list', methods=['GET'])
def list_saved_commands():
    """Get all saved commands"""
    return jsonify(saved_commands)

@app.route('/api/command/execute/<name>', methods=['POST'])
def execute_saved_command(name):
    """Execute a saved command by name"""
    if name in saved_commands:
        cmd = saved_commands[name]['command']
        command_queue.append({
            'command': cmd,
            'timestamp': datetime.now().isoformat()
        })
        return jsonify({'status': 'queued', 'command': cmd})
    return jsonify({'status': 'error', 'message': 'Command not found'}), 404

@app.route('/api/command/delete/<name>', methods=['DELETE'])
def delete_saved_command(name):
    """Delete a saved command"""
    if name in saved_commands:
        del saved_commands[name]
        return jsonify({'status': 'deleted', 'name': name})
    return jsonify({'status': 'error', 'message': 'Command not found'}), 404

# Simple HTML interface
HTML_TEMPLATE = '''
<!DOCTYPE html>
<html>
<head>
    <title>Arduino Command Center</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }
        h1 { color: #333; }
        .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        input, button { padding: 10px; margin: 5px; font-size: 14px; }
        button { background: #007bff; color: white; border: none; cursor: pointer; border-radius: 4px; }
        button:hover { background: #0056b3; }
        .btn-green { background: #28a745; }
        .btn-green:hover { background: #218838; }
        .btn-red { background: #dc3545; }
        .btn-red:hover { background: #c82333; }
        .btn-yellow { background: #ffc107; color: #333; }
        .btn-yellow:hover { background: #e0a800; }
        .btn-group { display: flex; gap: 10px; flex-wrap: wrap; }
        .saved-cmd { background: #f8f9fa; padding: 10px; margin: 5px 0; border-radius: 4px; }
        .delete-btn { background: #dc3545; }
        .delete-btn:hover { background: #c82333; }
        .status { padding: 10px; background: #e7f3ff; border-radius: 4px; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎛️ Arduino LED Control Center</h1>
        
        <div class="section">
            <h2>⚡ Quick Actions</h2>
            <div class="btn-group">
                <button class="btn-yellow" onclick="sendQuick('BLINK_FAST')">💨 Blink Fast (200ms)</button>
                <button class="btn-yellow" onclick="sendQuick('BLINK_SLOW')">🐌 Blink Slow (1000ms)</button>
                <button class="btn-green" onclick="sendQuick('LED_ON')">💡 LED ON</button>
                <button class="btn-red" onclick="sendQuick('LED_OFF')">🌑 LED OFF</button>
                <button onclick="sendQuick('STOP')">⏹️ Stop Blink</button>
                <button onclick="sendQuick('STATUS')">📊 Status</button>
            </div>
        </div>
        
        <div class="section">
            <h2>Send Custom Command</h2>
            <input type="text" id="cmdInput" placeholder="Enter command or JSON" style="width: 60%;">
            <button onclick="sendCommand()">Send</button>
            <div class="status">
                <small>
                    <b>Examples:</b><br>
                    • BLINK_FAST, BLINK_SLOW<br>
                    • LED_ON, LED_OFF, STOP<br>
                    • {"type":"blink","duration":500}
                </small>
            </div>
        </div>
        
        <div class="section">
            <h2>Save Command</h2>
            <input type="text" id="saveName" placeholder="Command name" style="width: 30%;">
            <input type="text" id="saveCmd" placeholder="Command" style="width: 40%;">
            <button onclick="saveCommand()">Save</button>
        </div>
        
        <div class="section">
            <h2>Saved Commands</h2>
            <div id="savedList"></div>
        </div>
    </div>
    
    <script>
        function sendQuick(cmd) {
            fetch('/api/command/send', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: cmd})
            }).then(r => r.json()).then(d => {
                console.log('✅ Queued:', d.command);
            });
        }
        
        function sendCommand() {
            const cmd = document.getElementById('cmdInput').value;
            fetch('/api/command/send', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({command: cmd})
            }).then(r => r.json()).then(d => {
                alert('Command queued: ' + d.command);
                document.getElementById('cmdInput').value = '';
            });
        }
        
        function saveCommand() {
            const name = document.getElementById('saveName').value;
            const cmd = document.getElementById('saveCmd').value;
            fetch('/api/command/save', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: name, command: cmd})
            }).then(r => r.json()).then(d => {
                alert('Command saved!');
                document.getElementById('saveName').value = '';
                document.getElementById('saveCmd').value = '';
                loadSaved();
            });
        }
        
        function executeCommand(name) {
            fetch('/api/command/execute/' + name, {method: 'POST'})
            .then(r => r.json())
            .then(d => console.log('✅ Executed:', d.command));
        }
        
        function deleteCommand(name) {
            if(confirm('Delete ' + name + '?')) {
                fetch('/api/command/delete/' + name, {method: 'DELETE'})
                .then(() => loadSaved());
            }
        }
        
        function loadSaved() {
            fetch('/api/command/list')
            .then(r => r.json())
            .then(data => {
                const list = document.getElementById('savedList');
                list.innerHTML = '';
                for(let name in data) {
                    const div = document.createElement('div');
                    div.className = 'saved-cmd';
                    div.innerHTML = `
                        <strong>${name}:</strong> ${data[name].command}
                        <button onclick="executeCommand('${name}')">Execute</button>
                        <button class="delete-btn" onclick="deleteCommand('${name}')">Delete</button>
                    `;
                    list.appendChild(div);
                }
            });
        }
        
        loadSaved();
        setInterval(loadSaved, 5000);
    </script>
</body>
</html>
'''

if __name__ == '__main__':
    # Get local IP
    try:
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
    except:
        local_ip = '127.0.0.1'
    
    print("\n" + "="*60)
    print("🎛️  Arduino Command Center")
    print("="*60)
    print(f"🌐 Local IP: {local_ip}")
    print(f"🔌 Access at: http://{local_ip}:5001")
    print(f"📡 Command endpoint: /api/command/get")
    print(f"🌍 Public URL: https://6fbx0j5c-5001.inc1.devtunnels.ms")
    print("="*60 + "\n")
    
    # Run server on all interfaces
    app.run(host='0.0.0.0', port=5001, debug=True)
