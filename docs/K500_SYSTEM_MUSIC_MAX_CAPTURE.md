# K500 System Music Max Capture Map

Status: physically captured on 2026-09-20 with the manufacturer K500 application over USB HID.

## Evidence files

| Capture | Size | SHA-256 |
| --- | ---: | --- |
| `System_MusicMax_0_84.pcapng` | 6504 bytes | `dc4187dc7aaec3d511b21e128d72f5ea7c61a9d95b463a686e98008626f21708` |
| `System_MusicMax_84_60_40_20_0.pcapng` | 21384 bytes | `62fffa0471dfb9c4db5ff629421a077d2b9971896dad7c253c9cb9d9e4a2bf4b` |

## Native write location

System **Music Max** is part of the existing Top Music `CMD 0x02` full block.

Relevant bytes:

```text
AA 0D 00 02 TOP_MUSIC MUSIC_INIT MUSIC_MAX ...
             ^         ^          ^
```

The captured native range is direct integer `0..84`.

Examples from the physical capture:

```text
Top Music 25, Music Max 60
AA 0D 00 02 19 19 3C 02 09 09 09 08 08 07 15 02 38

Top Music 20, Music Max 20
AA 0D 00 02 14 19 14 02 09 09 09 08 08 07 15 02 65

Top Music 0, Music Max 0
AA 0D 00 02 00 19 00 02 09 09 09 08 08 07 15 02 8D
```

## Native ceiling behavior

Music Max is not merely a stored setting. It is a **hard upper limit** for
Top Music / Master Music volume.

Observed while lowering Music Max from 84 with Top Music initially at 25:

```text
Max 26 -> Top Music 25, Max 26
Max 25 -> Top Music 25, Max 25
Max 24 -> Top Music 24, Max 24
Max 23 -> Top Music 23, Max 23
...
Max 20 -> Top Music 20, Max 20
...
Max  0 -> Top Music  0, Max  0
```

Therefore the native rule is:

```text
TopMusic = min(TopMusic, MusicMax)
```

When Music Max is raised again, the master is **not** raised automatically.
The second capture direction proves this explicitly: starting at Top Music 0
and raising Max toward 84 leaves Top Music at 0.

## READ mapping

Existing full readback already maps:

```text
file scalar 0x0008 -> Top Music Vol
file scalar 0x000C -> Music Max Vol
```

Both use direct integer range `0..84`.

## Application contract

1. Expose Music Max as a live device-backed value in System / Startup Limits.
2. Editing Music Max sends one Top Music `CMD 0x02` block.
3. If the new Max is below current Top Music, clamp Top Music in the same block.
4. Raising Max must never raise Top Music automatically.
5. Master Music keeps its visual 0..84 ruler.
6. Interaction must stop at Music Max without rescaling the fader. For example,
   Max=25 means the cap cannot move above the physical 25/84 position.
7. Direct edits of Master Music must also be clamped to current Music Max.
