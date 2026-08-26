# K500 AI PRESET ENGINEERING PLAYBOOK

> **Audience:** ChatGPT/Codex/AI agents and engineers who need to analyze, create, research, simulate, compare, or improve K500 `.k500` presets.
>
> **Goal:** make the repository sufficient context for a new thread to perform the same class of preset engineering that was previously done through long conversational research.
>
> **Authority rule:** simulation helps reason about direction and causality; **real K500 hardware listening remains the final authority.**

---

# 1. Start here

Before changing any preset, read:

1. `AGENTS.md`
2. `docs/K500_BIT_PERFECT_AI_PRESET_GUIDE.md`
3. `docs/K500_PRESET_NAME_LIMIT.md`
4. this playbook
5. `tools/k500_preset_lab.py`

The binary guide is the authoritative offset/format reference. This playbook explains **how to use that map to do sonic research well**.

---

# 2. Mission: engineer a listening experience, not merely an EQ curve

A good K500 karaoke preset is not defined by a flat line or a single frequency-response target.

A successful preset usually balances several perceptual objectives at once:

- singer can find pitch quickly;
- vocal feels easy to project;
- weak singers receive enough support and forgiveness;
- skilled singers still hear a clean, controllable direct cue;
- dynamics feel alive rather than clamped;
- echo/reverb create reward and scale without hiding articulation;
- music has foundation, punch, detail and air without masking vocal;
- Main / Center / Surround / Sub work as one spatial system rather than duplicate feeds.

For karaoke research, useful working dimensions are:

| Dimension | What the singer/listener perceives |
|---|---|
| Pitch Cue / Control | how clearly the singer hears note center and consonant/vowel definition |
| Confidence Support | how much the system makes the singer feel acoustically supported |
| Forgiveness | how tolerant the preset feels to imperfect tone, timing and projection |
| Dynamic Bounce | whether the vocal responds naturally instead of sounding clamped or static |
| Envelopment | width, room, halo and spatial scale |
| Joy / Musical Reward | harmonic polish, movement, air, groove and satisfying FX return |
| Harshness Guard | protection from aggressive upper-mid / sibilant / ringing energy |
| Intelligibility Guard | protection from excessive wet masking or low-mid congestion |

Do not optimize one dimension in isolation.

---

# 3. Proven safety model for creating `.k500` files

The safest creation model is **donor-based surgical editing**:

```text
known-good donor bytes
    -> explicit semantic patch
    -> write only proven offsets
    -> checksum last
    -> changed-byte audit
    -> validate 1144 bytes / checksum / name
    -> simulation + graphs
    -> hardware test
```

Never start with a blank or zero-filled 1144-byte file.

Why donor-based matters:

- unknown/reserved fields survive untouched;
- firmware-specific padding and aliases survive;
- `Alt` blocks remain intact;
- hidden device-owned values remain intact;
- no-op behavior can remain byte-identical;
- every sonic revision can be causally audited.

The repository tool `tools/k500_preset_lab.py patch` implements this philosophy.

---

# 4. K500 signal-flow mental model

Use this conceptual model when reasoning about cumulative response:

```text
                               +-> Mic Dry ---------------------------+
                               |                                     |
Mic A/B -> Gain/Dynamics -> Mic PEQ/XO                               |
                               |                                     |
                               +-> Reverb PEQ/XO -> Reverb return ---+--> Output routing --> Output PEQ/XO
                               |                                     |
                               +-> Echo PEQ/XO  -> Echo return ------+

Music Input -> Music PEQ/XO -------------------------------------------> Output routing --> Output PEQ/XO
```

The important rule is:

> **Dry Mic, Reverb and Echo contributions are parallel branches.**

Do not add Reverb EQ then Echo EQ as if the vocal passes through both sequentially.

For each output, think source-by-source:

```text
Mic dry to Main      = Mic chain * Main chain * MainMic route
Mic reverb to Main   = Mic chain * Reverb chain * Main chain * Reverb route
Mic echo to Main     = Mic chain * Echo chain * Main chain * Echo route
Music to Main        = Music chain * Main chain * MainMusic route
```

Likewise for Center, Surround and Sub.

---

# 5. Output roles: design the system, not four copies of Main

## 5.1 Main

Conceptual role:

```text
front image + tonal body + primary vocal + music punch
```

Main should normally carry the most authoritative direct image.

Use Main EQ to shape final front-system balance, but remember it affects both dry/wet vocal contributions and music routed through Main.

