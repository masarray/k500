# K500 v1.1.2 — native Windows GUI/UX visual acceptance

**Status:** manual release-gate checklist, not proof of visual QA or hardware acceptance.
**Source of truth:** tested commit and Windows artifact from its exact successful K500 CI run. Do not use an older build, and never promote a rebuilt RC instead of an accepted binary.

## Matrix: record screenshots + pass/fail

Test at **100%, 125%, and 150% Windows display scaling**, default **1484×920** and minimum **1260×800** app window. Capture OS version, monitor resolution, binary/artifact name and exact Git SHA.

| Surface | Visual and behavioral acceptance |
|---|---|
| Top Bar | Full 52px header, no collision of brand, transport, long live preset title, USB/BT selection, Connect, status, About and Report; report exports JSON |
| Navigation | Music, Mic A/B, Reverb, Echo, Main, Surround, Center, Sub, System legible; exactly one selected page, no persistent secondary focus |
| Music Input | Six exclusive sources; OPTIC/UAUDIO mirror only one verified DIGITAL gain; 160px faders and dB readouts readable, not clipped |
| Music Tone | KEY DOWN / ORIGINAL / KEY UP and current pitch visibly readable; all five tone controls still obey verified ownership/readback |
| EQ | Fixed 8-page lifetime survives repeated section switches; PEQ drag, wheel-Q, bypass, reset, A/B and HPF/LPF inspector unchanged |
| Filter dropdown | 10px value/options readable in compact heights, no text overlaps or clipped type labels |
| System settings | Title/detail/APPLYING or SYNCING status legible inside unchanged 50px toggle rows; disabled, pending and offline states truthful |
| System presets | PC staging separate from 10 physical slots; selected cyan is not hardware ACTIVE amber; Recall and Save remain explicitly gated |
| Mass Upload | At 0, 1, 5 and 10 staged slots, all rows 01–10 are understandable at both window sizes; Ctrl/Shift preserve manual order; Stage and Clear affect only PC-side plan; no last row clipped |
| Transfer | USB gate, ACK-driven progress, no duplicate writes; Slot 01 final Recall and 939-byte authoritative readback verified separately on real K500 |
| Popups | About, donation/QRIS and updater text readable, no overlap/stacking, no accidental premature device writes |

## Practical acceptance workflow

1. Verify exact-head Fast Contracts and Windows Build + Regression green; installer smoke only if affected files require it.
2. Use runnable Windows artifact from that exact run for offline visual screenshots; test dialog popup, interactions, keyboard, wheel and long names.
3. Separately on a test K500 via **USB HID**, verify Connect/readback/reconnect, slot truth and other hardware-specific assertions. Bluetooth remains experimental.
4. If a screenshot fails, attach screenshot, Windows scaling, binary SHA, page, expected/actual and concise repro. Limit fixes to the owning component; rerun the existing consolidated CI.
5. Public v1.1.2 promotion requires review of this matrix **and** device-specific RC updater acceptance. CI/lint passing alone is not a visual or hardware release approval.

## Acceptance record

- Commit SHA / exact GitHub run / artifact:
- Windows version / monitor / scaling:
- Viewports checked (1484×920, 1260×800):
- Screenshot evidence links:
- Hardware USB-connected? Device/firmware:
- Failures / reproduction and fixes:
- Reviewer / approval date:
