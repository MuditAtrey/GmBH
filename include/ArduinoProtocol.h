/*
 * Arduino Binary Protocol
 * 
 * High-performance binary protocol for ESP8266 <-> Arduino communication
 * Supports complex data types, bidirectional messaging, and peripheral abstractions
 * 
 * Frame Format:
 * [START_BYTE][CMD_ID][LENGTH_HIGH][LENGTH_LOW][PAYLOAD...][CRC_HIGH][CRC_LOW]
 * 
 * - START_BYTE: 0xAA (sync marker)
 * - CMD_ID: 1 byte command identifier
 * - LENGTH: 2 bytes payload length (big-endian, max 1024 bytes)
 * - PAYLOAD: Variable length data
 * - CRC: 16-bit CRC checksum of CMD_ID + LENGTH + PAYLOAD
 */

#ifndef ARDUINO_PROTOCOL_H
#define ARDUINO_PROTOCOL_H

#include <Arduino.h>

// Protocol constants
#define PROTO_START_BYTE 0xAA
#define PROTO_MAX_PAYLOAD 1024
#define PROTO_HEADER_SIZE 4  // START + CMD + LEN_H + LEN_L
#define PROTO_FOOTER_SIZE 2  // CRC_H + CRC_L
#define PROTO_TIMEOUT_MS 100

// Command IDs
enum CommandID : uint8_t {
    // Basic commands
    CMD_PING = 0x01,
    CMD_PONG = 0x02,
    CMD_ERROR = 0x03,
    CMD_ACK = 0x04,
    
    // LED commands
    CMD_LED_SET = 0x10,
    CMD_LED_BLINK = 0x11,
    CMD_LED_PATTERN = 0x12,
    
    // Sensor commands
    CMD_SENSOR_READ = 0x20,
    CMD_SENSOR_DATA = 0x21,
    CMD_SENSOR_CONFIG = 0x22,
    
    // Rotary encoder
    CMD_ENCODER_READ = 0x30,
    CMD_ENCODER_DATA = 0x31,
    CMD_ENCODER_RESET = 0x32,
    
    // OLED display
    CMD_OLED_CLEAR = 0x40,
    CMD_OLED_TEXT = 0x41,
    CMD_OLED_PIXEL = 0x42,
    CMD_OLED_LINE = 0x43,
    CMD_OLED_RECT = 0x44,
    CMD_OLED_BITMAP = 0x45,
    
    // Generic data
    CMD_DATA_UINT8 = 0x50,
    CMD_DATA_INT16 = 0x51,
    CMD_DATA_INT32 = 0x52,
    CMD_DATA_FLOAT = 0x53,
    CMD_DATA_STRING = 0x54,
    CMD_DATA_ARRAY = 0x55,
};

// Error codes (prefix with PROTO_ to avoid ESP8266 lwip conflicts)
enum ErrorCode : uint8_t {
    PROTO_ERR_OK = 0x00,
    PROTO_ERR_INVALID_CMD = 0x01,
    PROTO_ERR_INVALID_CRC = 0x02,
    PROTO_ERR_TIMEOUT = 0x03,
    PROTO_ERR_BUFFER_OVERFLOW = 0x04,
    PROTO_ERR_INVALID_PARAM = 0x05,
    PROTO_ERR_NOT_READY = 0x06,
};

// Protocol frame structure
struct ProtocolFrame {
    uint8_t startByte;
    uint8_t commandId;
    uint16_t length;
    uint8_t payload[PROTO_MAX_PAYLOAD];
    uint16_t crc;
    
    ProtocolFrame() : startByte(PROTO_START_BYTE), commandId(0), length(0), crc(0) {}
};

// Payload builder helper
class PayloadBuilder {
private:
    uint8_t* buffer;
    uint16_t capacity;
    uint16_t position;
    
public:
    PayloadBuilder(uint8_t* buf, uint16_t cap) : buffer(buf), capacity(cap), position(0) {}
    
    void reset() { position = 0; }
    uint16_t size() const { return position; }
    
    bool addUint8(uint8_t value) {
        if (position + 1 > capacity) return false;
        buffer[position++] = value;
        return true;
    }
    
    bool addInt16(int16_t value) {
        if (position + 2 > capacity) return false;
        buffer[position++] = (value >> 8) & 0xFF;
        buffer[position++] = value & 0xFF;
        return true;
    }
    
