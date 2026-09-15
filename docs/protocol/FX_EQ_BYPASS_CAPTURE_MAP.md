# Reverb / Echo EQ Bypass — Native Capture Map

Marker: `FX_EQ_BYPASS_CAPTURED_V1`

Native KTV uses a shared runtime register command, not five PEQ gain writes:

```text
USB: AA 06 00 0F 40 FD FF MASK 02 CHECKSUM
BT : AA 06    0F 40 FD FF MASK 02 CHECKSUM
```

`MASK` is a bitfield proven by the supplied HHD captures:

- bit 0 (`0x01`) = Reverb EQ Bypass
- bit 1 (`0x02`) = Echo EQ Bypass
- `0x00` = Reverb OFF, Echo OFF
- `0x01` = Reverb ON, Echo OFF
- `0x03` = Reverb ON, Echo ON

Reverb capture alternates `0x00 <-> 0x01`. Echo capture alternates `0x01 <-> 0x03` while Reverb stays ON, proving both toggles share one mask byte and each write must preserve the other bit.

Observed response/ack frame after each write:

```text
55 0E 00 F0 00 05 08 0D 66 66 66 66 66 66 66 00 00 1E
```

No native readback form for runtime register `0xFFFD` is proven by these captures. Until such a capture exists, a fresh controller session starts from UI default mask `0x00`; do not invent a read command.
