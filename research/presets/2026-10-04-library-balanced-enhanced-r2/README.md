# K500 library balanced-enhanced R2

Date: 2026-10-04

## Why this exists

Real-K500 listening has now converged on a tight music-balance family in the
accepted presets 01, 03 and 05. Their Music PEQ values cluster closely around:

```text
11704 Hz   +1.5 .. +1.6 dB
158 Hz     +2.9 .. +3.1 dB
66 Hz      +8.0 .. +8.2 dB
1315 Hz    -1.0 .. -0.8 dB
2916 Hz    +1.3 dB
6495 Hz    +1.0 .. +1.2 dB
HS 766 Hz  +5.7 .. +6.0 dB
```

The new product direction is therefore **not** to keep the remaining modes on
an older, less-enhanced music core. Music enhancement is a shared premium
benefit. Each preset keeps its own identity through Mic, FX, outputs, Sub and
small context-specific Music voicing.

Slots 01, 03 and 05 are LOCKED and are not changed by this pass.

## Nominal balanced-enhanced core

```text
[ +1.6, +3.0, +8.1, -0.9, +1.3, +1.1, +5.8 ] dB
```

The remaining modes stay close to that envelope:

| Slot | Context | R2 Music gains (bands 1..7) |
|---:|---|---|
| 02 | MC / radio | 1.6, 3.0, 8.1, -0.9, 1.3, 1.1, 5.8 |
| 04 | pop / rock | 1.6, 3.1, 8.2, -0.8, 1.3, 1.2, 5.9 |
| 06 | sholawat | 1.5, 2.9, 8.0, -1.0, 1.2, 1.0, 5.7 |
| 07 | jazz lounge | 1.5, 2.8, 8.0, -1.0, 1.2, 1.0, 5.7 |
| 08 | blues club | 1.4, 3.0, 8.1, -1.0, 1.1, 0.9, 5.6 |
| 09 | acoustic | 1.5, 2.8, 7.9, -1.0, 1.2, 1.0, 5.7 |
| 10 | reggae / dub | 1.4, 3.1, 8.3, -1.0, 1.1, 0.9, 5.5 |

## Surgical boundary

Only Music PEQ gain words are changed. Frequencies, Q, filter type aliases,
crossovers, Mic, Reverb, Echo, output routing/EQ, compressors, delays, Sub,
Alt sections and unknown/reserved bytes remain byte-for-byte donor state.

This means:

- 02 gets the same premium music evolution while its MC vocal path stays dry and controlled;
- 04 keeps rock impact through a tiny push toward the 03 end of the accepted envelope;
- 06/07/09 stay gentler and more natural without falling back to the old thin music balance;
- 08 remains warmer/darker;
- 10 remains deeper/groove-led.

Every candidate is 1144 bytes and has additive checksum modulo 256 = 0.
See `manifest.json` for exact SHA-256 and source/candidate blob identities.

## Promotion discipline

These exact R2 bytes are now promoted to the official bank for Slots 02, 04 and 06–10. Do not rebuild or normalize them after promotion. If one context needs correction, tune only that mode by a small Music-gain delta and keep all accepted regions locked.
