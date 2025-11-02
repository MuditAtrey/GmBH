# Photodiode Optical Data Transmission System

## Overview

This system provides a **backup communication method** between the web interface and Arduino R4 using **optical screen flashing** detected by a photodiode sensor. When WiFi communication fails or isn't available, commands can be transmitted by flashing the screen black/white to encode binary data.

## Hardware Setup

### Required Components
1. **Photodiode sensor** (e.g., BPW34, OPT101)
2. **10kΩ resistor** (pull-down)
3. Arduino R4 Minima

### Wiring
```
Photodiode Anode (+) → Arduino A0
Photodiode Cathode (-) → GND
10kΩ Resistor: A0 → GND (pull-down)
```

### Photodiode Placement
- **Location**: Bottom-right corner of the web browser window
- **Visual Indicator**: White box (50x50px) with 💡 icon and "Place your Photodiode here" label
- **Instructions**: 
  1. Connect photodiode to Arduino A0 as shown above
  2. Place photodiode sensor directly against the screen over the white indicator box
  3. Ensure good contact with the screen surface
  4. Dim room lighting for best results

## How It Works

### Encoding Protocol

**Binary Transmission Format:**
```
[START SEQUENCE] → [LENGTH BYTE] → [DATA BYTES] → [END]
```

**Start Sequence:**
- 5 rapid pulses (100ms white / 100ms black each)
- Used for synchronization

**Data Encoding:**
- Each bit is transmitted as a light pulse
- **Bit 0**: Short pulse (50ms white)
- **Bit 1**: Long pulse (100ms white)
- Gap between bits: 50ms black

**Example - Sending CMD_LED_SET (0x10) with payload [1]:**
```
Command ID: 0x10 (turn LED on)
Payload Length: 1
Payload: [0x01]

Transmitted bytes: [0x10, 0x01, 0x01]
Total bits: 3 bytes × 8 bits = 24 bits + start sequence
Duration: ~5 seconds
```

### Arduino Reception

The Arduino continuously monitors the photodiode on pin A0:

1. **Analog Reading**: Reads voltage on A0 (0-1023)
2. **Threshold Detection**: Values ≥ 512 = white (light), < 512 = black (dark)
3. **Start Detection**: Looks for 5 rapid pulses
4. **Bit Decoding**: Measures pulse widths to determine 0s and 1s
5. **Frame Building**: Assembles bytes into protocol frame
6. **Command Execution**: Processes command using existing handlers

**Key Functions in `arduino_target.cpp`:**
- `readPhotodiode()` - Read analog value and detect light/dark
- `detectOpticalStart()` - Wait for start sequence
- `readOpticalBit()` - Decode one bit from pulse width
- `receiveOpticalData()` - Receive complete packet
- `processOpticalData()` - Execute received command

### Web Interface Transmission

The JavaScript flashes the photodiode area div to transmit data:

**Key Functions in `arduino_designer.js`:**
- `flashPhotodiode(color, duration)` - Change background color for duration
- `sendOpticalStart()` - Send 5-pulse start sequence
- `sendOpticalBit(bit)` - Send one bit (short or long pulse)
- `sendOpticalByte(byte)` - Send 8 bits MSB first
- `transmitOpticalData(dataBytes)` - Main transmission function
- `testOpticalTransmission()` - Test with LED command

## Usage

### Test Optical Transmission

1. **Connect hardware**:
   - Wire photodiode to Arduino A0 with pull-down resistor
   - Upload `arduino_target.cpp` to Arduino R4
   - Open Serial Monitor (115200 baud) to see debug output

2. **Open web interface**:
   - Navigate to `http://localhost:5001/arduino_designer.html`
   - Locate the photodiode indicator (bottom-right corner)

3. **Position photodiode**:
   - Place photodiode sensor directly on screen over the white box
   - Ensure good contact and stable positioning

4. **Trigger test transmission**:
   - Click **"💡 Test Optical"** button in header
   - Watch the photodiode area flash rapidly
   - Check Arduino Serial Monitor for:
     ```
     Light detected: 850
     Optical start detected!
     Optical length: 3
     Byte 0: 0x10
     Byte 1: 0x01
     Byte 2: 0x01
     Processing optical command: 0x10
     CMD: 0x10
     → LED_SET
     ```
   - Arduino built-in LED should turn on!

### Send Custom Commands via Optical

