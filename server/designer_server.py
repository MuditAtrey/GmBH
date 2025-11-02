#!/usr/bin/env python3
"""
Arduino Designer Server
Serves the visual programming interface and handles configuration/deployment
"""

from flask import Flask, jsonify, request, send_from_directory
import json
from datetime import datetime
from queue import Queue
import os

app = Flask(__name__, static_folder='.')

# Storage
saved_configs = {}
command_queue = Queue()
response_queue = Queue()
device_logs = []
MAX_LOGS = 100

# Routes for Arduino Designer
@app.route('/')
def index():
    return send_from_directory('.', 'arduino_designer.html')

@app.route('/arduino_designer.html')
def designer():
    return send_from_directory('.', 'arduino_designer.html')

@app.route('/arduino_designer.js')
def designer_js():
    return send_from_directory('.', 'arduino_designer.js')

# API Endpoints
@app.route('/api/config/save', methods=['POST'])
def save_config():
    """Save Arduino configuration"""
    try:
        config = request.json
        config_id = config.get('id', 'default')
        saved_configs[config_id] = {
            'config': config,
            'timestamp': datetime.now().isoformat(),
            'deployed': False
        }
        
        # Save to file
        with open('arduino_config.json', 'w') as f:
            json.dump(config, f, indent=2)
        
        log_event('config_saved', f"Configuration saved: {len(config.get('devices', []))} devices")
        
        return jsonify({
            'success': True,
            'message': 'Configuration saved successfully',
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/config/load', methods=['GET'])
def load_config():
    """Load saved Arduino configuration"""
    try:
        if os.path.exists('arduino_config.json'):
            with open('arduino_config.json', 'r') as f:
                config = json.load(f)
            return jsonify(config)
        else:
            return jsonify({'devices': [], 'pinAssignments': {}, 'visualProgram': []})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/deploy', methods=['POST'])
def deploy_to_arduino():
    """Deploy configuration to Arduino"""
    try:
        data = request.json
        config = data.get('config', {})
        code = data.get('code', '')
        
        # Convert config to deployment package
        deployment = {
            'type': 'deploy',
            'timestamp': datetime.now().isoformat(),
            'config': config,
            'code': code
        }
        
        # Queue for ESP8266 to pick up
        command_queue.put(json.dumps(deployment))
        
        log_event('deployment', f"Deployed {len(config.get('devices', []))} devices to Arduino")
        
        return jsonify({
            'success': True,
            'message': 'Deployment queued for Arduino',
            'timestamp': datetime.now().isoformat()
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/command/get', methods=['GET'])
def get_command():
    """ESP8266 polls this to get commands"""
    if not command_queue.empty():
        command = command_queue.get()
        log_event('command_sent', f"Command sent to ESP8266: {len(command)} bytes")
        return command, 200, {'Content-Type': 'text/plain'}
    else:
        return '', 204  # No content

@app.route('/api/response', methods=['POST'])
def receive_response():
    """Receive response from Arduino via ESP8266"""
    try:
        data = request.get_data(as_text=True)
        response_queue.put(data)
        log_event('response_received', f"Response from Arduino: {data[:100]}")
        return jsonify({'success': True})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/logs', methods=['GET'])
def get_logs():
    """Get recent device logs"""
    return jsonify({
        'logs': device_logs[-50:],  # Last 50 logs
        'total': len(device_logs)
    })

@app.route('/api/status', methods=['GET'])
def get_status():
    """Get server status"""
    return jsonify({
        'status': 'online',
        'timestamp': datetime.now().isoformat(),
        'pending_commands': command_queue.qsize(),
        'pending_responses': response_queue.qsize(),
        'total_logs': len(device_logs)
    })

def log_event(event_type, message):
    """Log an event"""
    log = {
        'timestamp': datetime.now().isoformat(),
        'type': event_type,
        'message': message
    }
    device_logs.append(log)
    
    # Keep only last MAX_LOGS
    if len(device_logs) > MAX_LOGS:
        device_logs.pop(0)
    
    print(f"[{log['timestamp']}] {event_type}: {message}")

if __name__ == '__main__':
    print("=" * 60)
    print("  Arduino Designer Server")
    print("=" * 60)
    print("  Server: http://localhost:5001")
    print("  Designer: http://localhost:5001/arduino_designer.html")
    print("=" * 60)
    
    log_event('server_start', 'Arduino Designer Server started')
    
    app.run(host='0.0.0.0', port=5001, debug=True)
