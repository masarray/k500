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

## Use Init Volume — setter proven, connect readback not yet proven

The OFF/ON capture repeats the following exact USB writes:

```text
OFF  AA 03 00 12 00 03 E8
ON   AA 03 00 12 01 03 E7
```

Both receive RSP 0xED acknowledgements. This proves command 0x12 and its boolean payload.

However this capture does not perform a fresh handshake or full readback once while OFF and once while ON. Its heartbeat response is unchanged across the toggle. Therefore this evidence does NOT identify the connect-time readback bit/offset.

Until an OFF-connect versus ON-connect capture proves that read-side mapping:

- never infer Use Init Volume from QSettings/PC preferences;
- never overwrite device state automatically on connect just to make the checkbox appear synchronized;
- mark the value unknown after connect;
- after SonKuPik sends CMD 0x12 and receives valid RSP 0xED, the new value is known for that live session.

## Capture required to close Use Init read-side mapping

One capture is sufficient if it contains both cases:

1. Set Use Init Volume OFF in native app, disconnect/reconnect while capture is running, let heartbeat + handshake + full 939-byte readback complete.
2. Set Use Init Volume ON, disconnect/reconnect again in the same capture, and let the same sequence complete.

Diff E3, C0 and the reconstructed 939-byte snapshots. Only a stable OFF/ON delta may become the connect-time decoder.

## Change control

Any future mapping from these runtime status fields must include exact captured vectors in a hardware-free self-test and a repo guard so the semantics cannot silently regress.