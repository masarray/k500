# Balanced Enhanced Music Core — R2 candidate bank

Date: 2026-10-04

## Goal

The user explicitly prefers the Music result of official Modes **01, 03 and 05**.
Instead of leaving the rest of the library on an older Music generation, this
candidate bank derives one shared enhanced foundation from those three accepted
hardware results and lets each preset keep its existing context through its
donor Mic/FX/routing/output architecture.

## Accepted anchor derivation

```text
Mode 01: [1.5, 3.0, 8.2, -0.9, 1.3, 1.0, 5.8]
Mode 03: [1.6, 3.1, 8.2, -0.8, 1.3, 1.2, 6.0]
Mode 05: [1.6, 2.9, 8.0, -1.0, 1.3, 1.2, 5.7]

median : [1.6, 3.0, 8.2, -0.9, 1.3, 1.2, 5.8]
```

Bands are 11.704 kHz, 158 Hz, 66 Hz, 1.315 kHz, 2.916 kHz, 6.495 kHz and
HS 766 Hz. Every target value is inside the already-accepted 01/03/05 envelope.

## Surgical scope

Candidates: **02, 04, 06, 07, 08, 09, 10**.

Only the seven Music PEQ gain fields change. The patch does not alter:

- Music frequencies, Q or filter type aliases;
- Mic A/B or dynamics;
- Reverb/Echo;
- Main/Surround/Center/Sub routing, EQ, output gain or dynamics;
- crossover types/frequencies;
- delays;
- names;
- reserved/unknown bytes.

This means the shared Music path is enhanced while each mode's genre/use-case
identity remains encoded in the donor layers around it.

## Hardware gate

Audition with the same gain chain and familiar tracks. Recommended order:

1. 02 MC HOST RADIO — verify backing music is clearly enhanced while MC vocal remains dry/controlled.
2. 09 ACOUSTIC NATURAL — strongest check that the common foundation does not destroy natural scale.
3. 07 JAZZ LOUNGE — verify intimacy remains despite the stronger Music core.
4. 06 SHOLAWAT SYAHDU — verify long-session softness remains.
5. 04 POP ROCK BALLAD — verify punch/clarity without fatigue.
6. 08 BLUES CLUB — verify warm vintage character remains.
7. 10 REGGAE DUB — verify no excess low-frequency accumulation with its existing strong Sub architecture.

If one mode needs correction, change only the smallest causal Music band(s);
do not roll back the entire shared foundation or rewrite its vocal/FX identity.

Exact candidate bytes and hashes are in `manifest.json`.
