#!/usr/bin/env python3
"""Generate and verify three native K500 R5 Artist variants from trusted E9 parent."""
import hashlib,struct
from pathlib import Path
HERE=Path(__file__).resolve().parent
parent=HERE.parent/'refinement-r4'/'01_ARTIST_R4_E9_BT.k500'
e9=parent.read_bytes()
assert len(e9)==1144 and hashlib.sha256(e9).hexdigest()=='c847ddd316c699ec1c252e73f5b99ddbfa938667ed091cba859d320122cdbfd4'
assert sum(e9)%256==0 and e9[0x0E]==2 and e9[0x7C]==9
CANDIDATES={
'01_ARTIST_R5_SWEET_BT.k500':('F1 ARTIST R5',94,-128,114,'ec67e635d974fd6cd0f62d01ea4ad709909a77562c560840243ef2e52ffc6218'),
'01_ARTIST_R5_RETURN_BT.k500':('F1 E9 RETURN',92,-124,117,'729b531bd51da8cd62480c92423b121e434be47668399982f4313de155fc09ae'),
'01_ARTIST_R5_MID_BT.k500':('F1 E9 MID',95,-132,112,'0efd33fd78adafdc79b8a85ee5076282f790cac454a9ee5e67e3286d6027a1c1')
}
for filename,(name,ret,eq3,eq5,want) in CANDIDATES.items():
    b=bytearray(e9);b[0x2E]=ret
    struct.pack_into('<h',b,0x428,eq3)
    struct.pack_into('<h',b,0x438,eq5)
    n=name.encode('ascii');assert len(n)<=16
    b[0x454:0x464]=n+b'\0'*(16-len(n))
    b[0x475]=0;b[0x475]=(-sum(b))&255
    data=bytes(b)
    assert len(data)==1144 and sum(data)%256==0 and data[0x0E]==2 and data[0x7C]==9
    allowed={0x2E,0x428,0x429,0x438,0x439,0x475}|set(range(0x454,0x464))
    assert {i for i,(x,y) in enumerate(zip(e9,data)) if x!=y} <= allowed
    assert hashlib.sha256(data).hexdigest()==want,filename
    out=HERE/filename
    if out.exists():assert out.read_bytes()==data,filename
    else:out.write_bytes(data)
    print('validated',filename)
