# PhotoPainter EPD native frame — PPEF v1

The `.epd` extension identifies a ready-to-display PhotoPainter 7-color frame. Firmware also accepts `.edp` as a spelling alias. The format is **not** the undocumented Waveshare proprietary format. It is explicitly defined here for this project. All integers are little-endian.

| Offset | Bytes | Field | Value |
|---|---:|---|---|
| 0 | 4 | Signature | ASCII `PPEF` |
| 4 | 2 | Version | `1` |
| 6 | 2 | Physical width | `800` |
| 8 | 2 | Physical height | `480` |
| 10 | 1 | Pixel format | `1` (4-bit ACeP-7) |
| 11 | 1 | Flags | `0` |
| 12 | 4 | Payload length | `192000` |
| 16 | 4 | CRC32 | IEEE CRC32 of payload, starting at offset 24 |
| 20 | 4 | Reserved | `0` |
| 24 | 192000 | Payload | packed pixels, row-major |

**One packed byte is two pixels:** high nibble is even physical X, low nibble is odd physical X. Rows proceed left to right, top to bottom, no padding. Valid palette indices: `0=black`, `1=white`, `2=green`, `3=blue`, `4=red`, `5=yellow`, `6=orange`. The value `7` and all other indices are invalid.

Total size is **192024 bytes**. The payload is **always landscape 800×480 in the physical panel coordinate system**, even when the user's source photo is portrait. PhotoPainter Studio pre-rotates the quantized output into that physical order. The firmware **must not rotate native frames again** when `orientation` in `settings.conf` changes. That setting affects only JPEG and BMP sources.

## Reader security and power behavior

Before powering the panel, the RP2040 checks the signature, header version/format, declared length, exact file size, palette values and CRC32. It then seeks back to offset 24 and streams rows to EPD RAM, refreshing only after a complete read. This adds one sequential SD validation pass but avoids a potentially long erroneous screen refresh on a truncated or corrupt file. The CRC is not a cryptographic signature.

Original BMP/JPEG paths and their `.epc` caches are preserved. Native EPD files are **not additionally cached**. The existing last-displayed marker uses the file's length, timestamp and sampled file bytes to skip unnecessary repeats.

## Example microSD

```text
SD:/
  settings.conf
  pic/
    001_family.epd
    002_landscape.epd
  cache/  # only for BMP/JPEG conversions
```

## Compatibility

Introduced with firmware **0.4.0** and Studio v0.4.0. Earlier firmware releases do not recognize the format. Export BMP instead if using earlier firmware.
