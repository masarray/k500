#!/usr/bin/env python3
"""GEN4_AUDITION_PRESET_BANK_V1: deterministic, reversible K500 singing refinements.

The GEN3 listening reference remains authoritative. GEN4 is for hardware A/B
audition, NOT an accepted replacement or a proven sonic improvement.
Whitelisted edits: Mic A+B PEQ gains, Music PEQ gains, Main PEQ gains,
reverb pre-delay, 16-byte hardware name, checksum. Do not normalize or modify
other native data; always fail closed on donor SHA drift.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path

from k500_preset_lab import (FILE_SIZE, NAME_OFFSET, CHECKSUM_OFFSET,
    EQ_SECTIONS, build_device_slot_image, validate_device_slot_compatibility)

PROFILES = json.loads('''
[
  {
    "stem": "ARTIST_GEN3_ARI",
    "name": "ARTIST GEN4",
    "mic": {
      "1": -10.2,
      "6": -10.1,
      "7": -10.2
    },
    "music": {
      "5": -7.4
    },
    "main": {
      "3": -7.3
    },
    "predelayMs": 49,
    "donorSha256": "8ad4e2d6a953fdaf5e7ac844b9841c6ff6ec9f9fe6a5fc19071b07d2aa8a0d55"
  },
  {
    "stem": "PODCAST_REBORN",
    "name": "PODCAST GEN4",
    "mic": {
      "1": -7.7,
      "8": -8.1
    },
    "music": {
      "5": -5.9
    },
    "main": {
      "3": -5.4
    },
    "predelayMs": 12,
    "donorSha256": "a3001eafef5dde5993bafbcb56ac6834c806675ac4c36d0f55e0f154139bf170"
  },
  {
    "stem": "DANGDUT_GEN3_ARI",
    "name": "DANGDUT GEN4",
    "mic": {
      "6": -8.5,
      "7": -9.2
    },
    "music": {
      "2": -5.6,
      "5": -7.5
    },
    "main": {
      "3": -7.5
    },
    "predelayMs": 59,
    "donorSha256": "f23fa4612b0d75a171c1e3cce0d56cdd96d6cf081f6c7ed43504320467c941a1"
  },
  {
    "stem": "KARAOKE_ARTIST",
    "name": "KARAOKE GEN4",
    "mic": {
      "4": -2.5,
      "6": -8.1,
      "7": -9.1
    },
    "music": {
      "5": -6
    },
    "main": {
      "3": -5.3
    },
    "predelayMs": 34,
    "donorSha256": "2836a4bdaaf369c9d848950a85d3cbb4b900073ac1ef8faf18e28ab79f0ff1c9"
  },
  {
    "stem": "AKUSTIK_GEN3_ARI",
    "name": "ACOUSTIC GEN4",
    "mic": {
      "6": -8.5,
      "8": -9.1
    },
    "music": {
      "4": -6.7,
      "5": -7.9
    },
    "main": {
      "3": -8.5
    },
    "predelayMs": 47,
    "donorSha256": "de3f92cf1198f42b0d87bcaa554cadd6f668194429a0f04829bfbf4bd5705dc3"
  },
  {
    "stem": "IMAM_QORI_GEN3",
    "name": "QORI GEN4",
    "mic": {
      "1": -10.2,
      "6": -9.1,
      "8": -9.2
    },
    "music": {},
    "main": {
      "4": -7.7
    },
    "predelayMs": 54,
    "donorSha256": "3bbe02e77413f6b9bda1ba2b704d7a7beefcc23d977a47464109754760480ca3"
  },
  {
    "stem": "JAZZ_GEN3_ARI",
    "name": "JAZZ GEN4",
    "mic": {
      "6": -8.6,
      "7": -9
    },
    "music": {
      "4": -5.6,
      "5": -7.2
    },
    "main": {
      "3": -7.8
    },
    "predelayMs": 58,
    "donorSha256": "139e378f8d68af32e39d8fb717c7164e4af489926754d45cab6dc93cb162bf03"
  },
  {
    "stem": "ROCK_GEN3_ARI",
    "name": "ROCK GEN4",
    "mic": {
      "6": -8.2,
      "7": -8.6,
      "8": -9.2
    },
    "music": {
      "5": -7.7
    },
    "main": {
      "3": -7.3
    },
    "predelayMs": 52,
    "donorSha256": "1f841e113ff357b909f4fdff0f18bfd0bc72f3b2879edfc903c11b6214f79d9e"
  },
  {
    "stem": "MC_CERAMAH_GEN3",
    "name": "CERAMAH GEN4",
    "mic": {
      "2": -14.2,
      "6": -8.8,
      "7": -9.3,
      "8": -9.3
    },
    "music": {},
    "main": {
      "3": -7.4
    },
    "predelayMs": 12,
    "donorSha256": "f0d39c9bf1c168c973c7d60c53ba86713bfc20344f31d1ff419ea87e5ee12520"
  },
  {
    "stem": "ADZAN_MEKAH_GEN3",
    "name": "ADZAN GEN4",
    "mic": {
      "6": -9,
      "7": -9.7
    },
    "music": {
      "5": -7.4
    },
    "main": {
      "3": -7.4,
      "6": -6.8
    },
    "predelayMs": 72,
    "donorSha256": "9519acc2b77a14bdaef4098299f1f8594294e013171aaafcfc5daed03bebbac1"
  }
]
''')


def generate(source: Path, output: Path) -> list[dict]:
    expected = [f'{i:02d}_{p["stem"]}.k500' for i, p in enumerate(PROFILES, 1)]
    present = sorted(p.name for p in source.glob('[0-9][0-9]_*.k500')
                     if 1 <= int(p.name[:2]) <= 10)
    if present != expected:
        raise ValueError(f'GEN4 requires the exact GEN3 donor bank: {expected}; got {present}')
    output.mkdir(parents=True, exist_ok=True)
    report = []
    for i, profile in enumerate(PROFILES, 1):
        donor = expected[i-1]
        source_bytes = (source/donor).read_bytes()
        digest = hashlib.sha256(source_bytes).hexdigest()
        if digest != profile['donorSha256']:
            raise ValueError(f'GEN3 donor drift: {donor}, SHA-256={digest}')
        if len(source_bytes) != FILE_SIZE or source_bytes[0x0E] != 2:
            raise ValueError(f'Invalid GEN3 Bluetooth source: {donor}')
        if validate_device_slot_compatibility(source_bytes):
            raise ValueError(f'GEN3 donor incompatible: {donor}')
        result = bytearray(source_bytes)
        whitelist = {CHECKSUM_OFFSET}
        edits = []

        def gain(section: str, index: int, desired_db: float) -> None:
            base, count = EQ_SECTIONS[section]
            if not 1 <= index <= count:
                raise ValueError(f'{section} out-of-range band {index}')
            offset = base + 2 + (index-1)*8 + 6
            current = struct.unpack_from('<h', result, offset)[0]
            requested = round(desired_db*10)
            if abs(requested-current) > 8:
                raise ValueError(f'{section} band {index}: more than 0.8 dB step')
            struct.pack_into('<h', result, offset, requested)
            whitelist.update((offset, offset+1))
            edits.append({'section':section, 'band':index,
                          'fromDb':current/10, 'toDb':desired_db})

        # The A/B microphones have the same GEN3 PEQ foundation: keep them paired.
        for b, g in profile['mic'].items():
            for channel in ('micA', 'micB'):
                gain(channel, int(b), g)
        for b, g in profile['music'].items():
            gain('music', int(b), g)
        for b, g in profile['main'].items():
            gain('main', int(b), g)

        old_delay = struct.unpack_from('<H', result, 0xCA)[0]
        new_delay = profile['predelayMs']
        if not 0 < new_delay <= old_delay + 12 or new_delay < old_delay:
            raise ValueError(f'reverb predelay guard failed: {donor}')
        struct.pack_into('<H', result, 0xCA, new_delay)
        whitelist.update((0xCA, 0xCB))

        hwname = profile['name'].encode('ascii')
        if len(hwname) > 16:
            raise ValueError(f'hardware name too long: {hwname!r}')
        result[NAME_OFFSET:NAME_OFFSET+16] = hwname.ljust(16, b'\x00')
        whitelist.update(range(NAME_OFFSET, NAME_OFFSET+16))
        result[CHECKSUM_OFFSET] = 0
        result[CHECKSUM_OFFSET] = (-sum(result)) & 0xFF
        changed = {offset for offset,(a,b) in
                   enumerate(zip(source_bytes, result)) if a != b}
        if not changed <= whitelist:
            raise ValueError(f'unexpected changes: {donor} {sorted(changed-whitelist)}')
        if len(result) != FILE_SIZE or sum(result) % 256:
            raise ValueError(f'invalid size/checksum: {donor}')
        incompat = validate_device_slot_compatibility(result)
        if incompat:
            raise ValueError(f'native compatibility failure {donor}: {incompat}')
        if len(build_device_slot_image(result)) != 656:
            raise ValueError(f'native slot projection failed: {donor}')

        filename = f'{i+30:02d}_{profile["name"].replace(" ","_")}.k500'
        target = bytes(result)
        (output/filename).write_bytes(target)
        report.append({'number':i+30, 'file':filename, 'name':profile['name'],
                       'donor':donor, 'donorSha256':digest,
                       'sha256':hashlib.sha256(target).hexdigest(),
                       'changedOffsets':[f'0x{j:04X}' for j in sorted(changed)],
                       'predelayBeforeMs':old_delay, 'predelayAfterMs':new_delay,
                       'edits':edits})
    (output/'gen4-manifest.json').write_text(
        json.dumps(report, indent=2)+'\n', encoding='utf-8')
    return report

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    print('GEN4_AUDITION_PRESET_BANK_V1:',
          len(generate(args.source,args.output)),'strict-validated candidates')

if __name__ == '__main__':
    main()
