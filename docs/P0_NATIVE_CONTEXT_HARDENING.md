# P0 Native Context Hardening

Captured-native invariants implemented in P0:

- USB startup Retrieve All uses `CMD 0x40` mode byte `0x02`, 58-byte blocks, 939-byte active memory.
- EQ bypass is one shared 24-bit image written by `CMD 0x0F`: `40 M0 M1 M2 02`.
- Retrieve All owns the initial bypass image at active-memory offsets `0x027D..0x027F`.
- Bypass mapping: Mic `M0:0x60`, Music `M0:0x80`, Main `M1:0x01`, Surround `M1:0x04`, Center `M1:0x10`, Sub `M1:0x40`, Reverb `M2:0x01`, Echo `M2:0x02`.
- Every bypass edit is read/modify/write against the hydrated device image; unrelated bits are preserved.
- Reverb remains full-image `CMD 0x0B`; Echo remains full-image `CMD 0x0D`; unknown bytes are preserved from device truth.
- Echo LEVEL/REPEAT/LEFT DELAY/DIRECT are capture-verified live controls.