A change in Main upper mids can therefore alter:

- singer articulation;
- wet vocal brightness;
- music presence;
- total harshness.

Treat Main as a powerful shared stage, not a convenient place to fix one source blindly.

## 5.2 Center

Conceptual role:

```text
lead anchor / vocal localization
```

A useful Center tends to stabilize the singer without becoming a duplicate Main.

Typical direction:

- stronger Mic proportion than Music;
- controlled Reverb/Echo;
- enough body for vocal authority;
- avoid low-frequency competition with Main/Sub.

## 5.3 Surround

Conceptual role:

```text
width + air + ambience + decorrelated halo
```

Surround is usually more successful when it is **felt as space** instead of heard as another singer behind the listener.

Typical direction:

- lower direct Mic/Music than Main;
- proportionally stronger wet returns;
- controlled low end;
- optional modest asymmetric delay for decorrelation;
- reduced wet midrange where localization becomes obvious.

## 5.4 Sub

Conceptual role:

```text
deep music foundation
```

For clean karaoke:

- Music is normally the meaningful Sub source;
- Mic/Reverb/Echo feeds are usually zero or near-zero;
- use Main for musical upper-bass body/punch;
- use Sub for deep extension.

Do not solve weak 100–180 Hz punch by blindly raising 40–70 Hz sub energy.

---

# 6. Frequency-region vocabulary for K500 research

These are practical engineering bands, not psychoacoustic laws.

| Region | Approx. band | Typical interpretation |
|---|---:|---|
| Deep foundation | 45–90 Hz | sub depth, weight |
| Punch | 90–180 Hz | kick/body impact |
| Vocal/music body | 120–250 Hz | roundness, chest, warmth |
| Mud / box | 250–500 Hz | congestion, masking risk |
| Presence/control | 1.5–2.5 kHz | articulation, note cue |
| `i`-ring guard | 2.5–4.5 kHz | vowel ringing / sharp upper-mid risk |
| Harmonic detail | 5–7 kHz | definition, polish, brightness |
| Sparkle | 7–10 kHz | sheen, crispness |
| Air | 10–14 kHz | openness / “fresh air” |

Important lesson from real K500 iteration:

> **3.2–4.5 kHz should be treated primarily as a guardrail, not as the universal “detail” knob.**

If a vocal needs more premium detail, first consider controlled 5–7 kHz harmonics and 10–14 kHz air rather than adding broad 3–4 kHz energy.

---

# 7. Empirical sonic patterns learned from hardware iteration

These are useful hypotheses, not universal laws.

## 7.1 FAST SUPPORT + CLEAN TAIL

A strong universal karaoke architecture is:

```text
clear dry cue
+ fast early support
+ filtered musical echo
+ clean / controlled long tail
```

Why it works:

- beginner hears immediate acoustic support;
- skilled singer still receives a direct control cue;
- tail creates scale without stealing consonants;
- filtered echo can add rhythmic reward without upper-mid glare.

## 7.2 More wet is not automatically more enjoyable

Once a preset already feels supportive, simply increasing Reverb or Echo often produces:

- poorer pitch cue;
- less intelligibility;
- smeared words;
- monotonous “big room” sensation;
- fatigue.

When a preset is already “enak” but needs more reward, look for:

- harmonic polish around 5–7 kHz;
- air around 10–14 kHz;
- better bass/body balance;
- music micro-detail;
- filtered FX motion;
- dynamic bounce;

before increasing global wetness.

## 7.3 Mic body architecture strongly affects singability

Hardware iteration showed that temporal FX settings alone do not explain singability.

A common successful pattern was stronger control of very-low vocal body combined with useful broad lower-mid support, rather than simply adding 130 Hz warmth.

Practical implication:

- compare the whole Mic PEQ architecture of a successful donor;
- do not copy only Reverb/Echo times;
- inspect 100–700 Hz body/mud structure before declaring a preset “FX problem”.

## 7.4 Wet upper-mid ridges can make `ii/kii/jii/cii` sharp

A vocal can look acceptable in dry Mic response but still become sharp because Reverb/Echo branches reinforce 3–4 kHz.

Therefore always inspect:

- dry Mic 2.5–4.5 kHz;
- Reverb path 2.5–4.5 kHz;
- Echo path 2.5–4.5 kHz;
- cumulative Main vocal composite.