    bool addUint16(uint16_t value) {
        if (position + 2 > capacity) return false;
        buffer[position++] = (value >> 8) & 0xFF;
        buffer[position++] = value & 0xFF;
        return true;
    }
    
    bool addInt32(int32_t value) {
        if (position + 4 > capacity) return false;
        buffer[position++] = (value >> 24) & 0xFF;
        buffer[position++] = (value >> 16) & 0xFF;
        buffer[position++] = (value >> 8) & 0xFF;
        buffer[position++] = value & 0xFF;
        return true;
    }
    
    bool addFloat(float value) {
        union { float f; uint32_t u; } converter;
        converter.f = value;
        return addInt32(converter.u);
    }
    
    bool addString(const char* str) {
        uint16_t len = strlen(str);
        if (position + len + 1 > capacity) return false;
        if (!addUint8(len)) return false;
        memcpy(buffer + position, str, len);
        position += len;
        return true;
    }
    
    bool addBytes(const uint8_t* data, uint16_t len) {
        if (position + len > capacity) return false;
        memcpy(buffer + position, data, len);
        position += len;
        return true;
    }
};

// Payload parser helper
class PayloadParser {
public:
    const uint8_t* buffer;
    uint16_t length;
    uint16_t position;
    
    PayloadParser(const uint8_t* buf, uint16_t len) : buffer(buf), length(len), position(0) {}
    
    void reset() { position = 0; }
    uint16_t remaining() const { return length - position; }
    bool hasData() const { return position < length; }
    
    bool readUint8(uint8_t& value) {
        if (position + 1 > length) return false;
        value = buffer[position++];
        return true;
    }
    
    bool readInt16(int16_t& value) {
        if (position + 2 > length) return false;
        value = ((int16_t)buffer[position] << 8) | buffer[position + 1];
        position += 2;
        return true;
    }
    
    bool readUint16(uint16_t& value) {
        if (position + 2 > length) return false;
        value = ((uint16_t)buffer[position] << 8) | buffer[position + 1];
        position += 2;
        return true;
    }
    
    bool readInt32(int32_t& value) {
        if (position + 4 > length) return false;
        value = ((int32_t)buffer[position] << 24) | 
                ((int32_t)buffer[position + 1] << 16) |
                ((int32_t)buffer[position + 2] << 8) |
                buffer[position + 3];
        position += 4;
        return true;
    }
    
    bool readFloat(float& value) {
        int32_t intValue;
        if (!readInt32(intValue)) return false;
        union { float f; int32_t i; } converter;
        converter.i = intValue;
        value = converter.f;
        return true;
    }
    
    bool readString(char* str, uint16_t maxLen) {
        uint8_t len;
        if (!readUint8(len)) return false;
        if (len >= maxLen || position + len > length) return false;
        memcpy(str, buffer + position, len);
        str[len] = '\0';
        position += len;
        return true;
    }
    
    bool readBytes(uint8_t* data, uint16_t len) {
        if (position + len > length) return false;
        memcpy(data, buffer + position, len);
        position += len;
        return true;
    }
};

// CRC-16 calculation (CCITT polynomial)
class CRC16 {
public:
    static uint16_t calculate(const uint8_t* data, uint16_t length) {
        uint16_t crc = 0xFFFF;
        for (uint16_t i = 0; i < length; i++) {
            crc ^= (uint16_t)data[i] << 8;
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc = crc << 1;
                }
            }
        }
        return crc;
    }
};

// Protocol handler
class ProtocolHandler {
private:
    Stream* serial;
    ProtocolFrame rxFrame;
    uint8_t rxState;
    uint16_t rxByteCount;
    unsigned long rxStartTime;
    
    enum RxState {
        RX_WAIT_START,
        RX_WAIT_CMD,
        RX_WAIT_LEN_HIGH,
        RX_WAIT_LEN_LOW,
        RX_WAIT_PAYLOAD,
        RX_WAIT_CRC_HIGH,
        RX_WAIT_CRC_LOW
    };
    
public:
    ProtocolHandler(Stream* s) : serial(s), rxState(RX_WAIT_START), rxByteCount(0), rxStartTime(0) {}
    
