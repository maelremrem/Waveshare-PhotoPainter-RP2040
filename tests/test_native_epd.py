"""Inspect the canonical native-frame example without external dependencies."""
from pathlib import Path
import struct
import zlib

source = Path(__file__).resolve().parents[1] / 'sdcard/pic/00000_demo.epd'
data = source.read_bytes()
assert len(data) == 192024
magic, version, width, height, fmt, flags, length, crc, reserved = struct.unpack('<4sHHHBBIII', data[:24])
assert (magic, version, width, height) == (b'PPEF', 1, 800, 480)
assert (fmt, flags, length, reserved) == (1, 0, 192000, 0)
assert zlib.crc32(data[24:]) == crc
assert all((b >> 4) <= 6 and (b & 15) <= 6 for b in data[24:])
print('Native EPD fixture: PASS')