Do not fix a wet-path problem with a destructive broad dry-vocal treble cut unless hardware evidence requires it.

## 7.5 Round/punchy bass is not the same as more sub

If 55–90 Hz is already strong but music feels hollow or lacks impact, test moving energy toward 120–180 Hz before adding more deep bass.

Typical strategy:

```text
slightly less sub dominance
+ more controlled 120–180 Hz body/punch
+ keep 250–500 Hz mud under control
```

---

# 8. Choosing a donor preset

Choose the donor closest to the desired behavior.

Good donor dimensions:

- intended genre/use case;
- proven hardware enjoyment;
- desired amount of support;
- desired Reverb/Echo architecture;
- desired Mic body architecture;
- desired output-routing philosophy;
- firmware generation.

Avoid choosing a donor solely because its filename is semantically similar.

If the goal is “a more forgiving Rock preset,” a proven enjoyable vocal-support donor may be a better base than an older file named ROCK.

---

# 9. Research workflow for a new preset

## Step 1 — define the listening problem precisely

Weak request:

```text
make it better
```

Useful request:

```text
vocal is easy to sing but `i` vowel rings at high projection;
music low end has deep sub but lacks 120–170 Hz punch;
keep current air and reverb scale.
```

Translate listening language to a hypothesis table:

| Listening observation | Candidate cause |
|---|---|
| bindeng / too thick | excessive 150–500 Hz, routing accumulation, insufficient upper harmonics |
| thin / no body | insufficient 120–250 Hz or excessive HPF |
| `i` ringing | local 2.5–4.5 kHz ridge in dry or wet path |
| sibilant | excessive 5–10 kHz, source/mic dependent |
| dull | insufficient detail/air, excessive wet-mid masking |
| no punch | weak 90–180 Hz relative to sub |
| boomy | too much 45–100 Hz or room/sub overlap |
| hard to sing | weak dry cue, wrong body architecture, excessive masking, dynamics too clampy |
| too dry | insufficient early support / wet return |
| too washed | too much wet occupancy / decay / repeat / return |

## Step 2 — lock what is already good

Write an explicit lock list:

```text
LOCK:
- Main routing
- Reverb decay
- 10–14 kHz air
- Mode03 vocal architecture
```

A new revision should not casually modify locked areas.

## Step 3 — inspect donor and reference

```bash
python tools/k500_preset_lab.py inspect donor.k500 --json donor.json
python tools/k500_preset_lab.py inspect reference.k500 --json reference.json
```

Compare:

- Mic A/B bands;
- Music bands;
- Main/Center/Surround/Sub bands;
- Reverb/Echo bands;
- HPF/LPF;
- route levels;
- compressor parameters;
- Reverb decay/predelay;
- Echo delay/repeat.

## Step 4 — generate response plots

```bash
python -m pip install -r tools/requirements-preset-lab.txt
python tools/k500_preset_lab.py plot donor.k500 --out-dir donor-analysis/
```

The tool creates:

- `*_inputs.png`
- `*_vocal_fx.png`
- `*_outputs.png`
- `*_curves.csv`
- `*_report.json`

## Step 5 — form one narrow causal experiment

Example:

```text
Hypothesis:
Mode feels deep but not punchy because sub region dominates 120–180 Hz body.

Experiment:
+1.5 dB at existing Music ~150 Hz band
-0.3 dB at existing ~60 Hz band
leave Music presence/air and all vocal/FX unchanged.
```

This is better than changing ten unrelated controls in one revision.

## Step 6 — patch donor surgically

Create a JSON patch spec.

Example:

```json
{
  "name": "DANGDUT V26",
  "scalars": {
    "mic.comp.threshold_db": -11,
    "reverb.decay_ms": 1480,
    "echo.delay_ms": 330,
    "echo.repeat": 3,
    "main.mic": 93,
    "main.reverb": 84,
    "main.echo": 48
  },
  "eq": {
    "music": {
      "bands": {
        "2": {"gain_db": 1.5},
        "3": {"gain_db": 7.7}
      }
    },
    "micA": {
      "bands": {
        "6": {"gain_db": -2.0}
      }
    },
    "micB": {
      "bands": {
        "6": {"gain_db": -2.0}
      }
    }
  }
}
```

Then:

```bash
python tools/k500_preset_lab.py patch \
  donor.k500 \
  patch.json \
  candidate.k500
```

The tool writes a sibling `.audit.json` containing changed offsets and validation results.

