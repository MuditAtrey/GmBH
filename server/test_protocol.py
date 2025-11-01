#!/usr/bin/env python3
"""
Binary Protocol Test Suite
Validates encoding/decoding and command building
"""

import sys
sys.path.insert(0, '.')

import binary_protocol as proto

def test_basic_encoding():
    """Test basic frame encoding/decoding"""
    print("=" * 60)
    print("TEST 1: Basic Frame Encoding/Decoding")
    print("=" * 60)
    
    # Encode a simple ping
    frame = proto.build_ping()
    print(f"PING frame: {frame.hex()}")
    assert frame[0] == proto.PROTO_START_BYTE
    assert frame[1] == proto.CommandID.CMD_PING
    
    # Decode it
    decoded = proto.decode_frame(frame)
    assert decoded is not None
    cmd_id, payload = decoded
    assert cmd_id == proto.CommandID.CMD_PING
    assert len(payload) == 0
    
    print("✅ PING encode/decode OK\n")


def test_led_commands():
    """Test LED command encoding"""
    print("=" * 60)
    print("TEST 2: LED Commands")
    print("=" * 60)
    
    # LED ON
    frame = proto.build_led_set(True)
    print(f"LED ON frame:  {frame.hex()}")
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_LED_SET
    assert payload[0] == 1
    print("✅ LED ON OK")
    
    # LED OFF
    frame = proto.build_led_set(False)
    print(f"LED OFF frame: {frame.hex()}")
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_LED_SET
    assert payload[0] == 0
    print("✅ LED OFF OK")
    
    # LED BLINK
    frame = proto.build_led_blink(500)
    print(f"LED BLINK 500ms frame: {frame.hex()}")
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_LED_BLINK
    parser = proto.PayloadParser(payload)
    duration = parser.read_uint16()
    assert duration == 500
    print(f"✅ LED BLINK OK (duration={duration}ms)\n")


def test_oled_commands():
    """Test OLED command encoding"""
    print("=" * 60)
    print("TEST 3: OLED Commands")
    print("=" * 60)
    
    # OLED Clear
    frame = proto.build_oled_clear()
    print(f"OLED CLEAR frame: {frame.hex()}")
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_OLED_CLEAR
    print("✅ OLED CLEAR OK")
    
    # OLED Text
    text = "Hello Arduino!"
    frame = proto.build_oled_text(10, 20, text)
    print(f"OLED TEXT frame: {frame.hex()}")
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_OLED_TEXT
    
    parser = proto.PayloadParser(payload)
    x = parser.read_uint8()
    y = parser.read_uint8()
    decoded_text = parser.read_string()
    
    assert x == 10
    assert y == 20
    assert decoded_text == text
    print(f"✅ OLED TEXT OK (x={x}, y={y}, text='{decoded_text}')\n")


def test_encoder_data():
    """Test encoder data response parsing"""
    print("=" * 60)
    print("TEST 4: Encoder Data Response")
    print("=" * 60)
    
    # Build encoder data response
    builder = proto.PayloadBuilder()
    builder.add_int16(42)   # position
    builder.add_uint8(5)    # velocity
    builder.add_uint8(1)    # button pressed
    
    frame = proto.encode_frame(proto.CommandID.CMD_ENCODER_DATA, builder.get_payload())
    print(f"ENCODER_DATA frame: {frame.hex()}")
    
    # Decode
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_ENCODER_DATA
    
    data = proto.parse_encoder_data(payload)
    assert data is not None
    assert data['position'] == 42
    assert data['velocity'] == 5
    assert data['button_pressed'] == True
    
    print(f"✅ ENCODER_DATA OK: {data}\n")


