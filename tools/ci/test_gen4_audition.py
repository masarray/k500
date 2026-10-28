#!/usr/bin/env python3
"""GEN4 V2 strict regression. Golden byte hashes are derived from the supplied GEN3 ZIP."""
from __future__ import annotations
import hashlib
import json
import shutil
import sys
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from generate_gen4_presets import PROFILES, FX_OFFSETS, generate
from k500_preset_lab import CHECKSUM_OFFSET, EQ_SECTIONS, NAME_OFFSET, build_device_slot_image, validate_device_slot_compatibility

GOLDEN=(
    'c6aeb02e1c3c46e91ab412ee5bc34e934134add4f273b77daf6f04132d9c26ce',
    'd32730e62b8e210b2f1af871e921399197ad800e4a0251efa01b404432e5ea31',
    '4052026439e48ad3735337d1961248b1f4665185b7974dff8b9849aa8c192ccb',
    'cdcc0ea1e8221fe748a77b7f29eb39061e2171383ef9185c9703c3d387f9314e',
    'e62f7e8882a6ac27a98a303c61e1192a0b3d44eb1f8bdb282e7fd94b6b856d7d',
    '50e0367fbb5a10971abcd13ac10f17c367f99296896eeedf2aaa271c39394eb0',
    '431e95c13a594f94292359156c606058c24c4b6a582e9c4ce2a35c3ef40b39e7',
    '9eef39a4402b00d54f9651081b366ff70cdf9345db83974b58d926deb91d54a2',
    '4d21d04d28db39609c617f44431cc9467e611bba3cf88fce2ad04f1e692deeed',
    '1ca5950803404bdeae09c34d2c70e2a75b7d8763557d393075d6877cb80d0620'
)
EXPECTED_NAMES=('ARTIST GEN4','BALLAD GEN4','DANGDUT GEN4','KARAOKE GEN4',
                'ACOUSTIC GEN4','QORI GEN4','JAZZ GEN4','ROCK GEN4',
                'DUET GEN4','GRAND HALL GEN4')

def main() -> None:
    src=ROOT/'resources/presets'
    assert len(PROFILES)==10
    assert tuple(p['name'] for p in PROFILES)==EXPECTED_NAMES
    cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
    bridge=(ROOT/'src/k500/K500PresetFileBridge.cpp').read_text(encoding='utf-8')
    assert 'GEN4_AUDITION_PRESET_BANK_V2' in cmake
    assert 'GEN4_AUDITION_CATALOG_V2' in bridge
    with tempfile.TemporaryDirectory(prefix='gen4-v2-') as tmp:
        out=Path(tmp)/'generated'
        report=generate(src,out)
        assert len(report)==10 and len(list(out.glob('*.k500')))==10
        for i,(p,r) in enumerate(zip(PROFILES,report)):
            original=(src/r['donor']).read_bytes()
            candidate=(out/r['file']).read_bytes()
            assert r['number']==i+31 and r['name']==EXPECTED_NAMES[i]
            assert r['file'] in bridge and r['file'] in cmake
            assert r['donorSha256']==hashlib.sha256(original).hexdigest()==p['donorSha256']
            assert r['sha256']==hashlib.sha256(candidate).hexdigest()==GOLDEN[i]
            assert len(candidate)==1144 and sum(candidate)%256==0
            assert candidate[0x0E]==original[0x0E]==2
            assert len(build_device_slot_image(candidate))==656
            assert not validate_device_slot_compatibility(candidate)
            assert candidate[NAME_OFFSET:NAME_OFFSET+16].split(bytes((0,)),1)[0].decode('ascii')==r['name']
            changes={j for j,(a,b) in enumerate(zip(original,candidate)) if a!=b}
            allowed={CHECKSUM_OFFSET,*range(NAME_OFFSET,NAME_OFFSET+16)}
            expected_eq=set()
            for section,delta_map in (('micA',p['mic']),('micB',p['mic']),
                                      ('music',p['music']),('main',p['main'])):
                for n,delta in delta_map.items():
                    assert 0<abs(delta)<=3.5
                    off=EQ_SECTIONS[section][0]+2+(int(n)-1)*8+6
                    allowed.update((off,off+1))
                    expected_eq.add((section,int(n)))
                    assert int.from_bytes(candidate[off:off+2],'little',signed=True) - int.from_bytes(original[off:off+2],'little',signed=True)==round(delta*10)
            assert {(e['section'],e['band']) for e in r['eqEdits']}==expected_eq
            assert max(abs(e['deltaDb']) for e in r['eqEdits'])>=1.5
            for key,value in p['fx'].items():
                if key=='mainOutTrimDb':
                    assert key=='mainOutTrimDb' and i==1
                    for off in (0x24,0x26):
                        allowed.add(off)
                        assert candidate[off]-original[off]==-4
                    continue
                off,width=FX_OFFSETS[key]
                allowed.update(range(off,off+width))
                assert int.from_bytes(candidate[off:off+width],'little')==value
            assert changes<=allowed,(r['file'],sorted(changes-allowed))
            assert changes=={int(v,16) for v in r['changedOffsets']}
            assert len(changes)==r['changedBytes']
            # paired Mic A/B exact target gain values (donor also linked in all ten).
            for n in range(10):
                offA=EQ_SECTIONS['micA'][0]+2+8*n+6
                offB=EQ_SECTIONS['micB'][0]+2+8*n+6
                assert candidate[offA:offA+2]==candidate[offB:offB+2]
            # No phantom DSP reconstruction of fields we did not qualify.
            for section in ('mainAlt','surround','surroundAlt','center','centerAlt',
                            'sub','subAlt','reverb','echo'):
                base,count=EQ_SECTIONS[section]
                assert original[base:base+2+8*count]==candidate[base:base+2+8*count]
        # deliberate donor tamper is rejected even if checksum is repaired
        tamper=Path(tmp)/'tampered';tamper.mkdir()
        for i,p in enumerate(PROFILES,1):
            donor=f'{i:02d}_{p["stem"]}.k500'
            shutil.copyfile(src/donor,tamper/donor)
        target=tamper/f'01_{PROFILES[0]["stem"]}.k500'
        b=bytearray(target.read_bytes());b[0x2A]=(b[0x2A]+1)%256
        b[CHECKSUM_OFFSET]=0;b[CHECKSUM_OFFSET]=(-sum(b))&255
        target.write_bytes(b)
        try: generate(tamper,Path(tmp)/'should-fail')
        except ValueError as exc: assert 'hash drift' in str(exc),str(exc)
        else: raise AssertionError('tampered donor was accepted')
    print('GEN4 V2 PASS: 10 exact SHA, strict offsets, native 656 byte projection, donor tamper rejection, 10 new vocal profiles')

if __name__=='__main__':main()