From browser console:
```javascript
// Turn LED on
const CMD_LED_SET = 0x10;
await transmitOpticalData(commandToBytes(CMD_LED_SET, [1]));

// Turn LED off
await transmitOpticalData(commandToBytes(CMD_LED_SET, [0]));

// Blink LED (500ms interval)
const CMD_LED_BLINK = 0x11;
const duration = 500; // milliseconds
await transmitOpticalData(commandToBytes(CMD_LED_BLINK, [
    duration & 0xFF,        // Low byte
    (duration >> 8) & 0xFF  // High byte
]));

// Read encoder
const CMD_ENCODER_READ = 0x20;
await transmitOpticalData(commandToBytes(CMD_ENCODER_READ, []));
```

## Troubleshooting

### Arduino doesn't detect light
- **Check wiring**: Verify photodiode connected to A0 with pull-down resistor
- **Test analog read**: Add `Serial.println(analogRead(A0));` in loop to see raw values
- **Adjust threshold**: If values don't cross 512, change `OPTICAL_THRESHOLD` in code
- **Improve lighting**: Increase screen brightness, dim room lights

### Transmission errors
- **Improve contact**: Press photodiode firmly against screen
- **Reduce interference**: Close other browser tabs, disable screen dimming
- **Slow down**: Increase pulse durations in `OPTICAL_CONFIG` if photodiode is slow
- **Check serial debug**: Monitor Arduino Serial output for timing issues

### Inconsistent detection
- **Add retry logic**: Repeat start sequence if first attempt fails
- **Use error correction**: Implement CRC checksum on transmitted data
- **Increase gap duration**: Give more time between bits for slow photodiodes
- **Calibrate threshold**: Auto-detect black/white levels before transmission

## Technical Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| Photodiode Pin | A0 | Analog input |
| Threshold Voltage | ~2.5V | ADC value 512 (mid-range) |
| Start Sequence | 5 pulses | 100ms on/off each |
| Bit 0 Duration | 50ms | Short pulse |
| Bit 1 Duration | 100ms | Long pulse |
| Gap Duration | 50ms | Between bits |
| Max Packet Size | 32 bytes | Buffer limit in Arduino |
| Typical Transmission | ~5 seconds | For 3-byte command |
| Detection Frequency | 5 seconds | Arduino checks every 5s |

## Future Enhancements

1. **Error Correction**:
   - Add CRC-8 checksum to optical packets
   - Implement automatic retry on checksum failure
   - Add parity bits for single-bit error detection

2. **Bi-directional Communication**:
   - Use Arduino's built-in LED to send responses
   - Web interface captures screen region with webcam
   - Decode LED blink patterns for status/data

3. **Faster Encoding**:
   - Use Manchester encoding instead of pulse width
   - Reduce pulse durations for faster transmission
   - Implement hardware timer-based detection on Arduino

4. **Automatic Fallback**:
   - Detect WiFi failure in web interface
   - Auto-switch to optical transmission
   - Display clear user instructions for photodiode placement

5. **Multi-command Queue**:
   - Queue multiple commands for batch transmission
   - Optimize transmission by combining packets
   - Add progress indicator for long transmissions

## Command Reference

All existing binary protocol commands work via optical transmission:

| Command | ID | Payload | Description |
|---------|----|---------| ------------|
| PING | 0x01 | - | Test connection |
| LED_SET | 0x10 | [state] | Set LED on/off (0/1) |
| LED_BLINK | 0x11 | [duration_low, duration_high] | Blink LED at interval |
| ENCODER_READ | 0x20 | - | Read encoder position |
| OLED_TEXT | 0x30 | [x, y, text...] | Display text on OLED |
| OLED_CLEAR | 0x31 | - | Clear OLED display |

## Safety Notes

- **Screen burn-in**: Rapid flashing is brief (<10 seconds) and won't damage modern displays
- **Photosensitivity**: Flashing is in bottom-right corner only, minimal visible area
- **Power consumption**: Photodiode draws minimal current (~1-2mA)
- **EMI**: No radio transmission, purely optical (immune to RF interference)

## Comparison: WiFi vs Optical

| Feature | WiFi (ESP8266) | Optical (Photodiode) |
|---------|----------------|----------------------|
| **Speed** | Fast (~10ms) | Slow (~5s per command) |
| **Reliability** | Network-dependent | Always works |
| **Range** | 10-30m | Screen contact only |
| **Setup** | WiFi config needed | Just place sensor |
| **Power** | High (~70mA) | Low (~2mA) |
| **Cost** | $3-5 (ESP8266) | $1-2 (photodiode) |
| **Use Case** | Normal operation | Backup/emergency |

---

**Optical transmission provides a foolproof backup when WiFi fails!** 🌟