def test_complex_payload():
    """Test complex multi-type payload"""
    print("=" * 60)
    print("TEST 5: Complex Payload")
    print("=" * 60)
    
    # Build complex payload
    builder = proto.PayloadBuilder()
    builder.add_uint8(255)
    builder.add_int16(-1234)
    builder.add_int32(1000000)
    builder.add_float(3.14159)
    builder.add_string("Test String")
    builder.add_bytes(b'\x01\x02\x03\x04')
    
    frame = proto.encode_frame(proto.CommandID.CMD_DATA_ARRAY, builder.get_payload())
    print(f"Complex frame ({len(frame)} bytes): {frame.hex()}")
    
    # Decode
    cmd_id, payload = proto.decode_frame(frame)
    assert cmd_id == proto.CommandID.CMD_DATA_ARRAY
    
    parser = proto.PayloadParser(payload)
    v1 = parser.read_uint8()
    v2 = parser.read_int16()
    v3 = parser.read_int32()
    v4 = parser.read_float()
    v5 = parser.read_string()
    v6 = parser.read_bytes(4)
    
    assert v1 == 255
    assert v2 == -1234
    assert v3 == 1000000
    assert abs(v4 - 3.14159) < 0.0001
    assert v5 == "Test String"
    assert v6 == b'\x01\x02\x03\x04'
    
    print(f"✅ Complex payload OK")
    print(f"   uint8={v1}, int16={v2}, int32={v3}")
    print(f"   float={v4:.5f}, string='{v5}'")
    print(f"   bytes={v6.hex()}\n")


def test_crc_validation():
    """Test CRC validation"""
    print("=" * 60)
    print("TEST 6: CRC Validation")
    print("=" * 60)
    
    # Build valid frame
    frame = proto.build_led_blink(500)
    print(f"Valid frame: {frame.hex()}")
    
    # Should decode successfully
    result = proto.decode_frame(frame)
    assert result is not None
    print("✅ Valid CRC passes")
    
    # Corrupt the frame
    corrupted = bytearray(frame)
    corrupted[4] ^= 0xFF  # Flip bits in payload
    
    print(f"Corrupted frame: {bytes(corrupted).hex()}")
    result = proto.decode_frame(bytes(corrupted))
    assert result is None
    print("✅ Corrupted CRC detected\n")


def test_frame_sizes():
    """Test various frame sizes"""
    print("=" * 60)
    print("TEST 7: Frame Size Limits")
    print("=" * 60)
    
    # Empty payload
    frame = proto.build_ping()
    print(f"Empty payload: {len(frame)} bytes")
    assert len(frame) == 6  # START + CMD + LEN(2) + CRC(2)
    
    # Small payload
    frame = proto.build_led_blink(500)
    print(f"Small payload (2 bytes): {len(frame)} bytes")
    assert len(frame) == 8  # 6 + 2
    
    # Large string
    large_text = "X" * 200
    frame = proto.build_oled_text(0, 0, large_text)
    print(f"Large string (200 chars): {len(frame)} bytes")
    
    decoded = proto.decode_frame(frame)
    assert decoded is not None
    cmd_id, payload = decoded
    parser = proto.PayloadParser(payload)
    parser.read_uint8()  # x
    parser.read_uint8()  # y
    text = parser.read_string()
    assert text == large_text
    
    print("✅ Frame sizes OK\n")


def run_all_tests():
    """Run all tests"""
    print("\n")
    print("╔" + "=" * 58 + "╗")
    print("║" + " " * 10 + "BINARY PROTOCOL TEST SUITE" + " " * 22 + "║")
    print("╚" + "=" * 58 + "╝")
    print()
    
    try:
        test_basic_encoding()
        test_led_commands()
        test_oled_commands()
        test_encoder_data()
        test_complex_payload()
        test_crc_validation()
        test_frame_sizes()
        
        print("=" * 60)
        print("🎉 ALL TESTS PASSED!")
        print("=" * 60)
        print()
        print("Next steps:")
        print("1. Upload ESP8266 firmware: pio run -e esp8266_programmer -t upload")
        print("2. Upload Arduino firmware: pio run -e uno_r4_minima -t upload")
        print("3. Wire the devices (see BINARY_PROTOCOL_GUIDE.md)")
        print("4. Start server: python3 server/firmware_server_binary.py")
        print("5. Open web UI: http://localhost:5001")
        print()
        
        return True
        
    except AssertionError as e:
        print(f"\n❌ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        return False
    except Exception as e:
        print(f"\n❌ ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == '__main__':
    success = run_all_tests()
    sys.exit(0 if success else 1)
