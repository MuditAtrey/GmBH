#!/bin/bash

# Quick Start Script for Arduino Remote Programmer
# This script helps you get started quickly

set -e  # Exit on error

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     Arduino R4 Minima Remote Programming - Quick Start     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if Python is installed
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}✗ Python 3 is not installed${NC}"
    echo "  Please install Python 3.7 or higher"
    exit 1
fi
echo -e "${GREEN}✓${NC} Python 3 found: $(python3 --version)"

# Check if PlatformIO is installed
if ! command -v ~/.platformio/penv/bin/pio &> /dev/null; then
    echo -e "${RED}✗ PlatformIO is not installed${NC}"
    echo "  Please install PlatformIO Core"
    exit 1
fi
echo -e "${GREEN}✓${NC} PlatformIO found"

# Get project root directory
PROJECT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$PROJECT_ROOT"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Step 1: Install Python Dependencies"
echo "════════════════════════════════════════════════════════════"

if [ -d "server" ]; then
    cd server
    if [ -f "requirements.txt" ]; then
        echo "Installing Flask and dependencies..."
        pip3 install -r requirements.txt --quiet
        echo -e "${GREEN}✓${NC} Python dependencies installed"
    fi
    cd "$PROJECT_ROOT"
else
    echo -e "${YELLOW}⚠${NC}  Server directory not found"
fi

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Step 2: Configuration Check"
echo "════════════════════════════════════════════════════════════"

# Get PC's IP address
if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    PC_IP=$(ifconfig | grep "inet " | grep -v 127.0.0.1 | awk '{print $2}' | head -n 1)
else
    # Linux
    PC_IP=$(hostname -I | awk '{print $1}')
fi

echo "Your PC's IP address: ${GREEN}$PC_IP${NC}"
echo ""
echo -e "${YELLOW}⚠${NC}  Please update src/esp8266_programmer.cpp:"
echo "   Line 13: const char* serverHost = \"$PC_IP\";"
echo "   Lines 9-10: WiFi credentials"
echo ""
read -p "Press Enter after updating the code..."

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Step 3: Build Firmware"
echo "════════════════════════════════════════════════════════════"

echo "Building ESP8266 programmer..."
~/.platformio/penv/bin/pio run -e esp8266_programmer
echo -e "${GREEN}✓${NC} ESP8266 firmware built"

echo ""
echo "Building Arduino R4 test firmware..."
~/.platformio/penv/bin/pio run -e uno_r4_minima
echo -e "${GREEN}✓${NC} Arduino R4 firmware built"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Step 4: Upload ESP8266 Firmware"
echo "════════════════════════════════════════════════════════════"
echo -e "${YELLOW}⚠${NC}  Connect your ESP8266 (NodeMCU) via USB"
read -p "Press Enter when ready to upload..."

~/.platformio/penv/bin/pio run -e esp8266_programmer -t upload
echo -e "${GREEN}✓${NC} ESP8266 firmware uploaded"

echo ""
echo "════════════════════════════════════════════════════════════"
echo "Next Steps"
echo "════════════════════════════════════════════════════════════"
echo ""
echo "1. Start the PC server:"
echo "   ${GREEN}cd server && python3 firmware_server.py${NC}"
echo ""
echo "2. Open web interface:"
echo "   ${GREEN}http://localhost:5000${NC}"
echo ""
echo "3. Upload firmware via web interface:"
echo "   File: ${GREEN}.pio/build/uno_r4_minima/firmware.bin${NC}"
echo ""
echo "4. Monitor ESP8266:"
echo "   ${GREEN}~/.platformio/penv/bin/pio device monitor -e esp8266_programmer${NC}"
echo ""
echo "5. Check out the full documentation:"
echo "   ${GREEN}README.md${NC}"
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    Setup Complete! 🎉                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
