# K500 v1.1.2 — native Windows GUI/UX visual acceptance

**Status:** manual release-gate checklist, not proof of visual QA or hardware acceptance.
**Source of truth:** tested commit and Windows artifact from its exact successful K500 CI run. Do not use an older build, and never promote a rebuilt RC instead of an accepted binary.

## V6 minimum-window width budget

The former 1260px app minimum could squeeze the lower processor rack below
its declared child minima. The 1344px minimum reserves 24px app margins,
170px navigation and a 12px gap, leaving **1138px** for the processor.

| Lower rack | Required minimum | Workspace at 1344px |
|---|---:|---:|
| Music | 1132px | 1138px |
| Mic | 1136px | 1138px |
| Main / Surround / Center / Sub | 1132px | 1138px |

Both 216px right-hand columns, the 160px fader travel and 304px rack height
stay unchanged. At the old minimum there was a 78px Music deficit. Still
capture real Windows images at 100%, 125%, and 150% to certify the result.

## Matrix: record screenshots + pass/fail

Test at **100%, 125%, and 150% Windows display scaling**, default **1484×920** and minimum **1344×800** app window. Capture OS version, monitor resolution, binary/artifact name and exact Git SHA.

| Surface | Visual and behavioral acceptance |
|---|---|
| Top Bar | Full 52px header, no collision of brand, transport, long live preset title, USB/BT selection, Connect, status, About and Report; report exports JSON |
| Navigation | Music, Mic A/B, Reverb, Echo, Main, Surround, Center, Subwoofer, System legible; exactly one selected page, no persistent secondary focus |
| Music Input | Six exclusive sources; OPTIC/UAUDIO mirror only one verified DIGITAL gain; 160px faders and dB readouts readable, not clipped |
| Music Tone | KEY DOWN / ORIGINAL / KEY UP and current pitch visibly readable; all five tone controls still obey verified ownership/readback |
| EQ | Fixed 8-page lifetime survives repeated section switches; PEQ drag, wheel-Q, bypass, reset, A/B and HPF/LPF inspector unchanged |
| Filter dropdown | 10px value/options readable in compact heights, no text overlaps or clipped type labels |
| System settings | At rest the rows show only Use Init Volume and VR / Trim Pot Off (no permanent Startup volume / front-panel subtitles); explanations appear on hover, while APPLYING/SYNCING and blocked-operation feedback remain visible, with disabled/pending/offline states truthful |
| System presets | One-line preset names, no repeated SELECT or marketing descriptions; STAGED/INVALID and real action errors remain visible. PC staging remains separate from physical slots; cyan selection is never hardware ACTIVE amber. Recall and Save remain explicitly gated |
| Mode Name / BT Name | Offline: one clear “Connect K500 to read” hint per field and no overlapping retained draft; online: original edit, rename and verification gates preserved; BT 8-character write limit available via Rename tooltip, not permanent footer copy; inert Reset all hidden |
| Small text | Preset names readable as a single line at 11px; retain user-critical STAGED/INVALID, sync failures and USB connection guidance while removing decorative/technical copy. Recording/Mic Trigger captions readable on 100/125/150% Windows scaling |
| Font and graphic AA | All app text resolves through embedded Plus Jakarta Sans (four bundled faces) with native antialiasing; EQ curves, Canvas dial strokes and Lucide Shape icons remain smooth at 100/125/150% scaling; no doubled hover label outlines |
| Mass Upload | At 0, 1, 5 and 10 staged slots, rows 01–10 remain visible without repeated NOT STAGED, source descriptions or duplicate slot badges. Ctrl/Shift retain chronological selection (discoverable in header help tooltip); stage origin/file traceability is available on hover. Stage/Clear affect PC-side plan only; no last row clipped |
| Transfer | USB gate, ACK-driven progress, no duplicate writes; Slot 01 final Recall and 939-byte authoritative readback verified separately on real K500 |
| Popups | About, donation/QRIS and updater text readable, no overlap/stacking, no accidental premature device writes |

## Practical acceptance workflow

1. Verify exact-head Fast Contracts and Windows Build + Regression green; installer smoke only if affected files require it.
2. Use runnable Windows artifact from that exact run for offline visual screenshots; test dialog popup, interactions, keyboard, wheel and long names.
3. Separately on a test K500 via **USB HID**, verify Connect/readback/reconnect, slot truth and other hardware-specific assertions. Bluetooth remains experimental.
4. When checking quiet-copy UX, verify that invalid presets, failed sync, actual device transfer errors, verified progress and missing USB are still disclosed when actionable, while success/tutorial prose is not always visible. If a screenshot fails, attach scaling, binary SHA, page and repro; fix only the owning component and rerun the existing consolidated CI.
5. Public v1.1.2 promotion requires review of this matrix **and** device-specific RC updater acceptance. CI/lint passing alone is not a visual or hardware release approval.

## Acceptance record

- Commit SHA / exact GitHub run / artifact:
- Windows version / monitor / scaling:
- Viewports checked (1484×920, 1344×800):
- Screenshot evidence links:
- Hardware USB-connected? Device/firmware:
- Failures / reproduction and fixes:
- Reviewer / approval date:
