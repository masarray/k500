# FBE / FBX preset-file mapping evidence

Date: 2026-10-06

These five 1144-byte `.k500` files are controlled native exports from the same
preset/state with only the K500 FBE level changed from OFF through Level 4.

## Proven mapping

```text
file[0x0023]       = FBE / FBX raw level 0..4
activeMemory[0x1B] = FBE / FBX raw level 0..4
file[0x001B]       = Mic HP type raw (unchanged across the sweep)
file[0x001C]       = Mic LP type raw (unchanged across the sweep)
```

Every adjacent fixture pair differs at exactly:

```text
0x0023  FBE level (+1)
0x0475  additive checksum (-1)
```

All five files satisfy `sum(all 1144 bytes) % 256 == 0`.

| Fixture | SHA-256 | 0x0023 | 0x001B | 0x001C | checksum 0x0475 |
|---|---|---:|---:|---:|---:|
| FBE0.k500 | `ffeb1e3968b4e0dcf5437442d52fd948eeee91dfdfd6552ac3a0e31a05d9cf7a` | 0 | 7 | 7 | 50 |
| FBE1.k500 | `fe57c12e5f068e3b3c4d665a8874d78b46c584678d4306718ddae48257341d7a` | 1 | 7 | 7 | 49 |
| FBE2.k500 | `840cbc27595c4b1ee3a7f1a86beb94a1329f055a435799506a640ce068328f05` | 2 | 7 | 7 | 48 |
| FBE3.k500 | `9742ec33cc510fd68092bc0ac7915a9c27db82778548bb41275f81a7d076e9d5` | 3 | 7 | 7 | 47 |
| FBE4.k500 | `271cca43e0ff9e1e312c068eb0c528bb59d00137bdc39a16060751c1385db7fb` | 4 | 7 | 7 | 46 |

The P3.4 persistence regression consumes these exact fixtures and requires the
offline writer to reproduce every captured level byte-for-byte from FBE0.
This is an evidence fixture, not a sonic preset donor.
