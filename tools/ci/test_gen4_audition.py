#!/usr/bin/env python3
"""Fast independent GEN4 native safety and golden-identity regression."""
from __future__ import annotations
import hashlib
import json
import shutil
import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from generate_gen4_presets import PROFILES, generate
from k500_preset_lab import CHECKSUM_OFFSET, EQ_SECTIONS, NAME_OFFSET, build_device_slot_image, validate_device_slot_compatibility

GOLDEN = (
 '44ab7175ec3e35ff1946b54bb8d87e9ca7510b374d03af49a6b4f9086615117a',
 'f72bc68f9e243e85475914ff0909d693041e27ce1b57fb57ecfa10100639cf41',
 '0dff33856bacdfc650b7c6126d8e1880fdd878228495f48006ed1db35ceffd09',
 '16bf8364c75beb03eabd9dde4c1d32066d92bc19d1391ae254873953530cf266',
 '75cbdee592864ef03da08727ef04260a2c5a90298a57d1e387d9a7aa6b834bbb',
 '89ca6d052468fc0d8ad51d086cf748ecfe9c945f960cadbd71211bd5d89ed6c8',
 '009e53810456c9a0954233ea914abecb4c5569c8372c92b145e2822cb9dcb72b',
 '7bdcd6058a86dbad5d49a866b3f04e3645f88e52bb56a645110b2949115305fd',
 '23d4e3224126919c94f0cecb76093a88e17c4bc9220d39968a89197d82e13cb3',
 '210cef6691f73e0ef41351e430e2d5516b18d7585e33eeb59bf8770f041749ab',
)

def main() -> None:
    source = ROOT/'resources/presets'
    bridge = (ROOT/'src/k500/K500PresetFileBridge.cpp').read_text(encoding='utf-8')
    cmake = (ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
    assert 'GEN4_AUDITION_CATALOG_V1' in bridge
    assert 'GEN4_AUDITION_PRESET_BANK_V1' in cmake
    assert len(PROFILES) == 10
    with tempfile.TemporaryDirectory(prefix='gen4-audition-') as tmp:
        out = Path(tmp)/'out'
        manifest = generate(source,out)
        assert len(manifest)==10
        assert len(list(out.glob('*.k500')))==10
        for i, item in enumerate(manifest):
            original=(source/item['donor']).read_bytes()
            candidate=(out/item['file']).read_bytes()
            assert item['number']==31+i
            assert item['sha256']==GOLDEN[i]==hashlib.sha256(candidate).hexdigest()
            assert hashlib.sha256(original).hexdigest()==item['donorSha256']==PROFILES[i]['donorSha256']
            assert len(candidate)==1144 and sum(candidate)%256==0
            assert candidate[0x0E]==original[0x0E]==2
            assert not validate_device_slot_compatibility(candidate)
            assert len(build_device_slot_image(candidate))==656
            assert candidate[NAME_OFFSET:NAME_OFFSET+16].split(b'\0',1)[0].decode('ascii')==item['name']
            changed={j for j,(x,y) in enumerate(zip(original,candidate)) if x!=y}
            allowed={CHECKSUM_OFFSET,0xCA,0xCB}
            allowed.update(range(NAME_OFFSET,NAME_OFFSET+16))
            for edit in item['edits']:
                base,count=EQ_SECTIONS[edit['section']]
                band=edit['band']
                assert 1<=band<=count and abs(edit['toDb']-edit['fromDb'])<=0.8001
                offset=base+2+(band-1)*8+6
                allowed.update({offset,offset+1})
            assert changed<=allowed,(item['file'],sorted(changed-allowed))
            assert set(item['changedOffsets'])=={f'0x{j:04X}' for j in changed}
            assert all(candidate[EQ_SECTIONS['micA'][0]+2+8*j+6:EQ_SECTIONS['micA'][0]+2+8*j+8]
                       ==candidate[EQ_SECTIONS['micB'][0]+2+8*j+6:EQ_SECTIONS['micB'][0]+2+8*j+8]
                       for j in range(10))
            assert item['file'] in bridge and item['file'] in cmake
            assert item['predelayAfterMs']-item['predelayBeforeMs']<=12
        # Donor mutation must fail closed even if the native additive checksum is corrected.
        tamper=Path(tmp)/'tampered'
        tamper.mkdir()
        for i,p in enumerate(PROFILES,1):
            name=f'{i:02d}_{p["stem"]}.k500'
            shutil.copyfile(source/name,tamper/name)
        probe=tamper/f'01_{PROFILES[0]["stem"]}.k500'
        data=bytearray(probe.read_bytes());data[0x2A]=(data[0x2A]+1)%256
        data[CHECKSUM_OFFSET]=0;data[CHECKSUM_OFFSET]=(-sum(data))&255
        probe.write_bytes(data)
        try:
            generate(tamper,Path(tmp)/'bad')
        except ValueError as exc:
            assert 'donor drift' in str(exc),str(exc)
        else:
            raise AssertionError('tampered donor was not rejected')
    print('GEN4 candidate bank PASS: 10 golden hashes, source immutability, native projection, strict diffs, tamper rejection')

if __name__ == '__main__':
    main()
