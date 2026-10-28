#!/usr/bin/env python3
"""GEN4_AUDITION_PRESET_BANK_V2: meaningfully audible, test-only K500 refinements.

Use exact owner-preferred GEN3 BT donors. All changes preserve EQ type/f/Q,
large shelf gains, input source, crossover, alternate EQ and unknown bytes.
Do not claim hardware sonic acceptance until matched-level singing A/B.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from k500_preset_lab import (
    FILE_SIZE, NAME_OFFSET, CHECKSUM_OFFSET, EQ_SECTIONS,
    validate_device_slot_compatibility, build_device_slot_image
)

PROFILES = json.loads('''
[
  {
    "stem": "ARTIST_GEN3_ARI",
    "name": "ARTIST GEN4",
    "mic": {
      "1": 1.2,
      "4": -1.8,
      "6": 2.2,
      "7": -1.6,
      "8": -1.2
    },
    "music": {
      "4": -1,
      "5": -2.8,
      "6": -1
    },
    "main": {
      "3": 0.8,
      "4": -1.1
    },
    "fx": {
      "reverbLevel": 92,
      "echoLevel": 88,
      "decayMs": 2250,
      "predelayMs": 66
    },
    "donorSha256": "8ad4e2d6a953fdaf5e7ac844b9841c6ff6ec9f9fe6a5fc19071b07d2aa8a0d55"
  },
  {
    "stem": "PODCAST_REBORN",
    "name": "BALLAD GEN4",
    "mic": {
      "1": 1.3,
      "4": -1.6,
      "6": 2,
      "7": -1.4,
      "8": -1.1
    },
    "music": {
      "4": -1,
      "5": -2,
      "6": -0.6
    },
    "main": {
      "3": 0.7,
      "4": -0.8
    },
    "fx": {
      "reverbLevel": 83,
      "echoLevel": 71,
      "decayMs": 2470,
      "predelayMs": 54,
      "echoMs": 310,
      "repeat": 8,
      "mainMusic": 46,
      "surroundMusic": 36,
      "centerMusic": 25,
      "mainOutTrimDb": -2
    },
    "donorSha256": "a3001eafef5dde5993bafbcb56ac6834c806675ac4c36d0f55e0f154139bf170"
  },
  {
    "stem": "DANGDUT_GEN3_ARI",
    "name": "DANGDUT GEN4",
    "mic": {
      "1": 1.2,
      "4": -2,
      "6": 2,
      "7": -1.7,
      "8": -1.2
    },
    "music": {
      "2": 1.7,
      "3": -1,
      "5": -2.8,
      "6": -0.8
    },
    "main": {
      "2": -0.8,
      "3": 0.8,
      "4": -0.9
    },
    "fx": {
      "reverbLevel": 92,
      "echoLevel": 89,
      "decayMs": 2880,
      "predelayMs": 75
    },
    "donorSha256": "f23fa4612b0d75a171c1e3cce0d56cdd96d6cf081f6c7ed43504320467c941a1"
  },
  {
    "stem": "KARAOKE_ARTIST",
    "name": "KARAOKE GEN4",
    "mic": {
      "1": 1.5,
      "4": -2.5,
      "6": 2.4,
      "7": -1.6,
      "8": -1.3
    },
    "music": {
      "4": -1.2,
      "5": -3,
      "6": -1
    },
    "main": {
      "3": 1,
      "4": -1.1
    },
    "fx": {
      "reverbLevel": 92,
      "echoLevel": 88,
      "decayMs": 2100,
      "predelayMs": 58,
      "echoMs": 340,
      "repeat": 9
    },
    "donorSha256": "2836a4bdaaf369c9d848950a85d3cbb4b900073ac1ef8faf18e28ab79f0ff1c9"
  },
  {
    "stem": "AKUSTIK_GEN3_ARI",
    "name": "ACOUSTIC GEN4",
    "mic": {
      "1": 1.4,
      "4": -1.7,
      "6": 1.8,
      "7": -1.2,
      "8": -1.1
    },
    "music": {
      "2": 0.8,
      "3": -2,
      "4": 0.9,
      "5": -1.5,
      "6": 0.6
    },
    "main": {
      "3": 0.7,
      "4": -0.9
    },
    "fx": {
      "reverbLevel": 89,
      "echoLevel": 73,
      "decayMs": 2050,
      "predelayMs": 66,
      "repeat": 8
    },
    "donorSha256": "de3f92cf1198f42b0d87bcaa554cadd6f668194429a0f04829bfbf4bd5705dc3"
  },
  {
    "stem": "IMAM_QORI_GEN3",
    "name": "QORI GEN4",
    "mic": {
      "1": 1.4,
      "4": -1.7,
      "6": 2.5,
      "7": -1.8,
      "8": -1.4
    },
    "music": {
      "4": -1,
      "5": -2.7
    },
    "main": {
      "3": 0.8,
      "4": -1
    },
    "fx": {
      "reverbLevel": 96,
      "echoLevel": 71,
      "decayMs": 3450,
      "predelayMs": 84,
      "echoMs": 290,
      "repeat": 8
    },
    "donorSha256": "3bbe02e77413f6b9bda1ba2b704d7a7beefcc23d977a47464109754760480ca3"
  },
  {
    "stem": "JAZZ_GEN3_ARI",
    "name": "JAZZ GEN4",
    "mic": {
      "1": 1,
      "4": -1.6,
      "6": 1.6,
      "7": -1.3,
      "8": -1
    },
    "music": {
      "2": 1.3,
      "4": 0.8,
      "5": -1.8,
      "6": 0.8
    },
    "main": {
      "3": 0.7,
      "4": -0.7
    },
    "fx": {
      "reverbLevel": 91,
      "echoLevel": 72,
      "decayMs": 2540,
      "predelayMs": 75,
      "repeat": 7
    },
    "donorSha256": "139e378f8d68af32e39d8fb717c7164e4af489926754d45cab6dc93cb162bf03"
  },
  {
    "stem": "ROCK_GEN3_ARI",
    "name": "ROCK GEN4",
    "mic": {
      "1": 1.4,
      "4": -2,
      "6": 3.1,
      "7": -2.1,
      "8": -1.8
    },
    "music": {
      "4": -1.2,
      "5": -3.5,
      "6": -1.3
    },
    "main": {
      "3": 1.3,
      "4": -1.1
    },
    "fx": {
      "reverbLevel": 87,
      "echoLevel": 82,
      "decayMs": 1830,
      "predelayMs": 66,
      "echoMs": 290,
      "repeat": 7
    },
    "donorSha256": "1f841e113ff357b909f4fdff0f18bfd0bc72f3b2879edfc903c11b6214f79d9e"
  },
  {
    "stem": "MC_CERAMAH_GEN3",
    "name": "DUET GEN4",
    "mic": {
      "1": 1.2,
      "4": -2.2,
      "6": 2.2,
      "7": -1.3,
      "8": -1.1
    },
    "music": {
      "4": -0.9,
      "5": -2.8,
      "6": -0.7
    },
    "main": {
      "3": 0.8,
      "4": -1
    },
    "fx": {
      "reverbLevel": 88,
      "echoLevel": 74,
      "decayMs": 2330,
      "predelayMs": 62,
      "echoMs": 280,
      "repeat": 8
    },
    "donorSha256": "f0d39c9bf1c168c973c7d60c53ba86713bfc20344f31d1ff419ea87e5ee12520"
  },
  {
    "stem": "ADZAN_MEKAH_GEN3",
    "name": "GRAND HALL GEN4",
    "mic": {
      "1": 1.3,
      "4": -1.8,
      "6": 2.4,
      "7": -1.6,
      "8": -1.2
    },
    "music": {
      "4": -0.8,
      "5": -2.4
    },
    "main": {
      "3": 0.6,
      "4": -1
    },
    "fx": {
      "reverbLevel": 95,
      "echoLevel": 76,
      "decayMs": 3590,
      "predelayMs": 91,
      "echoMs": 390,
      "repeat": 10
    },
    "donorSha256": "9519acc2b77a14bdaef4098299f1f8594294e013171aaafcfc5daed03bebbac1"
  }
]
''')

# Proven file scalar offsets from K500 preset codec and current tool baseline.
FX_OFFSETS = {
    'reverbLevel':(0x74,1), 'echoLevel':(0x7B,1), 'repeat':(0x7C,1),
    'decayMs':(0xC8,2), 'predelayMs':(0xCA,2), 'echoMs':(0xCC,2),
    'mainMusic':(0x2A,1), 'surroundMusic':(0x3E,1), 'centerMusic':(0x52,1),
}
MAX_BAND_GAIN_CHANGE_DB = 3.5

def generate(source: Path, output: Path) -> list[dict]:
    donors = sorted(
        path.name for path in source.glob('[0-9][0-9]_*.k500')
        if 1 <= int(path.name[:2]) <= 10
    )
    expected = [f'{i:02d}_{p["stem"]}.k500' for i,p in enumerate(PROFILES,1)]
    if donors != expected:
        raise ValueError(f'GEN4 requires the exact 10 original GEN3 BT donors: {donors}')
    output.mkdir(parents=True, exist_ok=True)
    report = []
    for i,p in enumerate(PROFILES,1):
        donor = expected[i-1]
        original = (source / donor).read_bytes()
        digest = hashlib.sha256(original).hexdigest()
        if digest != p['donorSha256']:
            raise ValueError(f'GEN3 donor hash drift: {donor} {digest}')
        if len(original) != FILE_SIZE or original[0x0E] != 2:
            raise ValueError(f'Invalid BT donor: {donor}')
        if validate_device_slot_compatibility(original):
            raise ValueError(f'Native donor validation failure: {donor}')
        data = bytearray(original)
        permitted = {CHECKSUM_OFFSET}
        eq_edits, scalar_edits = [], []
        for section, delta_map in (
            ('micA',p['mic']), ('micB',p['mic']),
            ('music',p['music']), ('main',p['main'])
        ):
            base,count = EQ_SECTIONS[section]
            for key,delta in sorted(delta_map.items(), key=lambda item: int(item[0])):
                band=int(key)
                if not 1<=band<=count or not 0<abs(delta)<=MAX_BAND_GAIN_CHANGE_DB:
                    raise ValueError(f'Invalid EQ delta: {section} band {band}')
                offset=base+2+8*(band-1)+6
                previous=struct.unpack_from('<h',data,offset)[0]
                desired=previous+round(delta*10)
                if not -240<=desired<=240:
                    raise ValueError(f'Native PEQ gain out of range: {section} {band} {desired}')
                struct.pack_into('<h',data,offset,desired)
                permitted.update((offset,offset+1))
                eq_edits.append({'section':section,'band':band,
                                 'beforeDb':previous/10,'afterDb':desired/10,
                                 'deltaDb':(desired-previous)/10})
        for scalar,target in p['fx'].items():
            if scalar=='mainOutTrimDb':
                # Preserve left/right symmetry. This reduces excess loudness
                # on the previous Podcast donor turned into Ballad.
                for off in (0x24,0x26):
                    prev=data[off]
                    new=prev+round(target*2)
                    if not 0<=new<=100:raise ValueError('main output out of range')
                    data[off]=new
                    permitted.add(off)
                    scalar_edits.append({'key':'mainOutput','offset':f'0x{off:04X}',
                                         'beforeRaw':prev,'afterRaw':new})
                continue
            off,width = FX_OFFSETS[scalar]
            prev=int.from_bytes(data[off:off+width],'little')
            if width == 1:
                if not 0<=target<=100:raise ValueError(f'Scalar out of domain {scalar}')
                data[off]=target
            else:
                if scalar=='decayMs' and not 200<=target<=6000:
                    raise ValueError('reverb decay not in qualified conservative range')
                if scalar=='predelayMs' and not 0<=target<=200:
                    raise ValueError('pre-delay out of conservative range')
                if scalar=='echoMs' and not 0<=target<=1000:
                    raise ValueError('echo delay out of conservative range')
                data[off:off+width]=struct.pack('<H',target)
            permitted.update(range(off,off+width))
            scalar_edits.append({'key':scalar,'offset':f'0x{off:04X}',
                                 'beforeRaw':prev,'afterRaw':target})
        hw=p['name'].encode('ascii')
        if len(hw)>16:raise ValueError('K500 hardware name exceeds 16 bytes')
        data[NAME_OFFSET:NAME_OFFSET+16]=hw.ljust(16,b'\x00')
        permitted.update(range(NAME_OFFSET,NAME_OFFSET+16))
        data[CHECKSUM_OFFSET]=0
        data[CHECKSUM_OFFSET]=(-sum(data)) & 0xFF
        result=bytes(data)
        changed={j for j,(a,b) in enumerate(zip(original,result)) if a!=b}
        if changed-permitted:
            raise ValueError(f'Unexpected bytes changed: {donor} {sorted(changed-permitted)}')
        if len(result)!=FILE_SIZE or sum(result)%256:
            raise ValueError(f'Invalid file size or checksum: {donor}')
        compatibility=validate_device_slot_compatibility(result)
        if compatibility:raise ValueError(f'Native candidate incompatible: {compatibility}')
        if len(build_device_slot_image(result))!=656:
            raise ValueError(f'Native slot projection failed: {donor}')
        for section in ('mainAlt','surround','surroundAlt','center',
                        'centerAlt','sub','subAlt','reverb','echo'):
            base,count=EQ_SECTIONS[section]
            if result[base:base+2+8*count]!=original[base:base+2+8*count]:
                raise ValueError(f'Non-authorized section changed: {section}')
        filename=f'{i+30:02d}_{p["name"].replace(" ","_")}.k500'
        (output / filename).write_bytes(result)
        report.append({
            'number':i+30, 'name':p['name'],'file':filename, 'donor':donor,
            'donorSha256':digest, 'sha256':hashlib.sha256(result).hexdigest(),
            'changedOffsets':[f'0x{j:04X}' for j in sorted(changed)],
            'changedBytes':len(changed), 'eqEdits':eq_edits, 'scalarEdits':scalar_edits
        })
    (output/'gen4-manifest.json').write_text(
        json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    print('GEN4_AUDITION_PRESET_BANK_V2:',
          len(generate(args.source,args.output)),'byte-audited candidates')

if __name__=='__main__':
    main()
