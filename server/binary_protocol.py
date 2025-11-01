#!/usr/bin/env python3
"""
Binary Protocol Encoder/Decoder for Arduino Communication
Matches the C++ ArduinoProtocol.h implementation
"""

import struct
from typing import Optional, Tuple, List
from enum import IntEnum

# Protocol constants
PROTO_START_BYTE = 0xAA
PROTO_MAX_PAYLOAD = 1024

class CommandID(IntEnum):
    """Command IDs matching C++ enum"""
    # Basic commands
    CMD_PING = 0x01
    CMD_PONG = 0x02
    CMD_ERROR = 0x03
    CMD_ACK = 0x04
    
    # LED commands
    CMD_LED_SET = 0x10
    CMD_LED_BLINK = 0x11
    CMD_LED_PATTERN = 0x12
    
    # Sensor commands
    CMD_SENSOR_READ = 0x20
    CMD_SENSOR_DATA = 0x21
    CMD_SENSOR_CONFIG = 0x22
    
    # Rotary encoder
    CMD_ENCODER_READ = 0x30
    CMD_ENCODER_DATA = 0x31
    CMD_ENCODER_RESET = 0x32
    
    # OLED display
    CMD_OLED_CLEAR = 0x40
    CMD_OLED_TEXT = 0x41
    CMD_OLED_PIXEL = 0x42
    CMD_OLED_LINE = 0x43
    CMD_OLED_RECT = 0x44
    CMD_OLED_BITMAP = 0x45
    
    # Generic data
    CMD_DATA_UINT8 = 0x50
    CMD_DATA_INT16 = 0x51
    CMD_DATA_INT32 = 0x52
    CMD_DATA_FLOAT = 0x53
    CMD_DATA_STRING = 0x54
    CMD_DATA_ARRAY = 0x55


class ErrorCode(IntEnum):
    """Error codes matching C++ enum"""
    PROTO_ERR_OK = 0x00
    PROTO_ERR_INVALID_CMD = 0x01
    PROTO_ERR_INVALID_CRC = 0x02
    PROTO_ERR_TIMEOUT = 0x03
    PROTO_ERR_BUFFER_OVERFLOW = 0x04
    PROTO_ERR_INVALID_PARAM = 0x05
    PROTO_ERR_NOT_READY = 0x06


class PayloadBuilder:
    """Helper to build binary payloads"""
    
    def __init__(self, max_size: int = PROTO_MAX_PAYLOAD):
        self.buffer = bytearray()
        self.max_size = max_size
    
    def add_uint8(self, value: int) -> bool:
        if len(self.buffer) + 1 > self.max_size:
            return False
        self.buffer.append(value & 0xFF)
        return True
    
    def add_int16(self, value: int) -> bool:
        if len(self.buffer) + 2 > self.max_size:
            return False
        self.buffer.extend(struct.pack('>h', value))
        return True
    
    def add_uint16(self, value: int) -> bool:
        if len(self.buffer) + 2 > self.max_size:
            return False
        self.buffer.extend(struct.pack('>H', value))
        return True
    
    def add_int32(self, value: int) -> bool:
        if len(self.buffer) + 4 > self.max_size:
            return False
        self.buffer.extend(struct.pack('>i', value))
        return True
    
    def add_float(self, value: float) -> bool:
        if len(self.buffer) + 4 > self.max_size:
            return False
        self.buffer.extend(struct.pack('>f', value))
        return True
    
    def add_string(self, value: str) -> bool:
        encoded = value.encode('utf-8')
        if len(self.buffer) + 1 + len(encoded) > self.max_size:
            return False
        if len(encoded) > 255:
            return False
        self.buffer.append(len(encoded))
        self.buffer.extend(encoded)
        return True
    
    def add_bytes(self, data: bytes) -> bool:
        if len(self.buffer) + len(data) > self.max_size:
            return False
        self.buffer.extend(data)
        return True
    
    def get_payload(self) -> bytes:
        return bytes(self.buffer)


class PayloadParser:
    """Helper to parse binary payloads"""
    
    def __init__(self, payload: bytes):
        self.buffer = payload
        self.position = 0
    
    def remaining(self) -> int:
        return len(self.buffer) - self.position
    
    def read_uint8(self) -> Optional[int]:
        if self.position + 1 > len(self.buffer):
            return None
        value = self.buffer[self.position]
        self.position += 1
        return value
    
    def read_int16(self) -> Optional[int]:
        if self.position + 2 > len(self.buffer):
            return None
        value = struct.unpack('>h', self.buffer[self.position:self.position+2])[0]
        self.position += 2
        return value
    
    def read_uint16(self) -> Optional[int]:
        if self.position + 2 > len(self.buffer):
            return None
        value = struct.unpack('>H', self.buffer[self.position:self.position+2])[0]
        self.position += 2
        return value
    
    def read_int32(self) -> Optional[int]:
        if self.position + 4 > len(self.buffer):
            return None
        value = struct.unpack('>i', self.buffer[self.position:self.position+4])[0]
        self.position += 4
        return value
    
    def read_float(self) -> Optional[float]:
        if self.position + 4 > len(self.buffer):
            return None
        value = struct.unpack('>f', self.buffer[self.position:self.position+4])[0]
        self.position += 4
        return value
    
    def read_string(self) -> Optional[str]:
        length = self.read_uint8()
        if length is None or self.position + length > len(self.buffer):
            return None
        value = self.buffer[self.position:self.position+length].decode('utf-8', errors='replace')
        self.position += length
        return value
    
    def read_bytes(self, length: int) -> Optional[bytes]:
        if self.position + length > len(self.buffer):
            return None
        value = self.buffer[self.position:self.position+length]
        self.position += length
        return value


