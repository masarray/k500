# K500 Captured Runtime Status Contract

Physical-device evidence captured 2026-09-18 with the manufacturer Professional Audio System application and K500 USB HID.

## Source capture identities

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| Use_Init_Vol_ON_OFF.dmslog8 | 85670 | e0c1ddfd061e8120cdad0a05de2df481f91d5add6d98e7ed3451661446597969 |
| Connect_During_Music_Stoped.dmslog8 | 92830 | f52c79dbddcd43e0d166e3b9a90e5cadbbe42a13cb1364dc816d8f0bf096d0f1 |
| Connect_During_Music_Played.dmslog8 | 92022 | 72d8ec80bc88a48bec53651ff07a45cf5ad320ac1af76f0974211d1c30707dba |

## Playback status — captured and mapped

The STOPPED and PLAYING connect captures have byte-identical 939-byte active memory. Playback state is therefore runtime status, not preset/active-memory state.

Heartbeat response (RSP 0xE3):

```text
STOPPED  55 0E 00 E3 00 05 08 0D 66 66 66 66 66 66 66 00 00 2B
PLAYING  55 0E 00 E3 00 05 0C 0D 66 66 66 66 66 66 66 00 00 27
                              ^^
```

Handshake response (RSP 0xC0) repeats the same runtime-status byte:

```text
STOPPED ... 00 00 00 08 0D AB 03 CE 00 00 ...
PLAYING ... 00 00 00 0C 0D AB 03 CE 00 00 ...
                    ^^
```

Contract: bit 0x04 clear = not playing (stopped/paused UI state); bit 0x04 set = actively playing. SonKuPik must derive the Play/Pause icon from device RX, never from a local optimistic boolean.

## Mute — setter and connect read-side captured

Native write frames remain:

```text
OFF  AA 03 15 00 00 E8
ON   AA 03 15 01 00 E7
```

Paired Wireshark/USBPcap connect/change/disconnect captures:

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| Connect_MUTE_OFF.pcapng | 11932376 | 53db157ba1d0e4fddd31e0bf2c2c06eefd03b353e71d39af503bdd171fbbd36f |
| Connect_MUTE_ON.pcapng | 25326260 | 2379a324c83abf4bc68a8f9ea8d5ce7c36c91c8a0453527812cc2dc8978825a0 |

The filenames describe the value changed after CONNECT, so the C0 handshake carries
the pre-change state. The stable delta is again C0 data[7]:

```text
Device UNMUTED before changing Mute ON: ... 5A 84 ...
Device MUTED   before changing Mute OFF: ... 5A 86 ...
```

Contract:

- C0 data[7] bit 0x02 clear => Mute OFF / unmuted
- C0 data[7] bit 0x02 set   => Mute ON / muted

This shares the same C0 flags byte as Use Init Volume (bit 0x04) but is a separate bit.

## Use Init Volume — setter and connect read-side captured

The original toggle capture proves the exact USB setter frames:

```text
OFF  AA 03 00 12 00 03 E8
ON   AA 03 00 12 01 03 E7
```

Both receive RSP 0xED acknowledgements.

Two follow-up connect/change/disconnect captures close the read-side mapping:

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| Connect_UseInitVol_ON_Disconnect.dmslog8 | 93978 | 843f9a44e678560481c731a33eb41ff148c3b5c60713be52603cbc14d68e66bb |
| Connect_UseInitVol_OFF_Disconnect.dmslog8 | 93170 | f649c4b457332e7385883e0e85381c6059be24b398e00ae92595896b2695bdd7 |

Both reconstruct to byte-identical 939-byte active memory. The stable delta is in the
connect-time handshake response RSP 0xC0 at data byte index 7:

```text
Device OFF before changing it ON:
... 05 00 5A 80 01 00 F5 01 ...
            ^^

Device ON before changing it OFF:
... 05 00 5A 84 01 00 F5 01 ...
            ^^
```

The filenames describe the value changed during that session, so the handshake occurs
before the change. Therefore the authoritative contract is:

- C0 data[7] bit 0x04 clear => Use Init Volume OFF
- C0 data[7] bit 0x04 set   => Use Init Volume ON

SonKuPik must hydrate the checkbox from this C0 bit on initial connect and Recall
handshakes. QSettings/PC preferences must never override it. After a local CMD 0x12
change, a valid RSP 0xED also establishes the current-session value immediately.

## Change control

Any future mapping from these runtime status fields must include exact captured vectors in a hardware-free self-test and a repo guard so the semantics cannot silently regress.