## Step 7 — compare candidate to donor

```bash
python tools/k500_preset_lab.py compare \
  donor.k500 candidate.k500 \
  --out-dir comparison/
```

The graph is **candidate minus donor**.

Use it to answer:

- where energy moved;
- whether the change was local or broad;
- whether a body fix accidentally changed air;
- whether an `i`-ring guard also removed desirable 5–7 kHz detail.

## Step 8 — test on K500 hardware

Record:

```text
WHAT IMPROVED
WHAT REGRESSED
WHAT IS UNCHANGED
WHAT SHOULD BE LOCKED NEXT
```

Hardware feedback wins over the simulation score.

## Step 9 — create next revision from the best proven parent

Do not keep building from a rejected candidate just because its version number is newer.

Branch from the **best hardware-proven revision**.

---

# 10. Simulation model: what is valid to infer

The lab uses a comparative 48 kHz engineering model.

## 10.1 PEQ

Known PEQ/shelf parameters are modeled with RBJ-style biquads.

This is useful for:

- relative curve shape;
- cumulative serial response;
- local peaks/dips;
- comparing revisions.

It is not proof that K500 firmware uses the same floating-point coefficient implementation.

## 10.2 Crossovers

Known crossover families/orders are represented with comparative magnitude proxies.

Use them to reason about:

- relative low/high rolloff;
- overlap;
- gross source/output bandwidth.

Do not claim exact phase or hardware transfer-function equivalence.

## 10.3 Serial paths

For serial filters:

```text
H_total(f) = H_1(f) * H_2(f) * ...
```

or in dB:

```text
G_total_dB(f) = G_1_dB(f) + G_2_dB(f) + ...
```

Examples:

```text
Mic -> Main dry = Mic EQ/XO * Main EQ/XO
Music -> Main   = Music EQ/XO * Main EQ/XO
Mic -> Reverb -> Main = Mic * Reverb * Main
```

## 10.4 Parallel dry/wet composites

Dry, Reverb and Echo are not phase-coherent copies in a real room.

A useful comparative proxy is decorrelated energy summation:

```text
P(f) = (g_dry H_dry)^2
     + (g_rev H_rev)^2
     + (g_echo H_echo)^2

H_proxy(f) = sqrt(P(f))
```

The lab labels these curves as **composites/proxies**, not exact acoustic response.

---

# 11. Graphs every serious preset study should inspect

## 11.1 Inputs / serial front paths

Plot:

- Mic;
- Music;
- Main EQ;
- Mic→Main Dry;
- Music→Main.

Question answered:

> Is the source itself imbalanced, or is Main adding the problem?

## 11.2 Vocal FX paths

Plot:

- Mic→Main Dry;
- Mic→Reverb→Main;
- Mic→Echo→Main;
- Main Vocal Composite proxy.

Question answered:

> Is sharpness/masking coming from the dry cue or a wet branch?

## 11.3 Output-role view

Plot:

- Main Vocal Composite;
- Main Music contribution;
- Surround Vocal Composite;
- Center Vocal Composite;
- Music→Sub.

Question answered:

> Are outputs performing complementary roles, or duplicating each other?

---

# 12. Metrics: use as guardrails, not as a score to maximize

The lab exports averages for:

```text
sub_45_90
punch_90_180
body_120_250
mud_250_500
presence_1500_2500
i_ring_2500_4500
detail_5000_7000
sparkle_7000_10000
air_10000_14000
```

It also reports peak magnitude/frequency inside the `i`-ring region.

Do not rank presets by a single metric.

Example:

A proven enjoyable hardware preset may show more numerical 2.5–4.5 kHz energy than another preset because:

- Q is broader/narrower;
- wet routing differs;
- dynamic behavior differs;
- body support changes perceived balance;
- acoustic playback changes the result.

Metrics are best used for:

```text
same donor -> small candidate change -> did the intended region move?
```

not:

```text
lowest i_ring metric = best preset
```

---

# 13. Dynamics research

Static EQ graphs do not capture compression behavior.

When a preset feels:

- “ngayun” / pumping;
- hard to project;
- too flat;
- inconsistent;
- shouty only when loud;

inspect compressor parameters before over-EQing.

Useful dimensions:

- threshold;
- ratio;
- attack;
- release;
- upstream level;
- output headroom.

A rough static gain-reduction proxy can be useful for comparison, but hardware listening and level-step measurement are required for confidence.

---

# 14. Reverb / Echo research

