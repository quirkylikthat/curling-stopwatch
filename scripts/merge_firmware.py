#!/usr/bin/env python3
"""merge_firmware.py -- combine the four build outputs into ONE flashable image.

PlatformIO produces separate files (bootloader, partition table, boot_app0 and
the app). Flashing them one by one is fiddly, so the release workflow merges
them into a single file that is written at address 0x0:

    esptool.py --chip esp32s3 write_flash 0x0 curling-timer-X.Y.Z-merged.bin

Offsets come from the partition table itself (not hard-coded), so a change to
the partition layout can't silently produce a broken image. Standard library
only; run `python3 -m unittest discover -s scripts` for its tests.
"""

import argparse
import struct
import sys

# ESP32-S3 fixed addresses (the bootloader lives at 0x0 on the S3, not 0x1000).
BOOTLOADER_OFFSET = 0x0
PARTITION_TABLE_OFFSET = 0x8000

_ENTRY = struct.Struct("<2sBBII16sI")  # magic, type, subtype, offset, size, label, flags
_MAGIC = b"\xaa\x50"
_TYPE_APP, _TYPE_DATA = 0x00, 0x01
_APP_SUBTYPES = (0x00, 0x10)  # factory, ota_0
_DATA_OTA = 0x00  # "otadata": where boot_app0.bin goes


class MergeError(Exception):
    pass


def parse_partition_table(blob):
    """Return a list of (type, subtype, offset, size, label) from partitions.bin."""
    entries = []
    for pos in range(0, len(blob) - _ENTRY.size + 1, _ENTRY.size):
        magic, ptype, subtype, offset, size, label, _flags = _ENTRY.unpack_from(blob, pos)
        if magic != _MAGIC:
            break  # MD5 marker (0xEBEB) or erased flash: end of table
        entries.append((ptype, subtype, offset, size, label.rstrip(b"\x00").decode("ascii", "replace")))
    return entries


def find_layout(entries):
    """Return (app_offset, app_size, otadata_offset_or_None)."""
    app = next((e for e in entries if e[0] == _TYPE_APP and e[1] in _APP_SUBTYPES), None)
    if app is None:
        raise MergeError("no app partition (factory/ota_0) in the partition table")
    ota = next((e for e in entries if e[0] == _TYPE_DATA and e[1] == _DATA_OTA), None)
    return app[2], app[3], (ota[2] if ota else None)


def merge(parts):
    """parts: list of (offset, label, bytes). Returns the merged image (gaps = 0xFF)."""
    ordered = sorted(parts, key=lambda p: p[0])
    for (off_a, name_a, data_a), (off_b, name_b, _) in zip(ordered, ordered[1:]):
        if off_a + len(data_a) > off_b:
            raise MergeError(
                f"{name_a} (0x{off_a:x}, {len(data_a)} bytes) overlaps {name_b} (0x{off_b:x})"
            )
    end = max(off + len(data) for off, _, data in ordered)
    image = bytearray(b"\xff" * end)  # 0xFF = erased flash, so gaps stay "empty"
    for off, _, data in ordered:
        image[off : off + len(data)] = data
    return bytes(image)


def build_image(bootloader, partitions, boot_app0, app):
    entries = parse_partition_table(partitions)
    app_offset, app_size, ota_offset = find_layout(entries)
    if len(app) > app_size:
        raise MergeError(f"app is {len(app)} bytes but its partition holds only {app_size}")
    parts = [
        (BOOTLOADER_OFFSET, "bootloader", bootloader),
        (PARTITION_TABLE_OFFSET, "partition table", partitions),
        (app_offset, "app", app),
    ]
    if ota_offset is not None:
        parts.append((ota_offset, "boot_app0 (otadata)", boot_app0))
    return merge(parts), parts


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--bootloader", required=True)
    ap.add_argument("--partitions", required=True)
    ap.add_argument("--boot-app0", required=True)
    ap.add_argument("--app", required=True)
    ap.add_argument("-o", "--output", required=True)
    args = ap.parse_args(argv)

    def read(path):
        with open(path, "rb") as f:
            return f.read()

    try:
        image, parts = build_image(
            read(args.bootloader), read(args.partitions), read(args.boot_app0), read(args.app)
        )
    except (MergeError, OSError) as e:
        print(f"merge_firmware: {e}", file=sys.stderr)
        return 1

    with open(args.output, "wb") as f:
        f.write(image)
    for off, name, data in sorted(parts):
        print(f"  0x{off:06x}  {len(data):>8} bytes  {name}")
    print(f"wrote {args.output} ({len(image)} bytes); flash it at 0x0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
