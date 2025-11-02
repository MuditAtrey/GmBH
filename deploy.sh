#!/bin/bash
# Quick Deploy Script for Arduino Dynamic Configuration System

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  Arduino Dynamic Configuration - Quick Deploy           ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Change to project directory
cd /Users/muditatrey/Documents/PlatformIO/Projects/GmBH

echo "📦 Step 1: Installing dependencies..."
~/.platformio/penv/bin/platformio pkg install -e esp8266_programmer
~/.platformio/penv/bin/platformio pkg install -e uno_r4_minima

echo ""
echo "🔧 Step 2: Building ESP8266 firmware..."
~/.platformio/penv/bin/platformio run -e esp8266_programmer

if [ $? -ne 0 ]; then
    echo "❌ ESP8266 build failed!"
    exit 1
fi

echo ""
echo "🔧 Step 3: Building Arduino R4 firmware..."
~/.platformio/penv/bin/platformio run -e uno_r4_minima

if [ $? -ne 0 ]; then
    echo "❌ Arduino R4 build failed!"
    exit 1
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ✅ Both firmwares built successfully!"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "📋 NEXT STEPS:"
echo ""
echo "1. UPLOAD ESP8266 (NodeMCU):"
echo "   - Connect NodeMCU to USB"
echo "   - Run: platformio run -e esp8266_programmer -t upload"
echo "   - Wait for upload to complete"
echo "   - Monitor serial @ 115200: platformio device monitor -e esp8266_programmer"
echo ""
echo "2. UPLOAD ARDUINO R4 MINIMA:"
echo "   - Connect Arduino R4 to USB"
echo "   - Run: platformio run -e uno_r4_minima -t upload"
echo "   - Wait for upload to complete"
echo "   - Monitor serial @ 115200: platformio device monitor -e uno_r4_minima"
echo ""
echo "3. WIRE HARDWARE:"
echo "   ESP8266 → Arduino R4:"
echo "   - D1 (GPIO5)  →  RX0 (Pin 0)"
echo "   - D2 (GPIO4)  ←  TX1 (Pin 1)"
echo "   - GND         →  GND"
echo ""
echo "4. START SERVER:"
echo "   cd server"
echo "   python3 designer_server.py"
echo ""
echo "5. OPEN WEB UI:"
echo "   http://localhost:5001/arduino_designer.html"
echo ""
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Ready to upload? (Make sure only ONE device is plugged in at a time!)"
echo ""