def crc16_ccitt(data: bytes) -> int:
    """Calculate CRC-16 CCITT checksum"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ 0x1021
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


def encode_frame(command_id: int, payload: bytes = b'') -> bytes:
    """
    Encode a protocol frame
    
    Returns: Complete frame with START_BYTE, CMD, LENGTH, PAYLOAD, CRC
    """
    if len(payload) > PROTO_MAX_PAYLOAD:
        raise ValueError(f"Payload too large: {len(payload)} > {PROTO_MAX_PAYLOAD}")
    
    # Build CRC data: CMD + LENGTH + PAYLOAD
    length = len(payload)
    crc_data = bytearray()
    crc_data.append(command_id)
    crc_data.extend(struct.pack('>H', length))
    crc_data.extend(payload)
    
    crc = crc16_ccitt(bytes(crc_data))
    
    # Build complete frame
    frame = bytearray()
    frame.append(PROTO_START_BYTE)
    frame.append(command_id)
    frame.extend(struct.pack('>H', length))
    frame.extend(payload)
    frame.extend(struct.pack('>H', crc))
    
    return bytes(frame)


def decode_frame(data: bytes) -> Optional[Tuple[int, bytes]]:
    """
    Decode a protocol frame
    
    Returns: (command_id, payload) or None if invalid
    """
    if len(data) < 6:  # Minimum: START + CMD + LEN(2) + CRC(2)
        return None
    
    if data[0] != PROTO_START_BYTE:
        return None
    
    command_id = data[1]
    length = struct.unpack('>H', data[2:4])[0]
    
    if len(data) < 6 + length:
        return None
    
    payload = data[4:4+length]
    received_crc = struct.unpack('>H', data[4+length:6+length])[0]
    
    # Verify CRC
    crc_data = data[1:4+length]  # CMD + LENGTH + PAYLOAD
    calculated_crc = crc16_ccitt(crc_data)
    
    if received_crc != calculated_crc:
        return None
    
    return (command_id, payload)


# High-level command builders
def build_ping() -> bytes:
    """Build PING command"""
    return encode_frame(CommandID.CMD_PING)


def build_led_set(state: bool) -> bytes:
    """Build LED_SET command"""
    builder = PayloadBuilder()
    builder.add_uint8(1 if state else 0)
    return encode_frame(CommandID.CMD_LED_SET, builder.get_payload())


def build_led_blink(duration_ms: int) -> bytes:
    """Build LED_BLINK command"""
    builder = PayloadBuilder()
    builder.add_uint16(duration_ms)
    return encode_frame(CommandID.CMD_LED_BLINK, builder.get_payload())


def build_oled_text(x: int, y: int, text: str) -> bytes:
    """Build OLED_TEXT command"""
    builder = PayloadBuilder()
    builder.add_uint8(x)
    builder.add_uint8(y)
    builder.add_string(text)
    return encode_frame(CommandID.CMD_OLED_TEXT, builder.get_payload())


def build_oled_clear() -> bytes:
    """Build OLED_CLEAR command"""
    return encode_frame(CommandID.CMD_OLED_CLEAR)


def build_encoder_read() -> bytes:
    """Build ENCODER_READ command"""
    return encode_frame(CommandID.CMD_ENCODER_READ)


# Response parsers
def parse_encoder_data(payload: bytes) -> Optional[dict]:
    """Parse ENCODER_DATA response"""
    parser = PayloadParser(payload)
    position = parser.read_int16()
    velocity = parser.read_uint8()
    button = parser.read_uint8()
    
    if position is None:
        return None
    
    return {
        'position': position,
        'velocity': velocity if velocity is not None else 0,
        'button_pressed': button == 1 if button is not None else False
    }


def parse_sensor_data(payload: bytes) -> Optional[dict]:
    """Parse SENSOR_DATA response"""
    parser = PayloadParser(payload)
    sensor_id = parser.read_uint8()
    value = parser.read_int16()
    
    if sensor_id is None or value is None:
        return None
    
    return {
        'sensor_id': sensor_id,
        'value': value
    }


if __name__ == '__main__':
    # Test encoding/decoding
    print("Binary Protocol Test")
    print("=" * 60)
    
    # Test LED blink
    frame = build_led_blink(500)
    print(f"LED Blink (500ms): {frame.hex()}")
    decoded = decode_frame(frame)
    if decoded:
        cmd_id, payload = decoded
        print(f"  Decoded: CMD=0x{cmd_id:02X}, Payload={payload.hex()}")
    
    # Test OLED text
    frame = build_oled_text(10, 20, "Hello Arduino!")
    print(f"\nOLED Text: {frame.hex()}")
    decoded = decode_frame(frame)
    if decoded:
        cmd_id, payload = decoded
        print(f"  Decoded: CMD=0x{cmd_id:02X}, Payload={payload.hex()}")
        parser = PayloadParser(payload)
        x = parser.read_uint8()
        y = parser.read_uint8()
        text = parser.read_string()
        print(f"  Parsed: x={x}, y={y}, text='{text}'")
    
    # Test encoder data parsing
    print("\nEncoder Data Test:")
    builder = PayloadBuilder()
    builder.add_int16(42)  # position
    builder.add_uint8(5)   # velocity
    builder.add_uint8(1)   # button pressed
    payload = builder.get_payload()
    frame = encode_frame(CommandID.CMD_ENCODER_DATA, payload)
    print(f"  Frame: {frame.hex()}")
    decoded = decode_frame(frame)
    if decoded:
        cmd_id, payload = decoded
        data = parse_encoder_data(payload)
        print(f"  Parsed: {data}")
    
    print("\n✅ Tests complete")
