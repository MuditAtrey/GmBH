#!/usr/bin/env python3
"""
Helper script to find your PC's IP address and test the server
"""

import socket
import subprocess
import sys

def get_ip_address():
    """Get the local IP address of this machine"""
    try:
        # Create a socket to determine local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return None

def check_port_available(port=5000):
    """Check if port is available"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    result = sock.connect_ex(('127.0.0.1', port))
    sock.close()
    return result != 0

def main():
    print("=" * 60)
    print("Arduino Remote Programmer - Network Configuration Helper")
    print("=" * 60)
    print()
    
    # Get IP address
    ip = get_ip_address()
    if ip:
        print(f"✅ Your PC's IP Address: {ip}")
        print()
        print("Update src/esp8266_programmer.cpp with this IP:")
        print(f'   const char* serverHost = "{ip}";')
        print()
    else:
        print("❌ Could not determine IP address")
        print("   Please find it manually:")
        print("   - macOS: ifconfig | grep 'inet '")
        print("   - Linux: ip addr show")
        print("   - Windows: ipconfig")
        print()
    
    # Check port availability
    if check_port_available(5000):
        print("✅ Port 5000 is available for the server")
    else:
        print("⚠️  Port 5000 is in use")
        print("   Stop any running servers or change the port")
        print()
    
    # Show server URLs
    if ip:
        print()
        print("Server URLs (once started):")
        print(f"   Local:   http://localhost:5000")
        print(f"   Network: http://{ip}:5000")
        print()
        print("ESP8266 API endpoint:")
        print(f"   http://{ip}:5000/api/firmware/check")
        print()
    
    # Test network connectivity
    print()
    print("To test if ESP8266 can reach this PC:")
    print("1. Make sure both are on the same WiFi network")
    print("2. Start the server: python3 server/firmware_server.py")
    if ip:
        print(f"3. From ESP8266 serial monitor, verify it can reach: {ip}:5000")
    print()
    
    # Firewall check
    print("Firewall Configuration:")
    print("   Make sure port 5000 is allowed in your firewall")
    print()
    if sys.platform == 'darwin':
        print("   macOS: System Preferences → Security & Privacy → Firewall")
    elif sys.platform.startswith('linux'):
        print("   Linux: sudo ufw allow 5000")
    elif sys.platform == 'win32':
        print("   Windows: Windows Defender Firewall → Advanced Settings")
    print()

if __name__ == "__main__":
    main()
