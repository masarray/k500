# K500 v1.0.3-rc.2 — EQ / Music Max device-validation checkpoint

Baseline: `main@c38b2e132663c397a4c4621da032e374927b0223` (PR #83), plus this targeted RC2 closure.

This release candidate is **hardware-validation pending**, not a stable acceptance claim. Its purpose is to establish an exact reproducible build before reverse engineering the remaining K500 functions.

## Frozen capture-backed contracts

### EQ enable image

The three-byte image at `activeMemory[0x027D..0x027F]` is an **enable** image: section mask SET means EQ active, CLEAR means bypass. The Music reconnect pair is:

- Native Music EQ Bypass unchecked: `FD FF 03` (Music EQ active).
- Native Music EQ Bypass checked: `7D FF 03` (Music EQ bypassed).

The application decodes and writes this polarity in the same direction. Writes use read-modify-write on the shared image and preserve unrelated bits.

### System Music Max

Native `CMD 0x02` includes Top Music, Music Init and Music Max. The range is 0..84.

```text
TopMusic := min(TopMusic, MusicMax)
```

Lowering Music Max below the existing top volume clamps both fields in one command. Increasing Music Max does not increase the top volume. The Master Music visual ruler remains 0..84; its interaction ceiling, not the scale, changes.

## Additional RC2 protection

- A Master intent above Music Max is normalized **before** it enters canonical DesiredState.
- Lowering Max supersedes an older queued/in-flight Top Music desired revision and stages the clamped master value. A subsequent authoritative readback confirms both values independently.
- The System Music Max fader updates after new device state and cannot be edited before hardware connect/readback.
- The hardware-free engine self-test now exercises Top 70 -> Max 20, a direct out-of-range Top 90 attempt, and authoritative readback of Top 20 / Max 20.

## Physical validation checklist

Use only the exact RC2 installer/portable and record its manifest commit and SHA-256. In native K500 and SonKuPik, compare the following **after CONNECT and full readback**:

1. With all PEQs active, there must be no section-wide false `EQ BYPASS` overlay. Toggle Music bypass ON then OFF; native and SonKuPik must agree after reconnect.
2. Set Master Music=25, then Music Max=60, 25, 24, 20 and 0. Expected Master: 25, 25, 24, 20 and 0, respectively.
3. Raise Music Max back to 84; Master must remain 0. The fader ruler remains 0..84 throughout.
4. After reconnect, Music Max, Music Master, and all section bypass states must hydrate from the device again.
5. Ordinary live edits must not force a full 939-byte Retrieve All or a fake offline/online cycle on every edit.

Record deviations with an exact build SHA, initial device state, ordered actions, final native state and a short USB capture. **CI pass does not substitute for physical hardware acceptance.**

After physical acceptance, continue the pending `docs/K500_CAPTURE_TODO_MAP.md` items without changing this frozen checkpoint retroactively.