    // Send a frame
    bool sendFrame(uint8_t commandId, const uint8_t* payload, uint16_t length) {
        if (length > PROTO_MAX_PAYLOAD) return false;
        
        // Build CRC data: CMD + LEN_H + LEN_L + PAYLOAD
        uint8_t crcBuffer[3 + PROTO_MAX_PAYLOAD];
        crcBuffer[0] = commandId;
        crcBuffer[1] = (length >> 8) & 0xFF;
        crcBuffer[2] = length & 0xFF;
        if (length > 0) {
            memcpy(crcBuffer + 3, payload, length);
        }
        
        uint16_t crc = CRC16::calculate(crcBuffer, 3 + length);
        
        // Send frame
        serial->write(PROTO_START_BYTE);
        serial->write(commandId);
        serial->write((length >> 8) & 0xFF);
        serial->write(length & 0xFF);
        if (length > 0) {
            serial->write(payload, length);
        }
        serial->write((crc >> 8) & 0xFF);
        serial->write(crc & 0xFF);
        serial->flush();
        
        return true;
    }
    
    // Send simple command with no payload
    bool sendCommand(uint8_t commandId) {
        return sendFrame(commandId, nullptr, 0);
    }
    
    // Send ACK
    bool sendAck() {
        return sendCommand(CMD_ACK);
    }
    
    // Send error
    bool sendError(uint8_t errorCode) {
        uint8_t payload[1] = { errorCode };
        return sendFrame(CMD_ERROR, payload, 1);
    }
    
    // Receive a frame (non-blocking, call repeatedly in loop)
    // Returns true when a complete valid frame is received
    bool receiveFrame(ProtocolFrame& frame) {
        while (serial->available()) {
            uint8_t byte = serial->read();
            
            // Timeout check
            if (rxState != RX_WAIT_START) {
                if (millis() - rxStartTime > PROTO_TIMEOUT_MS) {
                    rxState = RX_WAIT_START;
                    rxByteCount = 0;
                }
            }
            
            switch (rxState) {
                case RX_WAIT_START:
                    if (byte == PROTO_START_BYTE) {
                        rxFrame.startByte = byte;
                        rxState = RX_WAIT_CMD;
                        rxStartTime = millis();
                    }
                    break;
                    
                case RX_WAIT_CMD:
                    rxFrame.commandId = byte;
                    rxState = RX_WAIT_LEN_HIGH;
                    break;
                    
                case RX_WAIT_LEN_HIGH:
                    rxFrame.length = (uint16_t)byte << 8;
                    rxState = RX_WAIT_LEN_LOW;
                    break;
                    
                case RX_WAIT_LEN_LOW:
                    rxFrame.length |= byte;
                    if (rxFrame.length > PROTO_MAX_PAYLOAD) {
                        rxState = RX_WAIT_START;
                        rxByteCount = 0;
                    } else if (rxFrame.length == 0) {
                        rxState = RX_WAIT_CRC_HIGH;
                    } else {
                        rxByteCount = 0;
                        rxState = RX_WAIT_PAYLOAD;
                    }
                    break;
                    
                case RX_WAIT_PAYLOAD:
                    rxFrame.payload[rxByteCount++] = byte;
                    if (rxByteCount >= rxFrame.length) {
                        rxState = RX_WAIT_CRC_HIGH;
                    }
                    break;
                    
                case RX_WAIT_CRC_HIGH:
                    rxFrame.crc = (uint16_t)byte << 8;
                    rxState = RX_WAIT_CRC_LOW;
                    break;
                    
                case RX_WAIT_CRC_LOW:
                    rxFrame.crc |= byte;
                    
                    // Verify CRC
                    uint8_t crcBuffer[3 + PROTO_MAX_PAYLOAD];
                    crcBuffer[0] = rxFrame.commandId;
                    crcBuffer[1] = (rxFrame.length >> 8) & 0xFF;
                    crcBuffer[2] = rxFrame.length & 0xFF;
                    if (rxFrame.length > 0) {
                        memcpy(crcBuffer + 3, rxFrame.payload, rxFrame.length);
                    }
                    uint16_t calculatedCrc = CRC16::calculate(crcBuffer, 3 + rxFrame.length);
                    
                    if (calculatedCrc == rxFrame.crc) {
                        // Valid frame received!
                        frame = rxFrame;
                        rxState = RX_WAIT_START;
                        rxByteCount = 0;
                        return true;
                    } else {
                        // CRC mismatch
                        rxState = RX_WAIT_START;
                        rxByteCount = 0;
                    }
                    break;
            }
        }
        
        return false;
    }
};

#endif // ARDUINO_PROTOCOL_H
