#!/usr/bin/env python3
"""K500 preset research / simulation / surgical patch lab.

Safety model:
- donor-based only
- preserve unknown bytes
- patch explicit proven fields only
- recompute checksum last
- fail if any byte changes outside the whitelist

Simulation is comparative, not an exact K500 DSP emulation.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import struct
import sys
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

FILE_SIZE = 0x478
CHECKSUM_OFFSET = 0x475
NAME_OFFSET = 0x454
NAME_FIELD_LENGTH = 0x21
NAME_VISIBLE_MAX = 16
FS = 48000.0

EQ_SECTIONS: Dict[str, Tuple[int, int]] = {
    "micA": (0x00F0, 10), "micB": (0x0150, 10), "music": (0x01B0, 7),
    "main": (0x01F8, 7), "mainAlt": (0x0240, 7), "surround": (0x0288, 5),
    "surroundAlt": (0x02C0, 5), "center": (0x02F8, 5), "centerAlt": (0x0330, 5),
    "sub": (0x0368, 5), "subAlt": (0x03A0, 5), "reverb": (0x03D8, 5), "echo": (0x0410, 5),
}

PRIMARY_XO: Dict[str, Dict[str, Any]] = {
    "mic": {"sections": ["micA", "micB"], "hpf_scalar": 0x98, "lpf_scalar": 0x9A},
    "music": {"sections": ["music"], "hpf_scalar": 0x9C, "lpf_scalar": 0x9E},
    "main": {"sections": ["main"], "hpf_scalar": 0xA0, "lpf_scalar": 0xA4},
    "surround": {"sections": ["surround"], "hpf_scalar": 0xA8, "lpf_scalar": 0xAC},
    "center": {"sections": ["center"], "hpf_scalar": 0xB0, "lpf_scalar": 0xB4},
    "sub": {"sections": ["sub"], "hpf_scalar": 0xB8, "lpf_scalar": 0xBC},
    "reverb": {"sections": ["reverb"], "hpf_scalar": 0xC0, "lpf_scalar": 0xC2},
    "echo": {"sections": ["echo"], "hpf_scalar": 0xC4, "lpf_scalar": 0xC6},
}

XO_CODES = {1: "Bessel2/12", 2: "Butter2/12", 3: "Bessel3/18", 4: "Butter3/18", 5: "Bessel4/24", 6: "Butter4/24", 7: "LR4/24"}
TYPE_NAMES = {0x0100: "LS", 0x0200: "HS"}


def u16(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def i16(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<h", data, offset)[0]


def put_u16(data: bytearray, offset: int, value: int, touched: set[int]) -> None:
    if not 0 <= int(value) <= 0xFFFF:
        raise ValueError(f"u16 out of range at 0x{offset:04X}: {value}")
    struct.pack_into("<H", data, offset, int(value)); touched.update((offset, offset + 1))


def put_i16(data: bytearray, offset: int, value: int, touched: set[int]) -> None:
    if not -32768 <= int(value) <= 32767:
        raise ValueError(f"i16 out of range at 0x{offset:04X}: {value}")
    struct.pack_into("<h", data, offset, int(value)); touched.update((offset, offset + 1))


def put_u8(data: bytearray, offset: int, value: int, touched: set[int]) -> None:
    if not 0 <= int(value) <= 255:
        raise ValueError(f"u8 out of range at 0x{offset:04X}: {value}")
    data[offset] = int(value); touched.add(offset)


def type_name(raw: int) -> str:
    if raw in (0x0000, 0x0001, 0x0002, 0x0003): return "P"
    return TYPE_NAMES.get(raw, f"0x{raw:04X}")


def checksum_ok(data: bytes | bytearray) -> bool:
    return len(data) == FILE_SIZE and (sum(data) & 0xFF) == 0


def preset_name(data: bytes | bytearray) -> str:
    raw = bytes(data[NAME_OFFSET: NAME_OFFSET + NAME_FIELD_LENGTH]).split(b"\x00", 1)[0]
    return raw.decode("ascii", errors="replace").rstrip(" ")


def validate_bytes(data: bytes | bytearray) -> List[str]:
    errors: List[str] = []
    if len(data) != FILE_SIZE:
        errors.append(f"size={len(data)} expected={FILE_SIZE}"); return errors
    if not checksum_ok(data): errors.append(f"checksum invalid: sum%256={sum(data) & 0xFF}")
    name = preset_name(data)
    if len(name) > NAME_VISIBLE_MAX: errors.append(f"name exceeds {NAME_VISIBLE_MAX} chars: {name!r}")
    return errors


def read_file(path: str | Path) -> bytes:
    data = Path(path).read_bytes(); errors = validate_bytes(data)
    if errors: raise ValueError(f"Invalid K500 preset {path}: " + "; ".join(errors))
    return data


def eq_footer_offset(section: str) -> int:
    base, count = EQ_SECTIONS[section]; return base + 2 + count * 8


def parse_eq(data: bytes, section: str) -> Dict[str, Any]:
    base, count = EQ_SECTIONS[section]; bands = []
    for idx in range(count):
        off = base + 2 + idx * 8; tr = u16(data, off)
        bands.append({"index": idx + 1, "offset": off, "typeRaw": tr, "type": type_name(tr), "frequencyHz": u16(data, off + 2), "q": u16(data, off + 4) / 10.0, "gainDb": i16(data, off + 6) / 10.0})
    foot = eq_footer_offset(section); lp_raw = u16(data, foot); hp_raw = u16(data, foot + 8)
    return {"enabledRaw": u16(data, base), "bands": bands, "footer": {"lpTypeRaw": lp_raw, "lpCode": lp_raw & 0xFF, "lpType": XO_CODES.get(lp_raw & 0xFF, f"code-{lp_raw & 0xFF}"), "lpHz": u16(data, foot + 2), "reservedHex": bytes(data[foot + 4:foot + 8]).hex(), "hpTypeRaw": hp_raw, "hpCode": hp_raw & 0xFF, "hpType": XO_CODES.get(hp_raw & 0xFF, f"code-{hp_raw & 0xFF}"), "hpHz": u16(data, foot + 10)}}


def out_db(raw: int) -> float: return (raw - 75) / 2.0


def parse_scalars(data: bytes) -> Dict[str, Any]:
    return {
        "top": {"music": data[0x08], "mic": data[0x09], "effect": data[0x0A]},
        "mic": {"aVolume": data[0x14], "bVolume": data[0x15], "gateDb": data[0x16] - 81, "compressor": {"thresholdDb": data[0x17] - 50, "ratio": data[0x18], "attackMs": data[0x19], "releaseS": data[0x1A] / 10.0}, "fbxA": data[0x1B], "fbxB": data[0x1C], "eqLink": data[0x92] == 1},
        "music": {"source": data[0x0E], "key": data[0x11] - 7, "inputGainDb": {"input1": data[0x1E] - 12, "input2": data[0x1F] - 12, "bluetooth": data[0x20] - 12, "udisk": data[0x21] - 12, "digital": data[0x22] - 12}},
        "main": {"leftDb": out_db(data[0x24]), "rightDb": out_db(data[0x26]), "mic": data[0x28], "music": data[0x2A], "reverb": data[0x2C], "echo": data[0x2E], "compressor": {"thresholdDb": data[0x30] - 50, "ratio": data[0x31], "attackMs": data[0x32], "releaseS": data[0x33] / 10.0}},
        "surround": {"leftDb": out_db(data[0x38]), "rightDb": out_db(data[0x3A]), "mic": data[0x3C], "music": data[0x3E], "reverb": data[0x40], "echo": data[0x42], "compressor": {"thresholdDb": data[0x44] - 50, "ratio": data[0x45], "attackMs": data[0x46], "releaseS": data[0x47] / 10.0}, "delayLeftMs": u16(data, 0xD8), "delayRightMs": u16(data, 0xDA)},
        "center": {"outputDb": out_db(data[0x4C]), "mic": data[0x50], "music": data[0x52], "reverb": data[0x54], "echo": data[0x56], "compressor": {"thresholdDb": data[0x58] - 50, "ratio": data[0x59], "attackMs": data[0x5A], "releaseS": data[0x5B] / 10.0}},
        "sub": {"outputDb": out_db(data[0x60]), "mic": data[0x64], "music": data[0x66], "reverb": data[0x68], "echo": data[0x6A], "compressor": {"thresholdDb": data[0x6C] - 50, "ratio": data[0x6D], "attackMs": data[0x6E], "releaseS": data[0x6F] / 10.0}},
        "reverb": {"level": data[0x74], "hpfHz": u16(data, 0xC0), "lpfHz": u16(data, 0xC2), "decayMs": u16(data, 0xC8), "predelayMs": u16(data, 0xCA)},
        "echo": {"level": data[0x7B], "repeat": data[0x7C], "hpfHz": u16(data, 0xC4), "lpfHz": u16(data, 0xC6), "delayMs": u16(data, 0xCC)},
    }


def inspect_report(data: bytes, source: str = "") -> Dict[str, Any]:
    return {"source": source, "size": len(data), "checksumOk": checksum_ok(data), "checksumModulo": sum(data) & 0xFF, "name": preset_name(data), "scalars": parse_scalars(data), "eq": {name: parse_eq(data, name) for name in EQ_SECTIONS}}


def import_plot_libs():
    try:
        import numpy as np
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit("plot/compare requires numpy and matplotlib: pip install numpy matplotlib") from exc
    return np, plt


def rbj_response(freqs, kind: str, f0: float, q: float, gain_db: float, fs: float = FS):
    np, _ = import_plot_libs()
    if f0 <= 0 or f0 >= fs / 2 or q <= 0: return np.ones_like(freqs, dtype=float)
    A = 10 ** (gain_db / 40.0); w0 = 2 * math.pi * f0 / fs; alpha = math.sin(w0) / (2 * q); cw = math.cos(w0)
    if kind == "P":
        b0, b1, b2 = 1 + alpha * A, -2 * cw, 1 - alpha * A; a0, a1, a2 = 1 + alpha / A, -2 * cw, 1 - alpha / A
    elif kind == "LS":
        S = max(0.1, q); alpha_s = math.sin(w0) / 2 * math.sqrt((A + 1/A) * (1/S - 1) + 2); t = 2 * math.sqrt(A) * alpha_s
        b0 = A * ((A + 1) - (A - 1) * cw + t); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - t); a0 = (A + 1) + (A - 1) * cw + t; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - t
    elif kind == "HS":
        S = max(0.1, q); alpha_s = math.sin(w0) / 2 * math.sqrt((A + 1/A) * (1/S - 1) + 2); t = 2 * math.sqrt(A) * alpha_s
        b0 = A * ((A + 1) + (A - 1) * cw + t); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - t); a0 = (A + 1) - (A - 1) * cw + t; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - t
    else: return np.ones_like(freqs, dtype=float)
    w = 2 * np.pi * freqs / fs; z1 = np.exp(-1j * w); z2 = np.exp(-2j * w)
    return np.abs((b0 + b1*z1 + b2*z2) / (a0 + a1*z1 + a2*z2))


def xo_response(freqs, fc: float, raw: int, highpass: bool):
    np, _ = import_plot_libs()
    if fc <= 0: return np.ones_like(freqs, dtype=float)
    code = raw & 0xFF; order = {1: 2, 2: 2, 3: 3, 4: 3, 5: 4, 6: 4, 7: 4}.get(code, 2)
    if code == 7:
        base = 1.0 / np.sqrt(1.0 + ((fc / np.maximum(freqs, 1e-9)) if highpass else (freqs / fc)) ** 4); return base ** 2
    return 1.0 / np.sqrt(1.0 + ((fc / np.maximum(freqs, 1e-9)) if highpass else (freqs / fc)) ** (2 * order))


def section_response(data: bytes, section: str, freqs):
    np, _ = import_plot_libs(); parsed = parse_eq(data, section); h = np.ones_like(freqs, dtype=float)
    for band in parsed["bands"]: h *= rbj_response(freqs, band["type"], band["frequencyHz"], band["q"], band["gainDb"])
    foot = parsed["footer"]
    if foot["hpHz"] > 0: h *= xo_response(freqs, foot["hpHz"], foot["hpTypeRaw"], True)
    if foot["lpHz"] > 0 and foot["lpHz"] < 24000: h *= xo_response(freqs, foot["lpHz"], foot["lpTypeRaw"], False)
    return h


def db20(x):
    np, _ = import_plot_libs(); return 20.0 * np.log10(np.maximum(x, 1e-9))


def route_gain(raw: int): return max(0.0, min(2.0, raw / 100.0))


def output_paths(data: bytes, freqs) -> Dict[str, Any]:
    np, _ = import_plot_libs(); s = parse_scalars(data)
    mic = section_response(data, "micA", freqs); music = section_response(data, "music", freqs); main_eq = section_response(data, "main", freqs); surround_eq = section_response(data, "surround", freqs); center_eq = section_response(data, "center", freqs); sub_eq = section_response(data, "sub", freqs); rev = section_response(data, "reverb", freqs); echo = section_response(data, "echo", freqs)
    paths: Dict[str, Any] = {"mic": mic, "music": music, "main_eq": main_eq, "mic_to_main_dry": mic * main_eq, "music_to_main": music * main_eq, "mic_to_reverb_to_main": mic * rev * main_eq, "mic_to_echo_to_main": mic * echo * main_eq, "mic_to_surround_dry": mic * surround_eq, "music_to_surround": music * surround_eq, "mic_to_center_dry": mic * center_eq, "music_to_center": music * center_eq, "music_to_sub": music * sub_eq}
    def energy_mix(items):
        p = np.zeros_like(freqs, dtype=float)
        for h, g in items: p += (h * g) ** 2
        return np.sqrt(p)
    m = s["main"]
    paths["main_vocal_composite"] = energy_mix([(mic * main_eq, route_gain(m["mic"])), (mic * rev * main_eq, route_gain(m["reverb"]) * route_gain(s["reverb"]["level"])), (mic * echo * main_eq, route_gain(m["echo"]) * route_gain(s["echo"]["level"]))])
    paths["main_music_contribution"] = music * main_eq * route_gain(m["music"])
    su = s["surround"]
    paths["surround_vocal_composite"] = energy_mix([(mic * surround_eq, route_gain(su["mic"])), (mic * rev * surround_eq, route_gain(su["reverb"]) * route_gain(s["reverb"]["level"])), (mic * echo * surround_eq, route_gain(su["echo"]) * route_gain(s["echo"]["level"]))])
    ce = s["center"]
    paths["center_vocal_composite"] = energy_mix([(mic * center_eq, route_gain(ce["mic"])), (mic * rev * center_eq, route_gain(ce["reverb"]) * route_gain(s["reverb"]["level"])), (mic * echo * center_eq, route_gain(ce["echo"]) * route_gain(s["echo"]["level"]))])
    return paths


METRIC_BANDS = {"sub_45_90": (45, 90), "punch_90_180": (90, 180), "body_120_250": (120, 250), "mud_250_500": (250, 500), "presence_1500_2500": (1500, 2500), "i_ring_2500_4500": (2500, 4500), "detail_5000_7000": (5000, 7000), "sparkle_7000_10000": (7000, 10000), "air_10000_14000": (10000, 14000)}


def curve_metrics(freqs, h) -> Dict[str, float]:
    np, _ = import_plot_libs(); db = db20(h); out: Dict[str, float] = {}
    for name, (lo, hi) in METRIC_BANDS.items():
        mask = (freqs >= lo) & (freqs <= hi); out[name] = float(np.mean(db[mask])) if np.any(mask) else float("nan")
    mask = (freqs >= 2500) & (freqs <= 4500)
    if np.any(mask):
        ff = freqs[mask]; dd = db[mask]; idx = int(np.argmax(dd)); out["i_ring_peak_db"] = float(dd[idx]); out["i_ring_peak_hz"] = float(ff[idx])
    return out


def plot_report(data: bytes, source: str, out_dir: Path, prefix: str = "") -> Dict[str, Any]:
    np, plt = import_plot_libs(); out_dir.mkdir(parents=True, exist_ok=True); freqs = np.logspace(math.log10(20), math.log10(20000), 1600); paths = output_paths(data, freqs); stem = prefix or Path(source).stem
    groups = {"inputs": ["mic", "music", "main_eq", "mic_to_main_dry", "music_to_main"], "vocal_fx": ["mic_to_main_dry", "mic_to_reverb_to_main", "mic_to_echo_to_main", "main_vocal_composite"], "outputs": ["main_vocal_composite", "main_music_contribution", "surround_vocal_composite", "center_vocal_composite", "music_to_sub"]}
    for suffix, names in groups.items():
        fig, ax = plt.subplots(figsize=(12, 6.5))
        for name in names: ax.semilogx(freqs, db20(paths[name]), label=name)
        ax.set_xlim(20, 20000); ax.set_ylim(-30, 18); ax.set_xlabel("Frequency (Hz)"); ax.set_ylabel("Comparative magnitude (dB)"); ax.set_title(f"{preset_name(data)} — {suffix.replace('_', ' ')}"); ax.grid(True, which="both", alpha=0.25); ax.legend(fontsize=8); fig.tight_layout(); fig.savefig(out_dir / f"{stem}_{suffix}.png", dpi=160); plt.close(fig)
    csv_path = out_dir / f"{stem}_curves.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f); names = list(paths.keys()); writer.writerow(["frequency_hz"] + [f"{n}_db" for n in names]); db_paths = {n: db20(paths[n]) for n in names}
        for i, freq in enumerate(freqs): writer.writerow([f"{freq:.6f}"] + [f"{float(db_paths[n][i]):.6f}" for n in names])
    report = inspect_report(data, source); report["simulation"] = {"notice": "Comparative engineering model; real K500 hardware listening is authoritative.", "metrics": {name: curve_metrics(freqs, h) for name, h in paths.items()}, "curveCsv": str(csv_path), "plots": [str(out_dir / f"{stem}_{suffix}.png") for suffix in groups]}; (out_dir / f"{stem}_report.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8"); return report


SCALAR_PATCHERS = {
    "top.music": (0x08, "u8"), "top.mic": (0x09, "u8"), "top.effect": (0x0A, "u8"), "mic.a_volume": (0x14, "u8"), "mic.b_volume": (0x15, "u8"), "mic.gate_db": (0x16, "offset81"), "mic.comp.threshold_db": (0x17, "offset50"), "mic.comp.ratio": (0x18, "u8"), "mic.comp.attack_ms": (0x19, "u8"), "mic.comp.release_s": (0x1A, "tenths"), "mic.eq_link": (0x92, "bool"), "music.key": (0x11, "offset7"), "music.input1_gain_db": (0x1E, "offset12"), "music.input2_gain_db": (0x1F, "offset12"), "music.bluetooth_gain_db": (0x20, "offset12"), "music.udisk_gain_db": (0x21, "offset12"), "music.digital_gain_db": (0x22, "offset12"),
    "main.left_db": (0x24, "outdb"), "main.right_db": (0x26, "outdb"), "main.mic": (0x28, "u8"), "main.music": (0x2A, "u8"), "main.reverb": (0x2C, "u8"), "main.echo": (0x2E, "u8"), "main.comp.threshold_db": (0x30, "offset50"), "main.comp.ratio": (0x31, "u8"), "main.comp.attack_ms": (0x32, "u8"), "main.comp.release_s": (0x33, "tenths"),
    "surround.left_db": (0x38, "outdb"), "surround.right_db": (0x3A, "outdb"), "surround.mic": (0x3C, "u8"), "surround.music": (0x3E, "u8"), "surround.reverb": (0x40, "u8"), "surround.echo": (0x42, "u8"), "surround.comp.threshold_db": (0x44, "offset50"), "surround.comp.ratio": (0x45, "u8"), "surround.comp.attack_ms": (0x46, "u8"), "surround.comp.release_s": (0x47, "tenths"), "surround.delay_left_ms": (0xD8, "u16"), "surround.delay_right_ms": (0xDA, "u16"),
    "center.output_db": (0x4C, "outdb"), "center.mic": (0x50, "u8"), "center.music": (0x52, "u8"), "center.reverb": (0x54, "u8"), "center.echo": (0x56, "u8"), "center.comp.threshold_db": (0x58, "offset50"), "center.comp.ratio": (0x59, "u8"), "center.comp.attack_ms": (0x5A, "u8"), "center.comp.release_s": (0x5B, "tenths"),
    "sub.output_db": (0x60, "outdb"), "sub.mic": (0x64, "u8"), "sub.music": (0x66, "u8"), "sub.reverb": (0x68, "u8"), "sub.echo": (0x6A, "u8"), "sub.comp.threshold_db": (0x6C, "offset50"), "sub.comp.ratio": (0x6D, "u8"), "sub.comp.attack_ms": (0x6E, "u8"), "sub.comp.release_s": (0x6F, "tenths"), "reverb.level": (0x74, "u8"), "reverb.decay_ms": (0xC8, "u16"), "reverb.predelay_ms": (0xCA, "u16"), "echo.level": (0x7B, "u8"), "echo.repeat": (0x7C, "u8"), "echo.delay_ms": (0xCC, "u16"),
}


def encode_scalar(kind: str, value: Any) -> int:
    if kind in ("u8", "u16"): return int(round(float(value)))
    if kind == "bool": return 1 if bool(value) else 0
    if kind == "offset81": return int(round(float(value) + 81))
    if kind == "offset50": return int(round(float(value) + 50))
    if kind == "offset7": return int(round(float(value) + 7))
    if kind == "offset12": return int(round(float(value) + 12))
    if kind == "tenths": return int(round(float(value) * 10))
    if kind == "outdb": return int(round(float(value) * 2 + 75))
    raise KeyError(kind)


def patch_name(data: bytearray, name: str, touched: set[int]) -> None:
    try: encoded = name.encode("ascii")
    except UnicodeEncodeError as exc: raise ValueError("K500 preset name must be ASCII") from exc
    if len(encoded) > NAME_VISIBLE_MAX: raise ValueError(f"K500 hardware visible preset name max is {NAME_VISIBLE_MAX} characters")
    original16 = bytes(data[NAME_OFFSET:NAME_OFFSET + NAME_VISIBLE_MAX]); pad = b" " if b" " in original16.rstrip(b"\x00") else b"\x00"; replacement = encoded + pad * (NAME_VISIBLE_MAX - len(encoded))
    for i, b in enumerate(replacement): data[NAME_OFFSET + i] = b; touched.add(NAME_OFFSET + i)


def patch_eq_section(data: bytearray, section: str, changes: Mapping[str, Any], touched: set[int]) -> None:
    if section not in EQ_SECTIONS: raise ValueError(f"Unknown EQ section {section!r}")
    if section.endswith("Alt"): raise ValueError(f"Writes to {section} are forbidden: Alt runtime semantics are not proven")
    base, count = EQ_SECTIONS[section]; bands = changes.get("bands", {})
    iterable = [(str(x.get("index")), x) for x in bands] if isinstance(bands, list) else list(bands.items()) if isinstance(bands, dict) else None
    if iterable is None: raise ValueError(f"eq.{section}.bands must be object or list")
    for idx_key, spec in iterable:
        idx = int(idx_key)
        if idx < 1 or idx > count: raise ValueError(f"eq.{section} band index {idx} out of range 1..{count}")
        off = base + 2 + (idx - 1) * 8
        if "frequency_hz" in spec: put_u16(data, off + 2, int(round(float(spec["frequency_hz"]))), touched)
        if "q" in spec: put_u16(data, off + 4, int(round(float(spec["q"]) * 10)), touched)
        if "gain_db" in spec: put_i16(data, off + 6, int(round(float(spec["gain_db"]) * 10)), touched)
        if "type_raw" in spec: raise ValueError("type_raw changes are intentionally blocked; preserve exact donor typeRaw unless a controlled hardware experiment proves the change")


def patch_crossover(data: bytearray, target: str, spec: Mapping[str, Any], touched: set[int]) -> None:
    if target not in PRIMARY_XO: raise ValueError(f"Unknown crossover target {target!r}")
    cfg = PRIMARY_XO[target]
    for key, scalar_key, footer_delta in (("hpf_hz", "hpf_scalar", 10), ("lpf_hz", "lpf_scalar", 2)):
        if key in spec:
            hz = int(round(float(spec[key]))); put_u16(data, cfg[scalar_key], hz, touched)
            for section in cfg["sections"]: put_u16(data, eq_footer_offset(section) + footer_delta, hz, touched)
    for key, footer_delta, base_raw in (("hpf_code", 8, 0x0400), ("lpf_code", 0, 0x0300)):
        if key in spec:
            code = int(spec[key])
            if code not in XO_CODES: raise ValueError(f"{target}.{key} must be one of {sorted(XO_CODES)}")
            for section in cfg["sections"]: put_u16(data, eq_footer_offset(section) + footer_delta, base_raw | code, touched)


def apply_patch(donor: bytes, spec: Mapping[str, Any]) -> Tuple[bytes, Dict[str, Any]]:
    data = bytearray(donor); touched: set[int] = set()
    if "name" in spec: patch_name(data, str(spec["name"]), touched)
    for key, value in spec.get("scalars", {}).items():
        if key not in SCALAR_PATCHERS: raise ValueError(f"Unsupported scalar key {key!r}")
        offset, kind = SCALAR_PATCHERS[key]; raw = encode_scalar(kind, value); put_u16(data, offset, raw, touched) if kind == "u16" else put_u8(data, offset, raw, touched)
    for section, changes in spec.get("eq", {}).items(): patch_eq_section(data, section, changes, touched)
    for target, changes in spec.get("crossovers", {}).items(): patch_crossover(data, target, changes, touched)
    data[CHECKSUM_OFFSET] = 0; data[CHECKSUM_OFFSET] = (-sum(data)) & 0xFF; touched.add(CHECKSUM_OFFSET); out = bytes(data); errors = validate_bytes(out)
    if errors: raise ValueError("patched preset invalid: " + "; ".join(errors))
    changed = [i for i, (a, b) in enumerate(zip(donor, out)) if a != b]; unexpected = sorted(set(changed) - touched)
    if unexpected: raise AssertionError("unexpected changed offsets: " + ", ".join(f"0x{x:04X}" for x in unexpected))
    return out, {"sourceName": preset_name(donor), "outputName": preset_name(out), "changedByteCount": len(changed), "changedOffsets": [f"0x{x:04X}" for x in changed], "allowedOffsetCount": len(touched), "unexpectedOffsets": [], "checksumOk": checksum_ok(out), "size": len(out)}


def compare_files(a: bytes, b: bytes, source_a: str, source_b: str, out_dir: Path) -> Dict[str, Any]:
    np, plt = import_plot_libs(); out_dir.mkdir(parents=True, exist_ok=True); freqs = np.logspace(math.log10(20), math.log10(20000), 1600); pa = output_paths(a, freqs); pb = output_paths(b, freqs); keys = ["mic_to_main_dry", "music_to_main", "main_vocal_composite", "music_to_sub"]; fig, ax = plt.subplots(figsize=(12, 6.5)); delta_metrics = {}
    for key in keys:
        delta = db20(pb[key]) - db20(pa[key]); ax.semilogx(freqs, delta, label=key); ma = curve_metrics(freqs, pa[key]); mb = curve_metrics(freqs, pb[key]); delta_metrics[key] = {name: mb[name] - ma[name] for name in METRIC_BANDS}
    ax.axhline(0, linewidth=1); ax.set_xlim(20, 20000); ax.set_ylim(-8, 8); ax.set_xlabel("Frequency (Hz)"); ax.set_ylabel("B - A (dB)"); ax.set_title(f"K500 comparative response delta\nA={preset_name(a)}  B={preset_name(b)}"); ax.grid(True, which="both", alpha=0.25); ax.legend(fontsize=8); fig.tight_layout(); plot_path = out_dir / "comparison_delta.png"; fig.savefig(plot_path, dpi=160); plt.close(fig)
    changed = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]; report = {"a": source_a, "b": source_b, "nameA": preset_name(a), "nameB": preset_name(b), "changedByteCount": len(changed), "changedOffsets": [f"0x{x:04X}" for x in changed], "metricDeltaBminusA": delta_metrics, "plot": str(plot_path), "notice": "Comparative engineering model; real K500 hardware listening is authoritative."}; (out_dir / "comparison_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8"); return report


def cmd_validate(args) -> int:
    data = Path(args.file).read_bytes(); errors = validate_bytes(data); result = {"file": args.file, "valid": not errors, "errors": errors, "size": len(data), "checksumModulo": sum(data) & 0xFF if data else None, "name": preset_name(data) if len(data) >= NAME_OFFSET + 1 else None}; print(json.dumps(result, indent=2)); return 0 if not errors else 2


def cmd_inspect(args) -> int:
    report = inspect_report(read_file(args.file), args.file); text = json.dumps(report, indent=2, ensure_ascii=False)
    if args.json: Path(args.json).write_text(text, encoding="utf-8")
    print(text); return 0


def cmd_plot(args) -> int:
    report = plot_report(read_file(args.file), args.file, Path(args.out_dir), args.prefix or ""); print(json.dumps(report["simulation"], indent=2)); return 0


def cmd_compare(args) -> int:
    report = compare_files(read_file(args.a), read_file(args.b), args.a, args.b, Path(args.out_dir)); print(json.dumps(report, indent=2)); return 0


def cmd_patch(args) -> int:
    donor = read_file(args.donor); spec = json.loads(Path(args.spec).read_text(encoding="utf-8")); out, audit = apply_patch(donor, spec); Path(args.output).write_bytes(out); audit_path = Path(args.audit) if args.audit else Path(args.output).with_suffix(".audit.json"); audit_path.write_text(json.dumps(audit, indent=2), encoding="utf-8"); print(json.dumps(audit, indent=2)); return 0


def make_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="K500 preset analysis, simulation and donor-based surgical patch lab"); sub = p.add_subparsers(dest="cmd", required=True)
    x = sub.add_parser("validate", help="validate size/checksum/name"); x.add_argument("file"); x.set_defaults(func=cmd_validate)
    x = sub.add_parser("inspect", help="decode proven fields to JSON"); x.add_argument("file"); x.add_argument("--json"); x.set_defaults(func=cmd_inspect)
    x = sub.add_parser("plot", help="generate comparative response plots + CSV + JSON"); x.add_argument("file"); x.add_argument("--out-dir", required=True); x.add_argument("--prefix"); x.set_defaults(func=cmd_plot)
    x = sub.add_parser("compare", help="compare two presets and graph B-A response deltas"); x.add_argument("a"); x.add_argument("b"); x.add_argument("--out-dir", required=True); x.set_defaults(func=cmd_compare)
    x = sub.add_parser("patch", help="create a new preset by surgical patching of a valid donor"); x.add_argument("donor"); x.add_argument("spec"); x.add_argument("output"); x.add_argument("--audit"); x.set_defaults(func=cmd_patch)
    return p


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = make_parser().parse_args(argv)
    try: return int(args.func(args))
    except (ValueError, KeyError, AssertionError, OSError, json.JSONDecodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr); return 2


if __name__ == "__main__": raise SystemExit(main())