## Reverb

Think in terms of:

- early confidence/support;
- decay occupancy;
- predelay separation;
- HPF/LPF cleanliness;
- wet EQ mid masking;
- output return distribution.

A useful support proxy for comparing similar presets is conceptually related to:

```text
return_level * sqrt(decay_time)
```

but this is not a device law.

## Echo

Think in terms of:

- delay relative to musical rhythm;
- repeat count / memory;
- wet bandwidth;
- return level;
- overlap with Reverb;
- whether repeated 2.5–4.5 kHz energy creates vowel ringing.

A long bright echo can sound impressive alone and still make singing less comfortable.

---

# 15. How to preserve “detail” while reducing sharp `i`

When hardware reports `i/ii/kii/jii/cii` ringing:

1. inspect dry Mic 2.5–4.5 kHz;
2. inspect Reverb/Echo path in the same region;
3. identify whether the ridge is local or broad;
4. apply the smallest local guard;
5. verify 5–7 kHz detail and 10–14 kHz air remain intact.

Preferred adjustment order:

```text
local 3–4 kHz guard
-> wet-path guard if wet branch is responsible
-> only then consider broader high-frequency change
```

Avoid using a broad 6–14 kHz cut to fix a narrow 3–4 kHz vowel problem.

---

# 16. How to make music bass more round and punchy

First inspect the relationship among:

- 45–90 Hz deep foundation;
- 90–180 Hz punch;
- 120–250 Hz body;
- 250–500 Hz mud.

Common situations:

## Deep but hollow

Try:

```text
slightly reduce extreme sub dominance
+ add controlled 120–180 Hz
```

## Punchy but no foundation

Try:

```text
small deep-bass increase
without adding 250–500 Hz
```

## Thick / boxy

Try:

```text
reduce 150–350 Hz overlap
before adding treble to “compensate”
```

## Main sounds good but room still lacks physical low-end

Then inspect Sub routing/PEQ/crossover and actual room/speaker phase rather than forcing more bass into Music/Main EQ.

---

# 17. Designing genre modes without losing universal singability

A genre preset should differ in musical character while preserving the core singer-support contract.

Useful separation:

```text
shared singability DNA:
- dry cue clarity
- useful body architecture
- dynamics behavior
- early support
- harshness guard

mode character:
- music low-end contour
- harmonic detail
- reverb scale
- echo rhythm/memory
- Center/Surround distribution
```

This prevents genre modes from becoming “good sound, hard to sing”.

---

# 18. Controlled experimental design

For each revision, write:

```text
Parent:
Goal:
Hypothesis:
Changed parameters:
Locked parameters:
Expected graph delta:
Expected listening delta:
Hardware result:
Decision:
```

Example:

```text
Parent: Mode01 V24
Goal: reduce sharp i without losing fresh air
Hypothesis: local 3.9 kHz Mic ridge, not global treble
Change: Mic A/B 3.9 kHz -0.4 dB
Lock: 6.3 kHz, 11 kHz shelf, Reverb/Echo, Music
Expected: -0.2..-0.4 dB i-ring, near-zero air delta
Hardware: i softer, openness unchanged
Decision: keep; lock as new parent
```

This discipline makes a long research program understandable to another AI thread.

---

# 19. Versioning strategy

Do not equate “highest version number” with “best parent”.

Maintain three labels conceptually:

```text
LATEST EXPERIMENT
BEST SIMULATION CANDIDATE
BEST HARDWARE-PROVEN
```

Only **BEST HARDWARE-PROVEN** should become the default parent for the next broad iteration unless the experiment specifically branches elsewhere.

---

# 20. Binary patch rules for AI-generated candidates

Before delivering any `.k500` candidate, verify:

```text
[ ] source donor recorded
[ ] output exactly 1144 bytes
[ ] checksum modulo 256 == 0
[ ] name <=16 characters
[ ] checksum written last
[ ] raw PEQ type aliases preserved unless explicitly targeted
[ ] unknown crossover footer bytes unchanged
[ ] Alt blocks unchanged unless explicitly researched
[ ] FBX 0x1B/0x1C preserved individually unless targeted
[ ] only intended scalar/EQ/crossover offsets changed
[ ] diff audit contains zero unexpected offsets
[ ] graph/report regenerated from final output bytes
```

The patch lab enforces much of this automatically.

---

# 21. Patch-lab JSON schema by example

The CLI deliberately supports a conservative subset of proven writes.

