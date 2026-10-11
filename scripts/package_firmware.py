#!/usr/bin/env python3
"""Validate PlatformIO's UF2 output and create GitHub Pages download metadata."""
import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path
import re
import shutil
import struct

ROOT = Path(__file__).resolve().parents[1]
UF2_PATH = ROOT / '.pio/build/photopainter_rp2040/firmware.uf2'
ARTIFACT = ROOT / 'tools/firmware/PhotoPainter-RP2040-main.uf2'
META_PATH = ROOT / 'tools/firmware/build.json'


def firmware_version(ini_text):
    match = re.search(r'-DFW_VERSION=\\?"([0-9]+(?:\.[0-9]+){1,3})\\?"', ini_text)
    if not match:
        raise ValueError('Cannot find -DFW_VERSION in platformio.ini')
    return match.group(1)


def inspect_uf2(data):
    if not data or len(data) % 512:
        raise ValueError('UF2 is empty or not aligned to 512-byte blocks')
    expected_count = len(data) // 512
    for index in range(expected_count):
        block = data[index * 512:(index + 1) * 512]
        magic0, magic1, flags, address, payload_size, block_no, block_count, family = struct.unpack_from('<8I', block)
        trailer = struct.unpack_from('<I', block, 508)[0]
        if magic0 != 0x0A324655 or magic1 != 0x9E5D5157 or trailer != 0x0AB16F30:
            raise ValueError(f'Invalid UF2 magic at block {index}')
        if block_no != index or block_count != expected_count:
            raise ValueError(f'UF2 block numbering mismatch at block {index}')
        if payload_size < 1 or payload_size > 476:
            raise ValueError(f'Invalid UF2 payload size at block {index}')
        if flags & 0x2000 and family != 0xE48BFF56:
            raise ValueError(f'Wrong UF2 family (0x{family:08x}); expected RP2040')
        if not 0x10000000 <= address < 0x11000000:
            raise ValueError(f'Unexpected UF2 flash address 0x{address:08x}')
    return expected_count


def package():
    version = firmware_version((ROOT / 'platformio.ini').read_text())
    data = UF2_PATH.read_bytes()
    blocks = inspect_uf2(data)
    ARTIFACT.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(UF2_PATH, ARTIFACT)
    repository = os.getenv('GITHUB_REPOSITORY', '')
    if repository and not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', repository):
        raise ValueError('Invalid GITHUB_REPOSITORY')
    meta = {
        'target': 'Waveshare PhotoPainter RP2040 (F), ACeP 7.3-inch 800x480',
        'version': version,
        'repository': repository,
        'commit': os.getenv('GITHUB_SHA', ''),
        'built_at': datetime.now(timezone.utc).isoformat(timespec='seconds'),
        'file': 'PhotoPainter-RP2040-main.uf2',
        'bytes': len(data),
        'blocks': blocks,
        'sha256': hashlib.sha256(data).hexdigest(),
    }
    META_PATH.write_text(json.dumps(meta, indent=2, ensure_ascii=False) + '\n')
    print(f'UF2 ready: {len(data)} bytes, {blocks} blocks; firmware {version}')
    print(f'Build metadata: {META_PATH}')


def verify_tag(tag, path):
    meta = json.loads(path.read_text())
    if tag != 'v' + meta['version']:
        raise ValueError(f'Tag {tag!r} must match firmware version v{meta["version"]}')
    print('Tag and compiled firmware version match:', tag)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--verify-tag')
    parser.add_argument('--metadata', type=Path, default=META_PATH)
    args = parser.parse_args()
    if args.verify_tag:
        verify_tag(args.verify_tag, args.metadata)
    else:
        package()
