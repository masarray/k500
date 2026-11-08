#!/usr/bin/env python3
"""Reproduce native Artist R4 A/B candidates from the exact K500 FOCUS A donor."""
import hashlib, struct
from pathlib import Path
HERE=Path(__file__).resolve().parent
src=(HERE.parent/'source'/'KTVPRO_FOCUS_A.k500').read_bytes()
assert len(src)==1144 and sum(src)%256==0
assert hashlib.sha256(src).hexdigest()=='e48e45d5cdbd66aa6b80609d1ad48987fff48eddb5673e2fc893e53e463ddfdb'
CANDIDATES={
  '01_ARTIST_R4_BT.k500':('F1 ARTIST R4',2500,20,-120,127,10,'e48948cb45bb016bcd41570221c6a1ad878a3a25a2e45782faba943b61a75b80'),
  '01_ARTIST_SOFT_R4_BT.k500':('F1 ARTIST SOFT',2550,21,-120,126,10,'2c2ea9d2f8622b2b425f7e79cb10cf5e307a898266718a361f16ed8f835cb359'),
  '01_ARTIST_R4_E9_BT.k500':('F1 ARTIST E9',2500,20,-120,127,9,'c847ddd316c699ec1c252e73f5b99ddbfa938667ed091cba859d320122cdbfd4')
}
for filename,(name,freq,q_raw,gain_raw,shelf,repeat,wanted) in CANDIDATES.items():
  d=bytearray(src);whitelist={0x000E,0x007C,0x0475}|set(range(0x454,0x464))
  d[0x0E]=2;d[0x7C]=repeat
  off=0x3D8+2+8
  struct.pack_into('<H',d,off+2,freq)
  struct.pack_into('<H',d,off+4,q_raw)
  struct.pack_into('<h',d,off+6,gain_raw)
  whitelist.update(range(off+2,off+8))
  shelf_off=0x3D8+2+4*8+6
  struct.pack_into('<h',d,shelf_off,shelf)
  whitelist.update((shelf_off,shelf_off+1))
  n=name.encode('ascii');assert len(n)<=16
  d[0x454:0x464]=n+b'\0'*(16-len(n))
  d[0x475]=0;d[0x475]=(-sum(d))&255
  b=bytes(d);assert len(b)==1144 and sum(b)%256==0 and b[0x0E]==2
  assert all(i in whitelist for i,(a,z) in enumerate(zip(src,b)) if a!=z)
  assert hashlib.sha256(b).hexdigest()==wanted,filename
  path=HERE/filename
  if path.exists():assert path.read_bytes()==b,filename
  else:path.write_bytes(b)
  print('PASS',filename)
