# v1.1.2 candidate — UAUDIO companions and Mass Upload workflow

## PC library 11–20 (not hardware slots)

The K500 has **10 hardware slots**, unchanged. SonKuPik's PC preset collection
provides **20 bundled official choices**, plus local files. Entries 11–20 are
the USB Audio playback counterparts of official 01–10, in the same order:

| PC entry | Donor | Hardware-visible name |
|---|---|---|
| 11 | 01 KONSER NYANYI | KONSER UAUDIO |
| 12 | 02 MC HOST RADIO | MC HOST UAUDIO |
| 13 | 03 KAR DANGDUT | DANGDUT UAUDIO |
| 14 | 04 POP ROCK BALLAD | POP ROCK UAUDIO |
| 15 | 05 POP KENANGAN V2 | KENANGAN UAUDIO |
| 16 | 06 SHOLAWAT SYAHDU | SHOLAWAT UAUDIO |
| 17 | 07 JAZZ LOUNGE | JAZZ UAUDIO |
| 18 | 08 BLUES CLUB | BLUES UAUDIO |
| 19 | 09 ACOUSTIC NATURAL | ACOUSTIC UAUDIO |
| 20 | 10 REGGAE DUB | REGGAE UAUDIO |

K500 native six-way music input routing uses `sourceRaw=5` for UAUDIO and
`sourceRaw=2` for Bluetooth. The variant changes **only** file byte `0x000E`
to 5, hardware-visible ASCII name bytes `0x0454..0x0463` and the additive
checksum at `0x0475`. All DSP/PEQ/crossover/level/effect bytes stay identical
to the corresponding official source. The normal short names respect the
hardware's strict 16-character limit.

`tools/generate_uaudio_presets.py` deterministically produces exact native-slot-
compatible `11_...20_*.k500` binaries at **CMake configure**. These are embedded
in the Windows application as `:/presets/*.k500` and work offline, with no Python
runtime dependency. Build tooling requires Python 3. No generated preset or
donor is overwritten in `resources/presets/`; provenance hashes and changed-byte
audit are produced in `generated-uaudio-presets/uaudio-manifest.json` under the
build directory. The GitHub official sync source still tracks the ten original
official files; it never rewrites the generated companion presets.

UAUDIO vs Bluetooth sound-quality preference is a user-reported listening
observation, **not a claim** that changing the source byte alone modifies the
DSP/EQ or guarantees superior audio quality.

## Mass Upload multi-select

- Plain click selects a single PC-library row.
- **Ctrl+click** toggles individual rows; **Shift+click** selects a continuous
  range; **Ctrl+Shift+click** extends the selection.
- **Add** stages selected valid rows in library order, skipping duplicates and
  stopping at 10 destination slots. The source selection itself never writes
  device hardware.
- Double-click adds only the clicked preset. Add All retains its original
  first-valid-up-to-ten behavior; device upload stays 10→1 and is unchanged.
- Refreshing the library clears positional source selections to avoid selecting
  a different preset after an asynchronous official sync.

## EQ bypass warning

The active **EQ BYPASS** toggle is ruby-red and returns to its original neutral
state when not active, across the shared PEQ sections. This is purely visual,
without bypass polarity or packet changes.

## Qualification before stable v1.1.2

CI must pass all existing Windows regressions, generated companion byte/hash
guards, SectionEqGraph color state, and multiselect source interaction guards.
Perform GUI audition of selection semantics and real K500 USB mass upload before
promoting the candidate. Do not mutate published v1.1.1 installers.
