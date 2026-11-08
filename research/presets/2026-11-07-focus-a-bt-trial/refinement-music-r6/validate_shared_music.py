#!/usr/bin/env python3
"""Check two real K500 Bluetooth preset files share Music EQ + Main Music."""
from pathlib import Path
import struct,hashlib
HERE=Path(__file__).resolve().parent
ROOT=HERE.parent
files={
'01':(ROOT/'01_FOCUS_ARTIST_BT.k500',HERE/'01_ARTIST_R4_ROLLBACK.k500',
'eb64073940b3907e15b0c77fc630baf8aef737fb3ab6980d5310d8b459604dd9',
'e48948cb45bb016bcd41570221c6a1ad878a3a25a2e45782faba943b61a75b80'),
'02':(ROOT/'02_FOCUS_MC_BT.k500',HERE/'02_MC_ORIGINAL_ROLLBACK.k500',
'4c8937765af3e2c7bc441b25daf4674cfdc87fbbb94240809cfbb8e00dfc9bfe',
'50c2dc10e188f213d7d8c2b3c662c0af9c42596383e70eec78c3f833407b6790')
}
gainbytes=set()
for band in range(7):
  off=0x1B0+2+band*8+6
  gainbytes.update((off,off+1))
allowed={0x2A,0x475}|set(range(0x454,0x464))|gainbytes
loaded={}
for slot,(candidate,parent,newsha,oldsha) in files.items():
  c=candidate.read_bytes();old=parent.read_bytes()
  assert len(c)==len(old)==1144
  assert sum(c)%256==sum(old)%256==0
  assert c[0x0E]==old[0x0E]==2
  assert hashlib.sha256(c).hexdigest()==newsha
  assert hashlib.sha256(old).hexdigest()==oldsha
  delta={i for i,(a,b) in enumerate(zip(c,old)) if a!=b}
  assert delta<=allowed, (slot,delta-allowed)
  assert c[0x20]==old[0x20]==9
  assert c[0x3E]==old[0x3E] and c[0x52]==old[0x52]
  assert c[0x2A]==73
  assert c[0x1B0:0x1B2]==old[0x1B0:0x1B2]
  for band in range(7):
    off=0x1B0+2+band*8
    assert c[off:off+6]==old[off:off+6]
    typ,freq,q,gain=struct.unpack_from('<HHHh',c,off)
    assert typ in (0,1,2,3,0x100,0x200)
    assert 20<=freq<=20000 and 1<=q<=250 and -240<=gain<=240
  assert c[0x1EA:0x1F8]==old[0x1EA:0x1F8]
  loaded[slot]=c
  print('PASS',slot,'diff bytes',len(delta))
assert loaded['01'][0x1B0:0x1F8]==loaded['02'][0x1B0:0x1F8]
assert loaded['01'][0x1F8:0x240]==loaded['02'][0x1F8:0x240]
assert loaded['01'][0x2A]==loaded['02'][0x2A]==73
print('PASS shared 72-byte Music EQ and Main EQ; Main Music 73; vocal FX preserved')
