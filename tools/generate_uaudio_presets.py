#!/usr/bin/env python3
"""Generate K500 official UAUDIO companions (11–20) from the exact 01–10 donors.

Build-time derivation keeps the hardware-auditioned original files byte-identical.
Only the proven music source scalar (file 0x000E), hardware-visible name
(0x0454..0x0463), and checksum (0x0475) may differ.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from k500_preset_lab import (
    CHECKSUM_OFFSET,
    FILE_SIZE,
    NAME_OFFSET,
    NAME_VISIBLE_MAX,
    build_device_slot_image,
    patch_name,
    preset_name,
    validate_device_slot_compatibility,
)

MUSIC_SOURCE_FILE_OFFSET = 0x000E
UAUDIO_SOURCE_RAW = 5  # INPUT1=0, INPUT2=1, BT=2, UDISK=3, OPTIC=4, UAUDIO=5
# Native slot / hardware visible names must fit within 16 ASCII characters.
UAUDIO_NAMES = (
    "KONSER UAUDIO",
    "MC HOST UAUDIO",
    "DANGDUT UAUDIO",
    "POP ROCK UAUDIO",
    "KENANGAN UAUDIO",
    "SHOLAWAT UAUDIO",
    "JAZZ UAUDIO",
    "BLUES UAUDIO",
    "ACOUSTIC UAUDIO",
    "REGGAE UAUDIO",
)


def generate(source_dir: Path, output_dir: Path) -> list[dict]:
    source_files = sorted(source_dir.glob("[0-9][0-9]_*.k500"))
    # Fail closed if the official bank changes shape: never silently shift mappings.
    expected_prefixes = [f"{n:02d}_" for n in range(1, 11)]
    if len(source_files) != 10 or any(
        not file.name.startswith(prefix) for file, prefix in zip(source_files, expected_prefixes)
    ):
        raise ValueError("UAUDIO companions require the canonical 01–10 official donor bank")

    output_dir.mkdir(parents=True, exist_ok=True)
    manifest: list[dict] = []
    for n, (source_path, new_name) in enumerate(zip(source_files, UAUDIO_NAMES), start=11):
        source = source_path.read_bytes()
        error = validate_device_slot_compatibility(source)
        if error:
            raise ValueError(f"{source_path.name}: invalid donor: {'; '.join(error)}")
        if len(source) != FILE_SIZE or len(new_name.encode("ascii")) > NAME_VISIBLE_MAX:
            raise ValueError(f"{source_path.name}: size or hardware-name constraint failed")

        data = bytearray(source)
        touched: set[int] = set()
        patch_name(data, new_name, touched)
        data[MUSIC_SOURCE_FILE_OFFSET] = UAUDIO_SOURCE_RAW
        data[CHECKSUM_OFFSET] = 0
        data[CHECKSUM_OFFSET] = (-sum(data)) & 0xFF
        result = bytes(data)
        if validate_device_slot_compatibility(result):
            raise ValueError(f"{source_path.name}: generated UAUDIO file is not upload-safe")
        # An accepted image must survive strict 1144-byte -> 656-byte native projection.
        build_device_slot_image(result)
        if preset_name(result) != new_name or result[MUSIC_SOURCE_FILE_OFFSET] != UAUDIO_SOURCE_RAW:
            raise ValueError(f"{source_path.name}: UAUDIO source or embedded identity mismatch")
        permitted = set(range(NAME_OFFSET, NAME_OFFSET + NAME_VISIBLE_MAX))
        permitted.update((MUSIC_SOURCE_FILE_OFFSET, CHECKSUM_OFFSET))
        changed = {i for i, (a, b) in enumerate(zip(source, result)) if a != b}
        if not changed <= permitted:
            raise ValueError(f"{source_path.name}: unexpected mutations {sorted(changed - permitted)}")

        filename = f"{n:02d}_{source_path.stem[3:]}_UAUDIO.k500"
        (output_dir / filename).write_bytes(result)
        manifest.append({
            "file": filename,
            "name": new_name,
            "donor": source_path.name,
            "sourceRaw": UAUDIO_SOURCE_RAW,
            "donorSha256": hashlib.sha256(source).hexdigest(),
            "sha256": hashlib.sha256(result).hexdigest(),
            "changedOffsets": [f"0x{i:04X}" for i in sorted(changed)],
        })

    (output_dir / "uaudio-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    entries = generate(args.source, args.output)
    print(f"UAUDIO_PRESET_BANK_V1: {len(entries)} companions; "
          f"original 01–10 donors untouched; sourceRaw={UAUDIO_SOURCE_RAW}")


if __name__ == "__main__":
    main()
