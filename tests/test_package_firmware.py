#!/usr/bin/env python3
"""Validation tests for CI UF2 verification and release version checks."""
import importlib.util
import json
import struct
import tempfile
from pathlib import Path

source = Path(__file__).resolve().parents[1] / 'scripts/package_firmware.py'
spec = importlib.util.spec_from_file_location('package_firmware', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def synthetic_uf2(count=3):
    blocks = []
    for i in range(count):
        chunk = bytearray(512)
        struct.pack_into('<8I', chunk, 0, 0x0A324655, 0x9E5D5157, 0x2000, 0x10000000 + i * 256, 256, i, count, 0xE48BFF56)
        struct.pack_into('<I', chunk, 508, 0x0AB16F30)
        blocks.append(bytes(chunk))
    return b''.join(blocks)


def main():
    uf2 = synthetic_uf2()
    assert module.inspect_uf2(uf2) == 3
    assert module.firmware_version('-DFW_VERSION=\\"0.4.1\\"') == '0.4.1'
    for blob in [b'', uf2[:-1], b'BAD!' + uf2[4:], uf2[:512] + uf2[:512] + uf2[1024:]]:
        try:
            module.inspect_uf2(blob)
        except ValueError:
            pass
        else:
            raise AssertionError('Corrupt UF2 accepted')
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / 'build.json'
        path.write_text(json.dumps({'version':'0.4.1'}))
        module.verify_tag('v0.4.1', path)
        try:
            module.verify_tag('v0.4.2', path)
        except ValueError:
            pass
        else:
            raise AssertionError('Incorrect release tag accepted')
    print('Firmware packaging checks OK')


if __name__ == '__main__':
    main()