```json
{
  "name": "ALLGENRE V27",
  "scalars": {
    "mic.comp.threshold_db": -11,
    "mic.comp.ratio": 3,
    "mic.comp.attack_ms": 10,
    "mic.comp.release_s": 0.2,

    "main.mic": 94,
    "main.music": 48,
    "main.reverb": 84,
    "main.echo": 42,

    "center.mic": 94,
    "center.music": 48,
    "surround.mic": 80,
    "surround.music": 72,

    "reverb.level": 98,
    "reverb.decay_ms": 1450,
    "reverb.predelay_ms": 20,

    "echo.level": 50,
    "echo.repeat": 2,
    "echo.delay_ms": 300
  },
  "eq": {
    "micA": {
      "bands": {
        "4": {"gain_db": -1.8},
        "6": {"gain_db": -2.7}
      }
    },
    "micB": {
      "bands": {
        "4": {"gain_db": -1.8},
        "6": {"gain_db": -2.7}
      }
    },
    "music": {
      "bands": {
        "2": {"gain_db": 1.8},
        "3": {"gain_db": 8.2}
      }
    }
  },
  "crossovers": {
    "music": {"hpf_hz": 60},
    "reverb": {"hpf_hz": 220, "lpf_hz": 12000},
    "echo": {"hpf_hz": 700, "lpf_hz": 4400}
  }
}
```

Important:

- EQ band indexes are **1-based**.
- `type_raw` writes are intentionally blocked by the tool.
- `Alt` EQ writes are intentionally blocked.
- crossover changes update the proven primary scalar + footer mirror(s).
- unsupported keys fail closed.

---

# 22. Recommended graphical artifact set for research bundles

For any important revision, preserve:

```text
candidate.k500
candidate.audit.json
candidate_report.json
candidate_curves.csv
candidate_inputs.png
candidate_vocal_fx.png
candidate_outputs.png
comparison_delta.png
comparison_report.json
NOTES.md or AUDIT.txt with real hardware feedback
```

This gives the next thread enough evidence to continue without reconstructing assumptions.

---

# 23. Objective hardware measurement phase

When subjective tuning reaches a strong plateau, measure the real device.

Useful tests:

- digital/analog loopback reference;
- K500 flat-path sweep;
- sine level steps;
- multitone;
- pink noise;
- clipping onset;
- limiter/compressor onset;
- frequency response;
- THD+N;
- SNR;
- crest-factor behavior.

Especially inspect output headroom before changing large output gains.

Simulation cannot reveal analog clipping, converter headroom, room modes, speaker crossover summation or firmware limiter details.

---

# 24. How a new ChatGPT thread should answer “make me a new preset”

A capable thread should not immediately invent EQ values.

It should:

1. identify the intended use case and listening goal from the request/context;
2. inspect the repository and current built-in/reference presets;
3. select a proven donor;
4. generate baseline plots and metrics;
5. explain the causal hypothesis briefly;
6. create a donor-based patch spec;
7. run the patch tool;
8. validate byte-level safety;
9. generate candidate graphs and comparison;
10. deliver the `.k500` plus audit artifacts;
11. ask for / use real K500 listening feedback for the next iteration.

If real hardware feedback is already supplied, it overrides an aesthetically nicer simulation curve.

---

# 25. How a new thread should answer “why does this preset sound X?”

Use this order:

```text
1. Mic source/body architecture
2. Music source curve
3. dry/wet routing proportions
4. Reverb/Echo spectral shaping
5. Reverb/Echo temporal occupancy
6. Main cumulative response
7. Center/Surround/Sub role interaction
8. dynamics/headroom
9. room/speaker/device measurement
```

This avoids the common failure of blaming every subjective issue on one PEQ band.

---

# 26. Current repository starter bank

The application contains a curated 10-preset public starter bank under:

```text
resources/presets/
```

Treat those files as useful current references, not immutable truth.

A new research thread should inspect the actual bytes present in the current `main` branch rather than assuming historical version values from old conversations.

---

# 27. Final principle

The best K500 preset research loop is:

```text
hardware listening
    -> precise perceptual observation
    -> signal-flow hypothesis
    -> smallest plausible binary change
    -> comparative simulation / graphs
    -> bit-perfect validation
    -> hardware listening
```

The purpose of the repository is to make that loop **repeatable across threads, engineers and future versions** without losing the hard-won binary and sonic knowledge.