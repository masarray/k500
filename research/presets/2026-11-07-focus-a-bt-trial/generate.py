#!/usr/bin/env python3
"""Bit-perfect reproduction of FOCUS A 10 native .k500 BT research presets."""
import argparse, hashlib, json, struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = json.loads((HERE / "patches.json").read_text(encoding="utf-8"))
donor = (HERE / spec["donor"]["path"]).read_bytes()
assert len(donor) == 1144 and sum(donor) % 256 == 0
assert hashlib.sha256(donor).hexdigest() == spec["donor"]["sha256"]
assert donor[0x0E] == 5
S = {
 "reverb.level":0x74,"reverb.decay_ms":0xC8,"reverb.predelay_ms":0xCA,
 "echo.level":0x7B,"echo.repeat":0x7C,"echo.delay_ms":0xCC,
 "main.mic":0x28,"main.music":0x2A,"main.reverb":0x2C,"main.echo":0x2E,
 "center.mic":0x50,"center.music":0x52,"center.reverb":0x54,"center.echo":0x56,
 "surround.mic":0x3C,"surround.music":0x3E,"surround.reverb":0x40,"surround.echo":0x42,
 "mic.comp.ratio":0x18,"mic.comp.attack_ms":0x19,"mic.comp.release_s":0x1A,
 "music.bluetooth_gain_db":0x20
}
EQ = {"micA":(0xF0,10),"micB":(0x150,10),"music":(0x1B0,7)}
p = argparse.ArgumentParser()
p.add_argument("--check", action="store_true")
opt = p.parse_args()
for item in spec["presets"]:
    b = bytearray(donor)
    whitelist = {0x0E, 0x475} | set(range(0x454,0x464))
    b[0x0E] = 2
    name = item["name"].encode("ascii")
    assert len(name) <= 16
    b[0x454:0x464] = name + b"\x00" * (16-len(name))
    for key, value in item["scalars"].items():
        assert key in S, key
        offset = S[key]
        raw = round(value*10) if key=="mic.comp.release_s" else value+12 if key=="music.bluetooth_gain_db" else value
        width = 2 if key in ("reverb.decay_ms","reverb.predelay_ms","echo.delay_ms") else 1
        assert int(raw) == raw and 0 <= raw < (65536 if width==2 else 256)
        if key=="reverb.decay_ms": assert 500 <= raw <= 5000
        if key=="reverb.predelay_ms": assert 0 <= raw <= 100
        if key=="echo.delay_ms": assert 0 <= raw <= 1000
        if key=="echo.repeat": assert 0 <= raw <= 10
        if key in ("echo.level","reverb.level"): assert 0 <= raw <= 100
        if width == 2: struct.pack_into("<H", b, offset, int(raw))
        else: b[offset] = int(raw)
        whitelist.update(range(offset,offset+width))
    for section, gain_bands in item["eqGainDb"].items():
        assert section in EQ
        start, size = EQ[section]
        for band_index, db in gain_bands.items():
            band = int(band_index)
            assert 1 <= band <= size
            off = start+2+(band-1)*8+6
            raw = round(db*10)
            assert -240 <= raw <= 240
            struct.pack_into("<h", b, off, raw)
            whitelist.update((off,off+1))
    b[0x475] = 0
    b[0x475] = (-sum(b)) & 255
    output = bytes(b)
    assert len(output)==1144 and sum(output)%256==0 and output[0x0E]==2
    actual_changes = {i for i,(a,z) in enumerate(zip(donor,output)) if a!=z}
    assert actual_changes <= whitelist, item["file"]
    assert hashlib.sha256(output).hexdigest()==item["sha256"], item["file"]
    target = HERE / item["file"]
    if opt.check: assert target.read_bytes()==output, item["file"]
    else: target.write_bytes(output)
    print(("verified" if opt.check else "generated"), item["file"])
