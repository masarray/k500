#!/usr/bin/env python3
"""Fast, deterministic repository contracts for SonKuPik K500 CI.

This intentionally checks only invariants that are not better proven by compiled
self-tests. It replaces dozens of per-feature GitHub workflows with one
cross-platform contract suite.
"""
from __future__ import annotations

import hashlib
import json
import shutil
import tempfile
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FAILURES: list[str] = []
PASSES: list[str] = []


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        FAILURES.append(f"missing file: {rel}")
        return ""
    return path.read_text(encoding="utf-8")


def require(rel: str, *tokens: str) -> None:
    data = read(rel)
    missing = [token for token in tokens if token not in data]
    if missing:
        FAILURES.append(f"{rel}: missing {missing}")
    else:
        PASSES.append(f"{rel}: required contracts")


def forbid(rel: str, *tokens: str) -> None:
    data = read(rel)
    present = [token for token in tokens if token in data]
    if present:
        FAILURES.append(f"{rel}: forbidden {present}")
    else:
        PASSES.append(f"{rel}: forbidden-pattern guard")


def require_count(rel: str, token: str, minimum: int) -> None:
    count = read(rel).count(token)
    if count < minimum:
        FAILURES.append(f"{rel}: expected >= {minimum} occurrences of {token!r}, found {count}")
    else:
        PASSES.append(f"{rel}: {token!r} x{count}")


def require_exact_count(rel: str, token: str, expected: int) -> None:
    count = read(rel).count(token)
    if count != expected:
        FAILURES.append(f"{rel}: expected exactly {expected} occurrences of {token!r}, found {count}")
    else:
        PASSES.append(f"{rel}: {token!r} exactly x{count}")


def check_presets() -> None:
    expected = {
        "01_KONSER_NYANYI.k500": ("KONSER NYANYI", "66ba788daadf56212e4de6673a702404cfc915dc934e98fe3b479ee972f978a1"),
        "02_MC_HOST_RADIO.k500": ("MC HOST RADIO", "adf3c868cfa471b4b0975bb58ffb4e8c05c071a7ed0896fc0a89b72933a88fa7"),
        "03_KAR_DANGDUT.k500": ("KAR DANGDUT", "ec683042962c635d8d87d262512a694797bd62842c07775e53eb497f34d329bc"),
        "04_POP_ROCK_BALLAD.k500": ("POP ROCK BALLAD", "bf8dbbdb0f8f79cca5a299f9d1c824bfa2f4c21facae9b03564ba1cbf2998e2c"),
        "05_POP_KENANGAN_V2.k500": ("POP KENANGAN V2", "e0d6e985f068576a37ea776c1ed730b63bafac44d3afc80ab31f8541ff6398b5"),
        "06_SHOLAWAT_SYAHDU.k500": ("SHOLAWAT SYAHDU", "9a4377cba86880ab18d0caf2933fe054749782355771eb221f396510bd7eb5cb"),
        "07_JAZZ_LOUNGE.k500": ("JAZZ LOUNGE", "a61a84c62865b66c51fb2baef857611cfeb34be0e3e62adb6d88bce7df25dbfb"),
        "08_BLUES_CLUB.k500": ("BLUES CLUB", "ca15409e5c5dddfcf8678c5ec225495d76f3331421410fc71944208c1982c060"),
        "09_ACOUSTIC_NATURAL.k500": ("ACOUSTIC NATURAL", "785c814bfc2bb3d6df7d03ca036dbd39bf36ce36a4acae13a095c5aeeae5c37b"),
        "10_REGGAE_DUB.k500": ("REGGAE DUB", "e0eb3825aec6ce17fff804f7c6c2b0575e0f4fd3d9c844cb3052a965c2c98e1f"),
    }
    root = ROOT / "resources" / "presets"
    files = sorted(root.glob("*.k500"))
    if len(files) != 10:
        FAILURES.append(f"preset library: expected 10 files, found {len(files)}")
        return
    actual_names = {p.name for p in files}
    if actual_names != set(expected):
        FAILURES.append(f"preset library: file set mismatch: {sorted(actual_names)}")
        return

    for path in files:
        raw = path.read_bytes()
        name, expected_hash = expected[path.name]
        if len(raw) != 0x478:
            FAILURES.append(f"{path.name}: expected 1144 bytes, got {len(raw)}")
            continue
        if sum(raw) & 0xFF:
            FAILURES.append(f"{path.name}: additive checksum invalid")
        embedded = raw[0x454:0x475].split(b"\0", 1)[0].decode("ascii", errors="strict").strip()
        if embedded != name:
            FAILURES.append(f"{path.name}: embedded name {embedded!r} != {name!r}")
        if len(embedded) > 16:
            FAILURES.append(f"{path.name}: hardware name exceeds 16 chars")
        digest = hashlib.sha256(raw).hexdigest()
        if digest != expected_hash:
            FAILURES.append(f"{path.name}: sha256 {digest} != {expected_hash}")
    PASSES.append("official preset bank: size/checksum/name/hash")



def check_uaudio_companions() -> None:
    """Build all ten derived .k500 files and guard the exact donor-only mutation."""
    require(
        "CMakeLists.txt",
        "UAUDIO_COMPANION_PRESET_BANK_V1",
        "tools/generate_uaudio_presets.py",
        "SONKUPIK_UAUDIO_PRESET_FILES",
        "list(APPEND SONKUPIK_PRESET_FILES",
    )
    require(
        "src/k500/K500PresetFileBridge.cpp",
        "UAUDIO_COMPANION_PRESET_BANK_V1",
        ":/presets/11_KONSER_NYANYI_UAUDIO.k500",
        ":/presets/20_REGGAE_DUB_UAUDIO.k500",
    )
    try:
        with tempfile.TemporaryDirectory(prefix="k500-uaudio-") as temp:
            out = Path(temp)
            completed = subprocess.run(
                [sys.executable, str(ROOT / "tools/generate_uaudio_presets.py"),
                 "--source", str(ROOT / "resources/presets"), "--output", str(out)],
                capture_output=True, text=True, timeout=30, check=False
            )
            if completed.returncode != 0:
                FAILURES.append("UAUDIO companions generation failed: " + completed.stderr[-1200:])
                return
            manifest = json.loads((out / "uaudio-manifest.json").read_text(encoding="utf-8"))
            if len(manifest) != 10 or len(list(out.glob("*.k500"))) != 10:
                FAILURES.append("UAUDIO companions: expected exactly 10 generated presets")
                return
            for index, entry in enumerate(manifest, 11):
                path = out / entry["file"]
                donor = ROOT / "resources/presets" / entry["donor"]
                generated = path.read_bytes()
                original = donor.read_bytes()
                if not path.name.startswith(f"{index:02d}_") or len(generated) != 0x478:
                    FAILURES.append(f"UAUDIO companions: incorrect output {path.name}")
                    continue
                if generated[0x0E] != 5 or sum(generated) & 0xFF:
                    FAILURES.append(f"UAUDIO companions: source or checksum invalid for {path.name}")
                if entry["donorSha256"] != hashlib.sha256(original).hexdigest():
                    FAILURES.append(f"UAUDIO companions: donor provenance mismatch for {path.name}")
                allowed = set(range(0x454, 0x464)) | {0x0E, 0x475}
                changed = {i for i, (a, b) in enumerate(zip(original, generated)) if a != b}
                if not changed <= allowed:
                    FAILURES.append(f"UAUDIO companions: illegal sonic edits in {path.name}")
            PASSES.append("UAUDIO companions: 10 verified donor-derived native presets")
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as exc:
        FAILURES.append("UAUDIO companions validation: " + str(exc))



def check_fbe_mapping_evidence() -> None:
    expected = {
        "FBE0.k500": (0, "ffeb1e3968b4e0dcf5437442d52fd948eeee91dfdfd6552ac3a0e31a05d9cf7a"),
        "FBE1.k500": (1, "fe57c12e5f068e3b3c4d665a8874d78b46c584678d4306718ddae48257341d7a"),
        "FBE2.k500": (2, "840cbc27595c4b1ee3a7f1a86beb94a1329f055a435799506a640ce068328f05"),
        "FBE3.k500": (3, "9742ec33cc510fd68092bc0ac7915a9c27db82778548bb41275f81a7d076e9d5"),
        "FBE4.k500": (4, "271cca43e0ff9e1e312c068eb0c528bb59d00137bdc39a16060751c1385db7fb"),
    }
    root = ROOT / "tests" / "fixtures" / "fbe"
    captures: list[bytes] = []
    for name, (level, expected_hash) in expected.items():
        path = root / name
        if not path.is_file():
            FAILURES.append(f"FBE evidence missing: {path.relative_to(ROOT)}")
            continue
        raw = path.read_bytes()
        captures.append(raw)
        if len(raw) != 0x478:
            FAILURES.append(f"{name}: expected 1144 bytes, got {len(raw)}")
            continue
        if sum(raw) & 0xFF:
            FAILURES.append(f"{name}: additive checksum invalid")
        if raw[0x0023] != level:
            FAILURES.append(f"{name}: file[0x0023]={raw[0x0023]} != FBE level {level}")
        if raw[0x001B] != 7 or raw[0x001C] != 7:
            FAILURES.append(f"{name}: Mic HP/LP capture bytes changed from 7/7")
        digest = hashlib.sha256(raw).hexdigest()
        if digest != expected_hash:
            FAILURES.append(f"{name}: sha256 {digest} != immutable evidence {expected_hash}")

    if len(captures) == 5:
        for level in range(1, 5):
            changed = {
                offset
                for offset, (before, after) in enumerate(zip(captures[level - 1], captures[level]))
                if before != after
            }
            if changed != {0x0023, 0x0475}:
                FAILURES.append(f"FBE{level - 1}->FBE{level}: unexpected diff offsets {sorted(changed)}")
    PASSES.append("FBE physical evidence: immutable hash/checksum/mapping")

def check_protocol_and_state() -> None:
    require(
        "src/k500/K500Protocol.h",
        "ActiveMemorySize = 0x03AB",
        "ActiveMemoryBlockSize = 0x003A",
        "MusicNoiseGate = 0x0005",
        "MicHpType = 0x0013",
        "MicLpType = 0x0014",
        "MusicInput1Gain = 0x0016",
        "MainHpType = 0x002C",
        "SurroundHpType = 0x0040",
        "CenterHpType = 0x0054",
        "SubHpType = 0x0068",
        "AdjMannerVrOff = 0x008C",
        "MainLDelay = 0x00CB",
        "SubDelay = 0x00D5",
    )
    require(
        "src/k500/K500Protocol.cpp",
        'case 0x05: return prefix + QStringLiteral("Bessel 24")',
        'case 0x06: return prefix + QStringLiteral("Butter 24")',
    )
    require(
        "src/k500/K500Protocol.cpp",
        "ADJ_MANNER_VR_OFF_CMD07_CAPTURED_V2",
        "enabled ? 0x01 : 0x00, 0x03",
        "Adj Manner VR OFF unticked capture V2",
        "Adj Manner VR OFF ticked capture V2",
    )
    forbid(
        "src/k500/K500ResponseParser.h",
        "tryDecodeAdjMannerVrOff",
    )
    forbid(
        "src/k500/K500ResponseParser.cpp",
        "tryDecodeAdjMannerVrOff",
        "response.data.at(19)",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "ADJ_MANNER_ACK_SESSION_AUTHORITY_V1",
        "response.rsp == 0xF8",
        "m_adjMannerVrOffKnown = true",
        "adjMannerVrOffAccepted(m_adjMannerVrOff)",
        "finishOperation(QStringLiteral(\"Adj Manner VR OFF\"))",
    )
    forbid(
        "src/k500/K500PresetManager.h",
        "AwaitAdjMannerVerify",
        "sendAdjMannerVerification",
        "finishOperationRejected",
    )
    forbid(
        "src/k500/K500PresetManager.cpp",
        "AdjMannerVerifyOffset",
        "AdjMannerVerifyIndex",
        "AwaitAdjMannerVerify",
        "sendAdjMannerVerification",
        "Adj Manner verify 0x0074",
        "ReadbackPurpose::AdjManner",
        "startReadback(ReadbackPurpose::AdjManner)",
        "939-byte ownership resync complete",
        "m_activeMemory[K500Protocol::ReadbackOffset::AdjMannerVrOff]",
    )
    require(
        "docs/K500_SYSTEM_CONTROLS_CAPTURE_MAP.md",
        "97c880d5caec84aaa760d552f852b64f3305b8c01a4be00fc28acf4b321ca3ba",
        "9a66ba4b54c767a594b0c4eea351b15b145a662de89056a07452fd98815e9e87",
        "64f5baa97db41cacc526e0c8d78be8b5b631462222f8c5b9fc31a85d97831839",
        "5cb8e2ee8a6f35de66aa5cbaeb95ec9cbd30bb1c87343a762d552fbbeab604db",
        "ebaf3be207ac66529acf53f355a17d6df8fd70dc4b76a1d1f5bc50108edb5bc9",
        "Do **not** decode VR OFF from C0 data[19]",
        "immediate CMD `0x40` verification read",
    )
    require(
        "docs/PROTOCOL_GOLDEN_VECTORS.md",
        "AA 03 00 07 00 03 F3",
        "AA 03 00 07 01 03 F2",
        "activeMemory[0x008C]",
        "RSP 0xF8",
        "does not issue an immediate CMD `0x40` read-after-write",
        "C0 `data[19]` is",
    )
    require(
        "docs/K500_CAPTURE_TODO_MAP.md",
        "C0 data[19] non-authoritative",
        "no immediate read-after-write",
    )
    require(
        "docs/PORTING_PARITY_MATRIX.md",
        "session WRITE authority",
        "connect/reconnect READ authority",
        "no immediate CMD 0x40 verify",
    )
    require(
        "docs/K500_NATIVE_VALUE_RANGES.md",
        "C0 `data[19]` is explicitly non-authoritative",
        "valid RSP 0xF8 commits current-session state",
    )
    require(
        "docs/K500_CAPTURED_RUNTIME_STATUS.md",
        "C0 `data[19]` is **not** an authority",
        "does not issue an immediate",
        "cached 939-byte snapshot is never",
    )
    require(
        "docs/HARDWARE_ACCEPTANCE_CHECKLIST.md",
        "AA 03 00 07 00 03 F3",
        "AA 03 00 07 01 03 F2",
        "C0 data[19] is ignored",
        "no immediate CMD 0x40 read-after-write",
    )
    require(
        "CHANGELOG.md",
        "valid `RSP 0xF8` commits current-session state",
        "C0 `data[19]` is explicitly non-authoritative",
        "Removed the non-native post-ACK read-after-write path",
    )
    forbid(
        "CHANGELOG.md",
        "inverse C0 handshake bit",
        "post-ACK 939-byte ownership resync",
    )
    require(
        "AGENTS.md",
        "Disconnect is a **presentation/editor handoff**",
        "Treating “unknown/offline” as boolean `false` is a regression.",
    )
    require(
        "CHANGELOG.md",
        "Fixed disconnect handoff for Use Init Volume and VR OFF",
        "reconnect readback remains authoritative",
    )
    forbid(
        "docs/PROTOCOL_GOLDEN_VECTORS.md",
        "AA 03 00 07 00 00 F6",
        "AA 03 00 07 01 00 F5",
        "No reconnect/readback bit is proven",
    )
    require(
        "src/k500/K500DeviceManager.cpp",
        "K500Protocol::ActiveMemorySize",
        "K500Protocol::ActiveMemoryBlockSize",
    )
    require(
        "src/k500/K500DeviceManager.cpp",
        "PLAYER_STATUS_E3_RUNTIME_AUTHORITY_V2",
        "PLAYER_STATUS_RESPONSIVE_POLL_V1",
        "response.rsp == 0xE3",
        "Player status connect refresh",
        "Player status poll",
        "PlayerStatusIntervalMs = 400",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "K500Protocol::ActiveMemorySize",
        "K500Protocol::ActiveMemoryBlockSize",
    )
    forbid(
        "src/k500/K500DeviceManager.cpp",
        "constexpr int ActiveMemorySize = 0x03AB",
        "constexpr int ActiveMemoryBlockSize = 0x003A",
    )
    forbid(
        "src/k500/K500PresetManager.cpp",
        "constexpr int ActiveMemorySize = 0x03AB",
        "constexpr int ActiveMemoryBlockSize = 0x003A",
    )
    require(
        "src/k500/K500Controller.cpp",
        "DANCE_MIC_READBACK_REOPENED_BY_ADJ_MANNER_20261004_V1",
        "m_danceMicSeedKnown = false",
        "system.adjMannerVrOff",
        "qRound(m_music.input1GainDb + 12.0)",
    )
    forbid(
        "src/k500/K500Controller.cpp",
        "fileU16(memory, 0x0034, 0)",
        "fileU16(memory, 0x0036, 0)",
        "fileU16(memory, 0x005C, 0)",
        "fileU16(memory, 0x0070, 0)",
        "fileU8(memory, 0x0094, 0xFF)",
    )
    require(
        "src/StudioEngine.cpp",
        "fileU16(memory, 0x00D4)",
        "fileU16(memory, 0x00D6)",
        "fileU16(memory, 0x00DC)",
        "fileU16(memory, 0x00DE)",
        "adjMannerVrOffKnown",
        "manualVrEnabled",
    )
    require(
        "src/k500/K500PresetEditMapper.cpp",
        "b.addU16(0x00D4",
        "b.addU16(0x00D6",
        "b.addU16(0x00DC",
        "b.addU16(0x00DE",
        "no native HP/LP filter-type control",
    )


def check_ui_contracts() -> None:
    require(
        "qml/components/RackFaderPanel.qml",
        "LIVE_RACK_STABLE_DELEGATE_V1",
        "readonly property var modelData: root.channels[index] || ({})",
        "modelData.accentColor",
        "DEFERRED_AUTHORITATIVE_SYNC_V1",
        "property bool deferredModelSync: false",
        "onDraggingChanged:",
        "OUTPUT_MUTE_SEMANTIC_LR_V2",
    )
    require_count("qml/components/RackFaderPanel.qml", "property bool deferredModelSync: false", 2)
    require_count("qml/components/RackFaderPanel.qml", "onDraggingChanged:", 2)
    require_count("qml/components/RackFaderPanel.qml", "model: root.channels ? root.channels.length : 0", 2)
    forbid(
        "qml/components/RackFaderPanel.qml",
        "function muteLiveLabel(label)",
        "return \"R\"",
        "return \"L\"",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_LIVE_STABLE_DELEGATE_V1",
        "root.recordingChannels.length",
        "root.micTriggerChannels.length",
        "SYSTEM_DEFERRED_AUTHORITATIVE_SYNC_V1",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_TOGGLE_DUAL_AUTHORITY_V2",
        "SYSTEM_CALM_TOGGLE_COPY_V1",
        "SYSTEM_DEVICE_MODE_INIT_TOGGLE_V4",
        "SYSTEM_MANUAL_ADJUSTMENT_CARD_V2",
        "SystemToggleRow {",
        "offlineUseInitVolume",
        "offlineAdjMannerVrOff",
        "requestUseInitVolumeToggle",
        "requestAdjMannerVrToggle",
        "setOfflineAdjMannerVrOff",
        "\"SYNCING…\"",
    )
    forbid(
        "qml/components/SystemWorkspaceImpl.qml",
        "\"LOCAL EDIT\"",
        "\"FILE EDIT\"",
        "\"SOFTWARE CONTROL\"",
        "\"FRONT VR ACTIVE\"",
        "\"LIVE · CMD 4E\"",
        "\"BT CAPTURED\"",
        "\"READ 19 · WRITE 8\"",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        'helpText:"Startup volume · Device-only setting"',
        '"Saved with preset · Disable front-panel adjustment"',
        "\"Rename: 1–8 ASCII characters. BLE name is read-only.\"",
    )
    require(
        "src/k500/K500PresetManager.h",
        "Q_PROPERTY(bool useInitBusy",
        "Q_PROPERTY(bool adjMannerBusy",
        "bool useInitBusy() const",
        "bool adjMannerBusy() const",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "root.presetManager.useInitBusy",
        "root.presetManager.adjMannerBusy",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_OFFLINE_HANDOFF_V1",
        "SYSTEM_LATE_LOAD_OFFLINE_SEED_V1",
        "mirrorAcceptedDeviceTogglesToOffline",
        "seedOfflineTogglesFromRetainedSession",
        "function onUseInitVolumeChanged()",
        "function onAdjMannerVrOffChanged()",
        "root.presetManager.useInitVolumeKnown",
        "root.presetManager.adjMannerVrOffKnown",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "OFFLINE_DEVICE_TRUTH_INVALIDATION_V2",
        "m_useInitVolumeKnown = false",
        "m_adjMannerVrOffKnown = false",
        "interruptedOperation == Operation::UseInit",
        "interruptedOperation == Operation::AdjManner",
    )
    forbid(
        "src/k500/K500PresetManager.cpp",
        "m_useInitVolume = false;",
        "m_adjMannerVrOff = false;",
    )
    require(
        "src/k500/K500DeviceManager.h",
        "ADJ_MANNER_ACK_SEMANTIC_FANOUT_V2",
        "adjMannerVrOffAccepted(bool enabled)",
    )
    require(
        "src/main.cpp",
        "K500DeviceManager::adjMannerVrOffAccepted",
        "StudioEngine::syncAdjMannerVrOff",
    )
    forbid(
        "src/k500/K500PresetManager.cpp",
        "emit m_manager->activeMemoryReady",
    )
    require(
        "src/k500/K500PresetEditMapper.cpp",
        "ADJ_MANNER_VR_OFF_FILE_EDIT_V1",
        "system.adjMannerVrOff",
        "K500FieldContract::Field::AdjMannerVrOff.fileOffset",
    )
    require(
        "src/k500/K500PresetFileBridge.h",
        "setOfflineAdjMannerVrOff",
        "persistMappedEdit",
    )
    require(
        "src/StudioEngine.h",
        "ADJ_MANNER_SEMANTIC_SYNC_V2",
        "syncAdjMannerVrOff",
    )
    require(
        "src/StudioEngine.cpp",
        "ADJ_MANNER_SEMANTIC_SYNC_V2",
        "manualVrEnabled",
        "emit deviceStateChanged()",
    )
    require(
        "qml/components/SystemToggleRow.qml",
        "SYSTEM_SETTING_TOGGLE_ROW_V2",
        "signal toggleRequested(bool checked)",
        "root.pending || root.statusText.length > 0",
        "signal blockedClicked()",
        "root.toggleRequested(!root.checked)",
        "root.blockedClicked()",
    )
    require("CMakeLists.txt", "qml/components/SystemToggleRow.qml")
    require_count("qml/components/SystemWorkspaceImpl.qml", "property bool deferredModelSync: false", 2)
    require_count("qml/components/SystemWorkspaceImpl.qml", "onDraggingChanged:", 2)
    require(
        "qml/components/MusicTonePanel.qml",
        "manualVrActive",
        "Theme.green",
    )
    require_count("qml/components/MusicTonePanel.qml", "editable:!root.manualVrActive", 4)
    require(
        "qml/components/SectionWorkspace.qml",
        "manualVrActive",
        "FX_NATIVE_FREQUENCY_ONLY_V1",
    )
    require_count("qml/components/SectionWorkspace.qml", "editable:!root.manualVrActive", 6)
    require_count("qml/components/SectionWorkspace.qml", "showTypes: false", 2)

    # CRASH_SAFE_FIXED_EQ_PAGES_V1 / P5_LAZY_SYSTEM_WORKSPACE_V1 — syntax lint
    # cannot prove object lifetime. Keep the small structural invariants that
    # prevent the historical 10/7/5-band graph hot-swap heap-corruption class.
    require(
        "qml/components/SystemWorkspace.qml",
        "P5_LAZY_SYSTEM_WORKSPACE_V1",
        "Loader",
        "asynchronous: true",
        "SystemWorkspaceImpl",
        "loadRequested",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "P2_DEVICE_PRESET_UI_V1",
        "MassUploadTransferWindow",
    )
    require("CMakeLists.txt", "qml/components/SystemWorkspaceImpl.qml")
    require(
        "qml/components/SectionEqGraphHost.qml",
        "CRASH_SAFE_FIXED_EQ_PAGES_V1",
    )
    require_exact_count("qml/components/SectionEqGraphHost.qml", "SectionEqGraph {", 8)
    forbid("qml/components/SectionEqGraphHost.qml", "Loader {")

    # EQ_BYPASS_RED_WARNING_V1: red is reserved for the bypassed/on state.
    require(
        "qml/components/SectionEqGraph.qml",
        "EQ_BYPASS_RED_WARNING_V1",
        'color:root.eqBypassActive?"#381B23"',
        'border.color:root.eqBypassActive?"#F0727A"',
        'color:root.eqBypassActive?"#FF9AA0":Theme.textSoft',
        'onClicked:root.setEqBypass(!root.eqBypassActive)',
    )

    # MASS_UPLOAD_EXTENDED_SELECTION_V1: modifiers work before staging; device
    # upload cardinality and descending 10->1 hardware order remain unchanged.
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "MASS_UPLOAD_EXTENDED_SELECTION_V1",
        "Qt.ControlModifier",
        "Qt.ShiftModifier",
        "selectedSourceIndexes = next",
        "var selected = selectedSourceIndexes.slice()",
        "var direction = index >= sourceAnchor ? 1 : -1",
        "for (var i = sourceAnchor; ; i += direction)",
        "selectionPosition(index)",
        'selected ? "#" + String(root.selectionPosition(index)).padStart(2, "0")',
        "targetContains(path)",
        "targetModel.count < maxSlots",
        "root.selectSource(index, mouse.modifiers)",
        "root.selectedSourceIndexes.length > 0",
    )
    forbid(
        "qml/components/MassUploadTransferWindow.qml",
        "onClicked: root.sourceIndex = index",
        "selectedSourceIndexes.slice().sort",
        "next.sort(function(a, b)",
        'text: validPreset ? "READY" : "INVALID"',
    )

    # SELECTION_CHRONOLOGY_BEHAVIOR_TEST_V1 — run the actual QML functions in
    # a no-GUI JS VM, covering Blues -> Pop Rock, reverse/forward Shift,
    # Ctrl re-pick ordering, visible rank, duplicate prevention and 10 slots.
    # Node is available on GitHub-hosted CI; never needed in the shipped EXE.
    node = shutil.which("node")
    if node is None:
        FAILURES.append("Mass Upload selection regression needs Node.js in CI")
    else:
        try:
            js_test = subprocess.run(
                [node, str(ROOT / "tools/ci/test_mass_selection_order.js")],
                text=True, capture_output=True, timeout=12, check=False,
            )
            if js_test.returncode:
                FAILURES.append(
                    "Mass Upload real-QML selection test failed: "
                    + (js_test.stderr or js_test.stdout)[-1800:]
                )
            else:
                PASSES.append(js_test.stdout.strip())
        except subprocess.TimeoutExpired:
            FAILURES.append("Mass Upload real-QML selection regression timed out")

    # UX titles are section-specific, not generic or copy-pasted to wrong DSP.
    require(
        "qml/components/SectionWorkspace.qml",
        'title: "Mic HPF & LPF"',
        'title: "Reverb HPF & LPF"',
        'title: "Echo HPF & LPF"',
        'title: "X-OVER & SPK DELAY"',
    )
    if read("qml/components/SectionWorkspace.qml").count(
        'title: "X-OVER & SPK DELAY"'
    ) != 4:
        FAILURES.append("all four output sections must say X-OVER & SPK DELAY")
    require("qml/components/FilterPanel.qml", 'text: "MUSIC HPF & LPF"')
    require("qml/components/MasterStripPanel.qml", 'text: "MASTER VOLUME"')
    require("qml/components/MusicTonePanel.qml", 'text: "PITCH SHIFTER (NADA MUSIK)"')
    forbid("qml/components/SectionWorkspace.qml", '"Band Limits / Delay"')


    # PRE_RC_UX_STABILIZATION_V1 — canonical PC catalog IDs never derive
    # from ListView order, and cannot silently change physical 01..10 slots.
    require(
        "src/k500/K500PresetFileBridge.cpp",
        "OFFICIAL_PRESET_CANONICAL_CATALOG_ID_V1",
        "fileName.left(2).toInt(&catalogNumberOk)",
        'entry.insert(QStringLiteral("catalogNumber"), catalogNumber)',
        'entry.insert(QStringLiteral("originLabel"), QStringLiteral("LOCAL"))',
    )
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "MASS_UPLOAD_CANONICAL_BADGE_V1",
        "modelData.catalogNumber",
        'String(catalogNumber).padStart(2, "0")',
        'origin === "LOCAL" ? "LOCAL" : "OFFICIAL"',
        "root.isSourceSelected(index)",
        "targetModel.count < maxSlots",
    )
    forbid(
        "qml/components/MassUploadTransferWindow.qml",
        "\\n                                                    text: origin\\n",
        "text: String(index + 1).padStart(2, \"0\") + \" PRESET\"",
    )

    # Off-line default is a constructor-only model state. No delayed QML
    # setter may clobber a selected file or verified Connect/Recall readback.
    require(
        "src/StudioEngine.cpp",
        "OFFLINE_CROSSOVER_SINGLE_AUTHORITY_V1",
        "m_musicEqBands.configure",
        "m_subEqBands.configure",
    )
    constructor = read("src/StudioEngine.cpp").split(
        "StudioEngine::StudioEngine(QObject *parent)", 1
    )[-1].split("    connectEqModel(", 1)[0]
    if constructor.count('QStringLiteral("Bypass"), QStringLiteral("Bypass")') != 9:
        FAILURES.append("StudioEngine: expected exactly 9 no-edit Bypass model initializations")
    else:
        PASSES.append("offline HP/LP authority: 9 constructor-only Bypass models")
    require(
        "src/StudioEngine.h",
        'QString m_hpType = QStringLiteral("Bypass")',
        'QString m_lpType = QStringLiteral("Bypass")',
    )
    require(
        "src/main.cpp",
        "OFFLINE_CROSSOVER_ENGINE_TEST_V1",
        "OFFLINE_CROSSOVER_HANDOFF_TEST_V1",
        "studioEngine.clearDeviceState()",
    )
    forbid("qml/components/SectionEqGraph.qml", "ensureStartupCrossoverDefaults", "startupCrossoverPrimed")
    forbid("qml/components/SectionEqGraphHost.qml", "restoreConfiguredCrossoverTypes", "setHpType(", "setLpType(")
    forbid("qml/components/EqGraph.qml", "Component.onCompleted: Qt.callLater", 'root.engine.hpType = "HP Butter 12"')
    require(
        "qml/components/SectionEqGraph.qml",
        "EQ_TOOLBAR_GEOMETRY_PARITY_V1",
        'Layout.preferredHeight:28;text:"Mic A"',
        'Layout.preferredHeight:28;text:"Mic B"',
        "Layout.preferredWidth:104\n                    Layout.preferredHeight:28",
        "Layout.preferredWidth:31\n                            Layout.preferredHeight:16",
        'color:root.eqBypassActive?"#381B23"',
    )

    # LAST_VERIFIED_DEVICE_SLOT_SNAPSHOT_V1 — the view may retain last
    # successfully read 10-slot identities after Disconnect, but never mark
    # them ACTIVE or let offline file Preview impersonate a hardware readback.
    require(
        "src/StudioEngine.h",
        "retainedDeviceModeNames",
        "retainedDeviceState",
        "void hydrateFromDeviceMemory(const QByteArray &memory)",
        "void hydrateFromPreviewMemory(const QByteArray &memory)",
    )
    require(
        "src/StudioEngine.cpp",
        "LAST_VERIFIED_DEVICE_SLOT_SNAPSHOT_V1",
        "if (hardwareReadback)",
        "m_retainedDeviceModeNames = modeNames",
        "m_retainedDeviceState = m_deviceState",
    )
    require(
        "src/k500/K500PresetFileBridge.cpp",
        "OFFLINE_PREVIEW_NOT_HARDWARE_V1",
        "hydrateFromPreviewMemory(preview)",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "OFFLINE_LAST_KNOWN_DEVICE_SLOT_V1",
        "m_lastKnownSlot = slot",
        "emit lastKnownSlotChanged()",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "DEVICE_SLOT_OFFLINE_SHADOW_V1",
        "root.engine.retainedDeviceModeNames",
        "root.engine.retainedDeviceState",
        "root.deviceConnected && index===root.activeDeviceSlot",
        "root.presetManager.lastKnownSlot",
    )
    require(
        "src/main.cpp",
        "RETAIN_VERIFIED_DEVICE_MODE_NAMES_TEST_V1",
        "OFFLINE_PREVIEW_CANNOT_CLOBBER_DEVICE_HISTORY_V1",
    )

    # MASS_UPLOAD_TEN_SLOT_SKELETON_V3 — inert 10-slot plan, not a
    # second source of truth or rewritten hardware transfer semantics.
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "MASS_UPLOAD_VISUAL_WORKFLOW_V3",
        "MASS_UPLOAD_STAGING_ACTIONS_V3",
        "MASS_UPLOAD_TEN_SLOT_SKELETON_V3",
        "model: root.maxSlots",
        "opacity: index < targetModel.count ? 0 : 1",
        "height: slotCanvas.slotRowHeight",
        "model: targetModel",
        "root.addSelected()",
        "root.addAll()",
        "root.removeSelected()",
        "root.clearTarget()",
        "root.uploadTransfer()",
        "MASS_UPLOAD_QUIET_COPY_V9",
        "MASS_UPLOAD_SELECTION_ORDER_V2",
        "MASS_UPLOAD_ACK_PROGRESS_OVERLAY_V1",
    )

    # FINAL_MASS_UPLOAD_LABEL_V1_1_2 — freeze the explicit control label
    # without changing staging scope, hardware gates, or click action.
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "FINAL_MASS_UPLOAD_LABEL_V1_1_2",
        'text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Mass Upload"',
        "enabled: !!root.fileBridge && (!root.presetManager || !root.presetManager.busy)",
        "onClicked: massPresetDialog.openTransfer()",
    )
    forbid("qml/components/SystemWorkspaceImpl.qml", 'text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Mass"')

    # QUIET_PRESET_COPY_V9 — remove redundant preset prose without ever
    # changing models, chronology, native Store/Recall, or meaningful errors.
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "MASS_UPLOAD_QUIET_COPY_V9",
        'text: "MASS UPLOAD PRESETS"',
        "ToolTip.text: \"Ctrl+click selects presets",
        'text: "No presets. Choose Folder or Sync."',
        'text: displayName',
        "ToolTip.text: originLabel + \" · \" + fileName",
        "FINAL_ADD_DIRECTION_COUNT_V1_1_2",
        'text: root.selectedSourceIndexes.length > 1 ? "Add (" + root.selectedSourceIndexes.length + ") →" : "Add →"; compact: true; enabled: root.selectedSourceIndexes.length > 0 && targetModel.count < root.maxSlots; onClicked: root.addSelected()',
        'text: "Add All →"; compact: true; enabled: sourceList.count > 0 && targetModel.count < root.maxSlots; onClicked: root.addAll()',
        'text: root.presetManager && root.presetManager.storeBusy ? "Uploading…" : "Upload to K500"',
        'root.presetManager.massUploadProgressPercent',
        "root.fileBridge.officialSyncError",
        'String(root.fileBridge.lastError)',
        "Connect K500 via USB to upload",
        "root.uploadTransfer()",
    )
    forbid(
        "qml/components/MassUploadTransferWindow.qml",
        'text: "NOT STAGED"',
        'text: "UPLOAD PLAN ONLY',
        'text: "SLOT " + String(index + 1)',
        "text: String(modelData.description || modelData.fileName",
        "Prepare the list offline;",
        'text: "Stage',
        'text: "Fill slots',
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_PRESET_NAME_ONLY_V9",
        "SYSTEM_QUIET_STATUS_V9",
        "root.pcSaveNotice.length > 0 ? root.pcSaveNotice",
        "String(root.fileBridge.lastError)",
        'text: loadedPreset ? "STAGED" : validPreset ? "" : "INVALID"',
        "root.loadPcLibraryEntry(index)",
        'text:"READ ONLY"',
        'ToolTip.text: "Rename: 1–8 ASCII characters. BLE name is read-only."',
        'text:root.deviceConnected?"":"OFFLINE"',
    )
    forbid(
        "qml/components/SystemWorkspaceImpl.qml",
        'text:"PC ONLY"',
        'text:"DEVICE MANAGED"',
        "Lock and Admin settings are read-only in K500.",
        'text: loadedPreset ? "STAGED" : validPreset ? "SELECT"',
        'text: String(modelData.description || modelData.presetName',
        "Offline preview/edit · verified PEQ/fader",
    )

    # MASS_UPLOAD_ACK_PROGRESS_V1 — progress only on device response ACK,
    # reserve final completion for the successful full Recall/readback. The
    # dialog cannot disappear or abort in the middle of a store transaction.
    require(
        "src/k500/K500PresetManager.h",
        "massUploadProgressPercent",
        "massUploadProgressChanged",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "MASS_UPLOAD_ACK_PROGRESS_V1",
        "m_storeOffset += m_pendingStoreLength",
        "updateMassUploadAcknowledgedProgress()",
        "99 * accepted / totalBytes",
        "setMassUploadProgressPercent(100)",
    )
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "MASS_UPLOAD_ACK_PROGRESS_OVERLAY_V1",
        "verifiedPercent",
        "transferInFlight",
        "onOperationCompleted(kind, slot)",
        "onOperationFailed(kind, message)",
        "onClosing: function(close)",
        "Behavior on width",
        "Easing.OutCubic",
    )

    # LOCAL_PRESET_LEGACY_FOLDER_DISCOVERY_V1: old Documents directory takes
    # precedence over an empty stale remembered folder; no recursive scans.
    require(
        "src/k500/K500PresetFileBridge.cpp",
        "LOCAL_PRESET_LEGACY_FOLDER_DISCOVERY_V1",
        "QStandardPaths::DocumentsLocation",
        "SONKUPIK STUDIO Presets",
        "pcPresetLibrary/folder",
        "rebuildFolderPresets()",
    )
    # Save PC As is an actual file-bridge operation with explicit prerequisite,
    # not an invented full .k500 export from incomplete LIVE device memory.
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SAVE_AS_EXPLICIT_SOURCE_GATE_V1",
        'text: "Save PC As"',
        "root.requestSaveAs()",
        "root.fileBridge.saveFile(selectedFile)",
        "root.pcSaveNotice",
        "Select a preset from the PC bank/local folder",
    )
    # Mic EQ LINK is an immediate, reversible editor invariant. Readback
    # is authoritative, but it NEVER silently re-transmits mic.eqLink.
    require(
        "src/StudioEngine.h",
        "MIC_EQ_LINK_INSTANT_STATE_PARITY_V1",
        "micEqLinkedChanged",
        "setMicEqLinked(bool linked, int sourceChannel = 0)",
    )
    require(
        "src/StudioEngine.cpp",
        "MIC_EQ_LINK_INSTANT_STATE_PARITY_V1",
        "mirrorMicEq(0)",
        "mirrorMicEq(1)",
        "m_micEqHydrating",
        "m_micEqMirrorGuard",
        "syncMicEqLink(linkedFromReadback, 0)",
        "QScopedValueRollback<bool>",
    )
    require(
        "qml/components/SectionWorkspace.qml",
        "root.engine.setMicEqLinked(linked, root.micChannel)",
        "micEqLinkUserRequested",
    )
    require(
        "qml/Main.qml",
        "MIC_EQ_LINK_USER_INTENT_ONLY_V1",
        "onMicEqLinkUserRequested(linked)",
        'root.studioEngine.editDevicePath("mic.eqLink", linked)',
    )
    forbid("qml/Main.qml", "onMicEqLinkedChanged()")
    require("src/main.cpp", "MIC_EQ_LINK_INSTANT_REGRESSION_V1")

    # Presentation may never bypass the native controller/engine boundary.
    for path in (ROOT / "qml").rglob("*.qml"):
        data = path.read_text(encoding="utf-8")
        if "K500Controller" in data or "K500WinIo" in data:
            FAILURES.append(f"{path.relative_to(ROOT)}: direct native I/O reference from QML")
    PASSES.append("QML/native ownership boundary")



def check_public_surface_contracts() -> None:
    # Donation prompt: preserve the accepted 3-second/no-auto-close behavior,
    # official QRIS source, readable hierarchy, and strictly voluntary boundary.
    require(
        "src/DonationPromptController.cpp",
        "support/donationPrompt/v1/observedDays",
        "kRequiredDistinctDays = 3",
        "qrisAvailable()",
        "DONATION_QRIS_FAIL_CLOSED_V1",
        "DONATION_PROMPT_DISTINCT_DAY_V1",
    )
    require("src/DonationPromptController.h", "QSettings")
    forbid(
        "src/DonationPromptController.cpp",
        "StudioEngine",
        "QNetwork",
        "http://",
        "https://",
    )
    require(
        "src/main.cpp",
        "--donation-prompt-preview",
        'QStringLiteral("donationPrompt")',
    )
    require(
        "qml/Main.qml",
        "DONATION_UPDATE_PROMPT_ARBITER_V1",
        "donationPromptDialog.opened",
    )
    require(
        "qml/components/DonationPrompt.qml",
        "countdownSeconds: 3",
        "Popup.NoAutoClose",
        "enabled: root.canAcknowledge",
        "DONATION_READABLE_HIERARCHY_V2",
        "DONATION_TEXT_LINKS_V2",
        "LUCIDE_HEART_FILLED_V1",
        'name: "heart"',
        "filled: true",
        "font.pixelSize: 24",
        "font.pixelSize: 14",
        "labelPixelSize: 13",
        "sourceClipRect: SupportLinks.qrisCrop",
        "Qt.openUrlExternally(SupportLinks.youtubeUrl)",
        "Qt.openUrlExternally(SupportLinks.tokopediaUrl)",
    )
    require_exact_count("qml/components/DonationPrompt.qml", "font.underline: true", 2)
    forbid(
        "qml/components/DonationPrompt.qml",
        "SupportLinks.merchantName",
        "SupportLinks.merchantNmid",
    )
    require(
        "qml/theme/SupportLinks.qml",
        "qrc:/support/qris-sonkupik.png",
        "https://www.youtube.com/@sonkupik",
        "https://www.tokopedia.com/dr-sonkupik/",
        "SONKUPIK, AUDIO DEVELOPER, DIGITAL & KREATIF",
        "ID1026551401775",
    )
    require(
        "CMakeLists.txt",
        "resources/support/qris-sonkupik.png",
        'if(EXISTS "${SONKUPIK_QRIS_FILE}")',
        "k500_donation_prompt_selftest",
    )

    qris = ROOT / "resources" / "support" / "qris-sonkupik.png"
    if not qris.is_file():
        FAILURES.append("QRIS support asset missing")
    else:
        raw = qris.read_bytes()
        if len(raw) <= 100_000 or raw[:8] != b"\x89PNG\r\n\x1a\n":
            FAILURES.append("QRIS support asset is not the expected production PNG")
        elif len(raw) < 24:
            FAILURES.append("QRIS support asset PNG header is truncated")
        else:
            width = int.from_bytes(raw[16:20], "big")
            height = int.from_bytes(raw[20:24], "big")
            if width < 600 or height < 600:
                FAILURES.append(f"QRIS support asset too small: {width}x{height}")
            else:
                PASSES.append(f"QRIS production asset: {width}x{height}")

    # WINDOW_TITLE_RUNTIME_VERSION_V1 — title reports the installed binary,
    # not a hard-coded release label or a separately cached website version.
    require(
        "qml/Main.qml",
        "WINDOW_TITLE_RUNTIME_VERSION_V1",
        'title: "SonKuPik K500 - Karaoke Processor v" + Qt.application.version',
    )
    require("src/AppVersionInit.cpp", "QCoreApplication::setApplicationVersion")
    forbid("qml/Main.qml", 'title: "SonKuPik K500 - Karaoke Processor v1.1.1"')

    # About/support link contract.
    require(
        "qml/components/AboutDialog.qml",
        "ABOUT_FLOATING_CARD_V2",
        "ABOUT_VERSION_BELOW_SUBTITLE_V2",
        "Qt.application.version",
        "Copyright © 2026, SonKuPik",
        "SupportLinks.youtubeUrl",
        "SupportLinks.tokopediaUrl",
        "qrc:/assets/SonKuPik-k500-logo.png",
    )
    require_exact_count("qml/components/AboutDialog.qml", "Qt.openUrlExternally", 2)
    forbid("qml/components/AboutDialog.qml", "font.pixelSize: 8")
    require(
        "qml/components/TopBar.qml",
        "signal aboutRequested()",
        "onClicked:root.aboutRequested()",
    )
    require(
        "qml/Main.qml",
        "AboutDialog {",
        "onAboutRequested: aboutDialog.open()",
    )

    # STARTUP_FRESH_RELEASE_DISCOVERY_V1 — reopening a previously checked
    # installation MUST send a new GitHub Latest request; only a successful
    # request made in the current process may activate the 6-hour throttle.
    require(
        "src/AppUpdateManager.h",
        "STARTUP_FRESH_RELEASE_DISCOVERY_V1",
        "bool m_successfulDiscoveryThisSession = false;",
    )
    require(
        "src/AppUpdateManager.cpp",
        "STARTUP_FRESH_RELEASE_DISCOVERY_V1",
        "!userInitiated && m_candidateTag.isEmpty() && m_successfulDiscoveryThisSession",
        "m_successfulDiscoveryThisSession = true;",
        'QSettings().setValue(QStringLiteral("updates/lastSuccessfulCheckUtc")',
    )
    require("qml/Main.qml",
        "STARTUP_UPDATE_DISCOVERY_V1",
        "interval: 2500",
        "onTriggered: AppUpdater.checkForUpdates(false)",
        "root.maybeOpenUpdateDialog()")
    forbid("src/AppUpdateManager.cpp",
        "if (!userInitiated && m_candidateTag.isEmpty()) {",
    )

    # OFFLINE_NETWORK_BOUNDED_FALLBACK_V1 — loss of connectivity must not
    # crash, block K500 device control, or advertise an update. Manual
    # checks still provide diagnostics, not a silent lie.
    require(
        "src/AppUpdateManager.cpp",
        "OFFLINE_NETWORK_BOUNDED_FALLBACK_V1",
        "request.setTransferTimeout(12000)",
        "void AppUpdateManager::handleDiscoveryFailure(bool userInitiated, const QString &reason)",
        "handleDiscoveryFailure(userInitiated, reason);",
        "HostNotFoundError",
        "TimeoutError",
    )
    require(
        "src/AppUpdateManagerSelfTest.cpp",
        "OFFLINE_NETWORK_BOUNDED_FALLBACK_V1",
        "manager.handleDiscoveryFailure(false",
        "manager.handleDiscoveryFailure(true",
        "offline automatic discovery exposed an error",
    )
    require(
        "src/k500/K500OfficialPresetSync.cpp",
        "OFFLINE_PRESET_CACHE_FALLBACK_V1",
        "request.setTransferTimeout(githubApi ? 12000 : 30000)",
        "Network unavailable · using bundled/cached SonKuPik presets",
        "m_officialSyncNetworkFallback = true;",
        "m_officialSyncNetworkFallback)",
    )
    forbid(
        "src/k500/K500OfficialPresetSync.cpp",
        "QStringLiteral(\"Offline · using bundled/cached SonKuPik presets\"), reason",
        "m_officialSyncError = QStringLiteral(\"%1: %2\").arg(name, reply->errorString())",
    )

    # ABOUT_MANUAL_UPDATE_CHECK_V1 — explicit recovery path for users
    # whose background check is throttled or whose Inno registration is
    # missing/ambiguous. This requests no silent device or installer action.
    require(
        "qml/components/AboutDialog.qml",
        "ABOUT_MANUAL_UPDATE_CHECK_V1",
        "signal checkForUpdatesRequested()",
        'text: "Cek Update"',
        "root.checkForUpdatesRequested()",
    )
    require(
        "qml/Main.qml",
        "onCheckForUpdatesRequested:",
        "AppUpdater.checkForUpdates(true)",
        "updateDialog.open()",
        "onTriggered: AppUpdater.checkForUpdates(false)",
    )

    # Selected native-text / Lucide raster baseline. These are intentionally
    # small optical contracts, not a pixel-perfect theme snapshot.
    require(
        "src/AppVersionInit.cpp",
        "UI_NATIVE_TEXT_RENDERING_V1",
        "QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering)",
    )
    require(
        "qml/components/LucideIcon.qml",
        "LUCIDE_CURVE_AA_V1",
    )
    require_count("qml/components/LucideIcon.qml", "preferredRendererType: Shape.CurveRenderer", 11)
    forbid("qml/components/LucideIcon.qml", "layer.samples")
    require(
        "qml/components/SoftButton.qml",
        "renderType: Text.NativeRendering",
        "font.hintingPreference: Font.PreferFullHinting",
        "root.compact ? 10",
        "LUCIDE_OPTICAL_NORMALIZATION_V1",
        'root.iconName === "usb" ? 1.82',
        "TEXT_NATIVE_PRESS_STABILITY_V1",
        "y: mouse.pressed ? 1 : 0",
    )
    forbid("qml/components/SoftButton.qml", "scale: mouse.pressed")
    require(
        "qml/components/SectionDrawer.qml",
        "MICRO_TYPE_OPTICAL_POLISH_V1",
        "font.pixelSize: 10",
        "y: navPointer.pressed ? 1 : 0",
        "strokeWidth: 1.85",
    )
    # SYSTEM_CLEAN_TOGGLE_COPY_V1_1_2 — persistent helper text moves to
    # context hover, while hardware APPLYING / SYNCING remain visible.
    require(
        "qml/components/SystemToggleRow.qml",
        "SYSTEM_TOGGLE_CONTEXT_HELP_V1_1_2",
        "property string helpText:",
        "ToolTip.visible: interactionMouse.containsMouse && root.helpText.length > 0",
        "ToolTip.text: root.helpText",
        "visible: root.detail.length > 0 || root.pending || root.statusText.length > 0",
        "visible: root.detail.length > 0",
        'root.pending ? "APPLYING…" : root.statusText',
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        'title:"Use Init Volume"',
        'helpText:"Startup volume · Device-only setting"',
        'title:"VR / Trim Pot Off"',
        '"Saved with preset · Disable front-panel adjustment"',
        'onToggleRequested:root.requestUseInitVolumeToggle()',
        'onToggleRequested:root.requestAdjMannerVrToggle()',
        "SYSTEM_CALM_TOGGLE_COPY_V1",
    )
    forbid(
        "qml/components/SystemWorkspaceImpl.qml",
        'title:"Use Init Vol"',
        'detail:root.deviceConnected',
        'detail:!root.deviceConnected && root.offlineEditMode',
    )

    # SUBWOOFER_EMBEDDED_FONT_AA_V8 — audit every shipped QML file in
    # the existing Fast Contracts, without adding expensive global MSAA.
    require(
        "qml/components/SectionDrawer.qml",
        '{name:"Subwoofer", sub:"Bass management", icon:"activity"}',
    )
    require(
        "qml/theme/Theme.qml",
        'fontFamily: "Plus Jakarta Sans"',
        'displayFamily: "Plus Jakarta Sans"',
        'monoFamily: "Plus Jakarta Sans"',
    )
    require(
        "src/main.cpp",
        'const QString kUiFontFamily = QStringLiteral("Plus Jakarta Sans");',
        "QFontDatabase::addApplicationFont(resource)",
        "appFont.setStyleStrategy(QFont::PreferAntialias)",
        "app.setFont(appFont)",
    )
    for weight in ("Regular", "Medium", "SemiBold", "Bold"):
        name = f"PlusJakartaSans-{weight}.ttf"
        require("src/main.cpp", f":/fonts/{name}")
        require("CMakeLists.txt", f"resources/fonts/{name}")
        if not (ROOT / "resources" / "fonts" / name).is_file():
            FAILURES.append(f"Missing embedded Jakarta font: {name}")
    require(
        "src/AppVersionInit.cpp",
        "UI_NATIVE_TEXT_RENDERING_V1",
        "QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering)",
    )
    require(
        "src/EqCurveItem.cpp",
        "P1_NATIVE_PEQ_ANALYTIC_AA_V1",
        "edgeColor.setAlpha(0)",
        "QSGMaterial::Blending",
    )
    require("qml/components/StudioKnob.qml", "CLEAN_NATIVE_GLYPH_AA_V8")
    forbid("qml/components/StudioKnob.qml", "Text.Outline", "Behavior on styleColor")

    allowed_fonts = {"Theme.fontFamily", "Theme.displayFamily", "Theme.monoFamily"}
    fonts_seen = canvases_seen = shapes_seen = 0
    for qml in sorted((ROOT / "qml").rglob("*.qml")):
        qml_text = qml.read_text(encoding="utf-8")
        rel = qml.relative_to(ROOT).as_posix()
        for m in re.finditer(r"\bfont\.family\s*:\s*([^;\n}]+)", qml_text):
            fonts_seen += 1
            if m.group(1).strip() not in allowed_fonts:
                FAILURES.append(f"{rel}: non-embedded font {m.group(1).strip()!r}")
        if "Text.QtRendering" in qml_text or "PreferNoAntialias" in qml_text:
            FAILURES.append(f"{rel}: native antialias rendering disabled")
        for m in re.finditer(r"\bCanvas\s*\{", qml_text):
            canvases_seen += 1
            paint_at = qml_text.find("onPaint:", m.end())
            if paint_at < 0 or not re.search(
                r"\bantialiasing\s*:\s*true\b", qml_text[m.end():paint_at]
            ):
                FAILURES.append(f"{rel}: Canvas stroke without antialiasing")
        for m in re.finditer(r"\bShape\s*\{", qml_text):
            shapes_seen += 1
            if "preferredRendererType: Shape.CurveRenderer" not in qml_text[m.end():m.end() + 512]:
                FAILURES.append(f"{rel}: Shape missing CurveRenderer")
    if fonts_seen < 100 or canvases_seen != 3 or shapes_seen < 13:
        FAILURES.append(
            f"UI font/AA scope shrank: {fonts_seen} fonts, "
            f"{canvases_seen} Canvas, {shapes_seen} Shapes"
        )
    else:
        PASSES.append(
            f"Embedded Plus Jakarta / AA: {fonts_seen} font overrides, "
            f"{canvases_seen} antialiased Canvas, {shapes_seen} curve Shapes"
        )

    # VISUAL_FOUNDATION_REGRESSION_V1 — fast source-only safeguards.
    # Do not add a separate workflow for presentation refinements.
    require(
        "qml/theme/Theme.qml",
        "VISUAL_FOUNDATION_V1",
        "navActiveSurface",
        "navSubtitleSize: 11",
    )
    require(
        "qml/components/SectionDrawer.qml",
        "MICRO_TYPE_OPTICAL_POLISH_V1",
        "y: navPointer.pressed ? 1 : 0",
        "Theme.navActiveSurface",
        "Theme.navSubtitleSize",
    )
    require(
        "qml/components/BandInspector.qml",
        "VISUAL_INSPECTOR_SURFACE_V1",
        "onClicked: root.resetRequested()",
        "signal frequencyEdited(real value)",
    )
    require(
        "qml/components/CrossoverInspector.qml",
        "VISUAL_INSPECTOR_SURFACE_V1",
        "onClicked: root.resetRequested()",
        "signal typeEdited(string value)",
    )
    require(
        "qml/components/SectionEqGraph.qml",
        "VISUAL_BAND_CHIP_HIERARCHY_V1",
        "model:root.bands",
        "onClicked:root.selectBand(index)",
    )

    # RACK_TYPE_HIERARCHY_V1 — source-only checks in the existing single CI.
    require(
        "qml/theme/Theme.qml",
        "RACK_TYPE_HIERARCHY_V1",
        "rackHeaderTracking: 0.75",
        "rackCaptionSize: 9",
        "rackReadoutSize: 10",
        "rackUnitSize: 8",
    )
    require(
        "qml/components/ParameterSlider.qml",
        "RACK_READOUT_OPTICAL_V2",
        "font.pixelSize:Theme.rackCaptionSize",
        "font.pixelSize:Theme.rackReadoutSize",
        "onPositionChanged:function(event){if(pressed)setFromX(event.x)}",
        "signal valueEdited(real newValue)",
    )
    require(
        "qml/components/RackFaderPanel.qml",
        "Theme.rackHeaderTracking",
        "Theme.rackReadoutSize",
        "LIVE_RACK_STABLE_DELEGATE_V1",
        "onDraggingChanged:",
    )
    require(
        "qml/components/RackDynamicsPanel.qml",
        "Theme.rackHeaderTracking",
        "function dispatchLive(field, value)",
        "graph.requestPaint()",
    )
    require(
        "qml/components/RackFilterPanel.qml",
        "Theme.rackCaptionSize",
        "root.editField(root.hpfIndex,v)",
        "root.editField(root.lpfIndex,v)",
    )
    require(
        "qml/components/MasterStripPanel.qml",
        "PERSISTENT_MASTER_UTILITY_V2",
        'text: "GLOBAL"',
        "root.engine.masterMusic = v",
        "root.engine.masterMic = v",
        "root.engine.masterFx = v",
    )
    forbid("qml/components/ParameterSlider.qml", "Text.Outline")
    forbid("qml/components/MasterStripPanel.qml", "Text.Outline")

    # SYSTEM_DASHBOARD_OPTICAL_V2B — source-only regression guards.
    require(
        "qml/theme/Theme.qml",
        "SYSTEM_DASHBOARD_OPTICAL_V2B",
        "systemListTitleSize: 10",
        "systemCaptionSize: 9",
        "systemStatusSize: 9",
    )
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_SLOT_SCOPE_VISUAL_V2B",
        'text:active?"ACTIVE":""',
        "root.selectedDeviceSlot=index",
        "readonly property var modelData: root.recordingChannels[index]",
        "readonly property var modelData: root.micTriggerChannels[index]",
        "property bool deferredModelSync: false",
        "root.engine.editDevicePath(String(modelData.path),v)",
    )

    # SYSTEM_OFFLINE_SINGLE_SURFACE_V7 — offline editor MUST be hidden
    # rather than merely read-only, to prevent overdraw with status captions.
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_MODE_NAME_SINGLE_SURFACE_V7",
        "SYSTEM_BT_NAME_SINGLE_SURFACE_V7",
        "SYSTEM_OFFLINE_TRUTH_V7",
        "id: modeNameInput",
        "text:root.modeNameDraft",
        "onTextEdited:root.modeNameDraft=text",
        "text:root.btNameDraft",
        "onTextEdited:root.btNameDraft=text",
        'text:root.deviceConnected?"":"OFFLINE"',
        "root.activeDeviceSlot",
        'text:active?"ACTIVE":""',
        "onClicked:root.presetManager.renameActiveMode(root.normalizedModeNameDraft())",
        "onClicked:root.presetManager.recallMode(root.selectedDeviceSlot+1)",
        "onClicked:root.presetManager.saveCurrentToSlot(root.selectedDeviceSlot+1)",
        "onClicked:root.presetManager.setBtName(root.normalizedBtNameDraft())",
        "onClicked:root.presetManager.resetBtName()",
    )
    require_count("qml/components/SystemWorkspaceImpl.qml", "visible:root.deviceConnected", 2)
    require_count("qml/components/SystemWorkspaceImpl.qml", 'text:"Connect K500 to read"', 2)
    forbid("qml/components/SystemWorkspaceImpl.qml", 'text:"Reset all"')
    forbid("qml/components/SystemWorkspaceImpl.qml", "Behavior on styleColor")

    # TOP_BAR_COHESION_V4 — source-only semantic/optical checks in K500 CI.
    require(
        "qml/theme/Theme.qml",
        "TOP_BAR_COHESION_V4",
        "topBarCaptionSize: 10",
        "topBarContextNameSize: 11",
        "topBarStatusSize: 10",
        "topBarContextWidth: 176",
    )
    require(
        "qml/components/TopBar.qml",
        "TOP_BAR_COHESION_V4",
        "font.pixelSize: Theme.topBarCaptionSize",
        "font.pixelSize: Theme.topBarContextNameSize",
        "font.pixelSize: Theme.topBarStatusSize",
        "Layout.preferredWidth: Theme.topBarContextWidth",
        "font.hintingPreference: Font.PreferFullHinting",
        'text:"Report"',
        "onClicked:supportReportDialog.open()",
        'root.deviceManager.sendPlayerCommand("rewind")',
        'root.deviceManager.sendPlayerCommand("playPause")',
        'root.deviceManager.sendPlayerCommand("forward")',
        "root.deviceManager.toggleMute()",
        'root.deviceManager.setTransportMode("bt")',
        'root.deviceManager.setTransportMode("usb")',
        "root.deviceManager.toggleConnection()",
        "signal aboutRequested()",
        "onClicked:root.aboutRequested()",
    )
    forbid("qml/components/TopBar.qml", 'text:"Support"')

    # MIN_WINDOW_LAYOUT_BUDGET_V6 — fast deterministic layout budget.
    # No new workflow or hard-coded screenshot dimensions in runtime code.
    require(
        "qml/Main.qml",
        "MIN_WINDOW_LAYOUT_BUDGET_V6",
        "minimumWidth: 1344",
        "Layout.minimumWidth: 440",
        "Layout.minimumWidth: 224",
    )
    require(
        "qml/components/SectionWorkspace.qml",
        "MIN_WINDOW_LAYOUT_BUDGET_V6",
        "Layout.minimumWidth: 422",
        "Layout.minimumWidth: 318",
        "Layout.minimumWidth:318",
        "Layout.minimumWidth: 346",
        "Layout.minimumWidth:346",
    )
    require_count("qml/components/SectionWorkspace.qml", "Layout.minimumWidth:318", 3)
    require_count("qml/components/SectionWorkspace.qml", "Layout.minimumWidth:346", 3)

    window_min = re.search(r"(?m)^\s*minimumWidth:\s*(\d+)\s*$", read("qml/Main.qml"))
    if not window_min:
        FAILURES.append("Main window must declare numeric minimumWidth")
    else:
        available = int(window_min.group(1)) - (2 * 12 + 170 + 12)
        music = 440 + 224 + 2 * 216 + 3 * 12
        mic = 246 + 422 + 216 + 2 * 12 + 216 + 12
        output = 318 + 346 + 216 + 2 * 12 + 216 + 12
        for section, required in (("Music", music), ("Mic", mic), ("Output", output)):
            if required > available:
                FAILURES.append(
                    f"{section} minimum rack {required}px exceeds available {available}px"
                )
            else:
                PASSES.append(f"{section} minimum rack budget {required}/{available}px")

    # FINAL_MICRO_TEXT_READABILITY_V5 — existing CI, no new workflows.
    require(
        "qml/components/SystemToggleRow.qml",
        "FINAL_MICRO_TEXT_READABILITY_V5",
        "implicitHeight: 50",
        "font.pixelSize: Theme.rackReadoutSize",
        "font.pixelSize: Theme.rackCaptionSize",
        "root.toggleRequested(!root.checked)",
        "root.blockedClicked()",
    )
    require(
        "qml/components/KeyControl.qml",
        "FINAL_MICRO_TEXT_READABILITY_V5",
        "Layout.preferredHeight: 16",
        "font.pixelSize: Theme.rackCaptionSize",
        "font.pixelSize: Theme.rackReadoutSize",
        "root.commit(root.key - 1)",
        "root.commit(root.key + 1)",
    )
    require(
        "qml/components/InputFader.qml",
        "FINAL_MICRO_TEXT_READABILITY_V5",
        "Layout.preferredHeight: 160",
        "font.pixelSize: Theme.rackCaptionSize",
        "font.pixelSize: Theme.rackReadoutSize",
        "font.pixelSize: Theme.rackUnitSize",
        "onClicked: root.sourceRequested()",
        "onValueEdited: function(v) { root.valueEdited(v) }",
    )
    require(
        "qml/components/StudioComboBox.qml",
        "FINAL_MICRO_TEXT_READABILITY_V5",
        "font.pixelSize: Theme.rackReadoutSize",
        "onActivated: function(index)",
        "control.valueEdited(control.value)",
        "model:control.popup.visible?control.delegateModel:null",
    )
    forbid("qml/components/SystemToggleRow.qml", "font.pixelSize: 7")
    forbid("qml/components/KeyControl.qml", "font.pixelSize: 7")
    forbid("qml/components/InputFader.qml", "font.pixelSize: 7")

    require(
        "qml/components/StudioKnob.qml",
        "font.pixelSize:root.premium ? 10 : 9",
    )


def check_release_contracts() -> None:
    cmake = read("CMakeLists.txt")
    if not re.search(r"project\(SonkupikStudioNative VERSION 1\.1\.2 LANGUAGES CXX\)", cmake):
        FAILURES.append("CMakeLists.txt: source version must be 1.1.2 for the patch release")
    if "V1_1_2_PATCH_RELEASE_QUALIFICATION_ANCHOR" not in cmake:
        FAILURES.append("CMakeLists.txt: v1.1.2 qualification anchor missing")

    legacy_acceptance = read("docs/V1_1_UPDATER_RC_ACCEPTANCE.md")
    for token in (
        "UPDATER_V1_1_ACCEPTANCE=accepted",
        "UPDATER_V1_1_ACCEPTED_TAG=v1.1.0-rc.5",
        "UPDATER_V1_1_ACCEPTED_COMMIT=01c545ed27d9a4316fb255765a25ae41f0e1457a",
    ):
        if token not in legacy_acceptance:
            FAILURES.append(f"historical v1.1.0 acceptance provenance changed: {token}")

    # Freeze gate: the 1.1.2 promotion may only accept an RC tag that
    # matches the source version. A stale 1.1.1 tag filter would make the
    # public stable path impossible even with correct binary hashes.
    forbid(
        ".github/workflows/windows-updater-promote.yml",
        "'^v1\\.1\\.1-rc",
    )

    # Stable v1.1.1 accepted bytes remain immutable while qualifying the
    # independent v1.1.2 RC line.
    historic111 = read("docs/V1_1_1_UPDATER_RC_ACCEPTANCE.md")
    for token in (
        "UPDATER_V1_1_1_ACCEPTANCE=accepted",
        "UPDATER_V1_1_1_ACCEPTED_TAG=v1.1.1-rc.1",
        "UPDATER_V1_1_1_ACCEPTED_COMMIT=df7a6dd3504f4c7ffbf19b12d9e0c2e1b9243b13",
    ):
        if token not in historic111:
            FAILURES.append(f"historical v1.1.1 RC acceptance changed: {token}")

    acceptance = read("docs/V1_1_2_UPDATER_RC_ACCEPTANCE.md")
    if not re.search(r"(?m)^UPDATER_V1_1_2_ACCEPTANCE=(pending|accepted)\s*$", acceptance):
        FAILURES.append("v1.1.2 updater acceptance machine-readable token missing")
    require(
        ".github/workflows/windows-updater-rc.yml",
        "v1.1.2-rc.${{ inputs.rc_number }}",
        "docs/V1_1_2_UPDATER_RC_ACCEPTANCE.md",
        "UPDATER_V1_1_2_ACCEPTANCE",
        "v1.1.2 candidate is pending independent acceptance",
    )
    require(
        ".github/workflows/windows-updater-promote.yml",
        "docs/V1_1_2_UPDATER_RC_ACCEPTANCE.md",
        "UPDATER_V1_1_2_ACCEPTED_COMMIT",
        "$stableTag = 'v1.1.2'",
        "PROMOTION_RC_TAG_MATCHES_SOURCE_VERSION_V1",
        "[regex]::Escape($version)",
        "if ($tag -notmatch $expectedRcPattern)",
        "Public stable SonKuPik K500 v1.1.2.",
    )

    require(
        "packaging/windows/installer.iss",
        "SMART_INSTALL_LAYOUT_V2",
        "PrivilegesRequired=lowest",
        "PrivilegesRequired=admin",
        "UsePreviousAppDir=no",
        "MIGRATE_LOCALAPPDATA_INSTALL_V1",
    )
    # ISSUE182_NATIVE_SMART_INSTALLER_V1: one native Inno artifact selects
    # new/previous scope without renaming legacy machine/user backend assets.
    require(
        "packaging/windows/installer.iss",
        "NATIVE_SMART_INSTALLER_V1",
        "PrivilegesRequiredOverridesAllowed=dialog",
        "UsePreviousPrivileges=yes",
        "DefaultDirName={autopf}",
        "SonKuPik-K500-v{#AppVersion}-Windows-Smart-Installer",
        "IsAdminInstallMode",
        "SMART_SAME_SCOPE_DESTINATION_GUARD_V1",
        "SmartRegisteredDestinationSafe",
        "LegacyPerUserInstallPresent(ProbeError)",
    )
    require(
        "tools/ci/installer_smoke.ps1",
        '"/DSmartInstaller=1"',
        "Smart Installer current-user fresh install and previous-scope reuse",
        "Smart Installer rejects opposite-scope registered machine install",
        "Smart Installer fails closed on stale same-scope registration",
        "KEEP-SMART-USER-DATA",
    )
    require(
        "functions/download/[kind].js",
        "SonKuPik-K500-${tag}-Windows-Setup.exe",
    )
    # Installed machine Setup cannot execute a legacy per-user uninstaller.
    require(
        "packaging/windows/installer.iss",
        "SAFE_CROSS_SCOPE_PREFLIGHT_V1",
        "OriginalUserRegistrationExists('64', ProbeError)",
        "OriginalUserRegistrationExists('32', ProbeError)",
        "ExecAsOriginalUser",
        "LegacyPerUserInstallPresent(ProbeError)",
        "Machine Setup stopped before changing either copy.",
    )
    forbid(
        "packaging/windows/installer.iss",
        "procedure MigrateLegacyPerUserInstall",
        "start \"\" /wait",
    )
    require(
        "tools/ci/installer_smoke.ps1",
        "Assert-MachineSetupBlocked",
        "K500-CI-NEVER-EXECUTE",
        "HKCU registration",
    )
    require(
        "src/AppUpdateManager.cpp",
        "release-manifest.json",
        "SHA256SUMS.txt",
        "QCryptographicHash::Sha256",
        "stableReleaseEligible",
    )
    require(
        ".github/workflows/windows-updater-rc.yml",
        "name: Windows Updater Release Candidate",
        "'K500 CI'",
        "requiredExactHeadWorkflows = @('K500 CI')",
        "Run deep updater lifecycle acceptance",
        "tools/ci/updater_deep_acceptance.ps1",
        "prerelease: true",
        "overwrite_files: false",
    )
    require(
        "tools/ci/updater_deep_acceptance.ps1",
        "RC_DEEP_UPDATER_ACCEPTANCE_V1",
        "SONKUPIK_UPDATE_HELPER_CI=1",
        "verified per-user helper update",
        "installer-nonzero",
        "failed-health-check",
        "explicit machine-to-user migration",
        "preset-user-data-must-survive",
        "official-cache-must-survive",
        "qsettings-must-survive",
        "--remove-stale-registration",
        "Active registration cleanup did not fail closed",
    )
    require(
        "src/AppUpdateManager.cpp",
        "Range",
        "Content-Range",
        "partialInstallerPath",
        "QCryptographicHash::Sha256",
    )
    require(
        "tools/ci/windows_test_suite.ps1",
        "RUNTIME_VERSION_FROM_CMAKE_V1",
        "$runtimeVersion = $cmakeVersionMatch.Groups[1].Value",
        "--update-health-check=$runtimeVersion",
    )
    forbid(
        "tools/ci/windows_test_suite.ps1",
        "--update-health-check=1.1.0",
    )
    require(
        ".github/workflows/windows-updater-promote.yml",
        "byte-identical-rc-artifact-promotion",
        "UPDATER_V1_1_2_ACCEPTED_COMMIT",
        "Stable staging changed accepted bytes:",
        "overwrite_files: false",
    )
    require(
        "README.md",
        "**v1.1.2** is the current public stable",
        "promoted byte-identically from accepted `v1.1.2-rc.2`",
        "landing API",
    )
    forbid(
        "README.md",
        "**v1.1.0** remains the current public stable",
        "Public downloads continue to resolve v1.1.0",
    )
    require(
        "docs/RELEASES.md",
        "Current public stable: **v1.1.2**",
        "## v1.1.2 stable record",
        "## v1.1.1 stable record",
        "122689e86e5624e7ce251108b9551f98216b62fcab976ea1d912c400c0d83b03",
        "0387799511a2c05269f536112ec4a17d12d05487814ba16cce4a9774d4f8b389",
    )
    forbid(
        "docs/RELEASES.md",
        "Current public stable: **v1.1.0**",
        "Next patch candidate source line: **v1.1.1**",
    )
    require(
        "CHANGELOG.md",
        "## v1.1.2 — 2026-10-10",
        "Released v1.1.2 stable from accepted `v1.1.2-rc.2`",
        "## v1.1.1 — 2026-10-08",
        "stable promotion completed without rebuilding",
        "landing `/api/release` endpoint",
    )
    forbid(
        "CHANGELOG.md",
        "## Unreleased — v1.1.1 patch release",
    )
    require(
        "site/app.js",
        "LANDING_RELEASE_BUTTON_VERSION_V1",
        "[data-release-template]",
        "[data-release-aria-template]",
        "applyTemplate",
    )
    require(
        "site/index.html",
        "Download SonKuPik K500 v1.1.2",
        'data-release-template="Download SonKuPik K500 {tag}"',
        'data-release-template="Download {tag}"',
    )
    require(
        "site/features/index.html",
        "Download v1.1.2",
        'data-release-template="Download {tag}"',
    )
    require(
        "site/download/index.html",
        "Download Smart Installer v1.1.2",
        'data-release-template="Download Smart Installer {tag}"',
        'data-release-template="Smart Installer {tag}"',
    )
    require(
        "AGENTS.md",
        "Public stable line: **v1.1.2**",
    )
    forbid(
        "AGENTS.md",
        "Public stable line: **v1.1.0**",
    )
    require(
        "docs/README.md",
        "| Public stable | `v1.1.2`",
        "| Stable source line | `v1.1.2`, promoted byte-identically from accepted `v1.1.2-rc.2`",
        "V1_1_1_UPDATER_RC_ACCEPTANCE.md",
        "landing buttons show the resolved stable tag",
    )
    forbid(
        "docs/README.md",
        "| Public stable | `v1.1.0`",
        "| Stable source line | `v1.1.0`",
    )
    require(
        "CHANGELOG.md",
        "## v1.1.1 — Mode 02 flagship broadcast lock",
        "## v1.1.1 — Mode 01 flagship singer-comfort lock",
        "## v1.1.0 — Balanced Enhanced Music Core candidate",
        "## v1.1.0 — Smart Installer public distribution",
        "current latest non-prerelease public stable is v1.1.1",
    )
    forbid(
        "CHANGELOG.md",
        "## v1.1.1 — Balanced Enhanced Music Core candidate",
        "## v1.1.1 — Smart Installer public distribution",
        "v1.1.0 is the current latest non-prerelease public stable",
    )
    # POST_STABLE_112_SOURCE_TRUTH — no stale visible fallback or
    # ambiguous current release metadata; dynamic download routes unchanged.
    require("docs/RELEASES.md",
        "decd7d8e54837d664fa586a7425b502faeec27dee280d9dc48f6f6989a23b8ab",
        "f9a1f29a9b209f47ffeb4f289b0d28b38a9df10764927dcef304397c65b3171b",
        "38046752955")
    require("functions/api/release.js", "releases/latest", "channel: 'stable'")
    require("functions/download/[kind].js", "releases/latest", "versioned: (tag) =>")
    for page in ("site/index.html", "site/features/index.html", "site/download/index.html"):
        if "v1.1.1" in read(page):
            FAILURES.append(f"{page}: outdated public stable fallback")

    require(
        ".github/workflows/windows-updater-rc.yml",
        "RC_LINE_CLOSED_AFTER_ACCEPTANCE_V1",
        "UPDATER_V1_1_2_ACCEPTANCE=accepted",
        "v1.1.2 RC line is closed after acceptance/stable promotion",
        "Bump the source version before starting a new RC line.",
    )
    rc_workflow = read(".github/workflows/windows-updater-rc.yml")
    rc_gate = rc_workflow.find("$gate = Get-Content 'docs/V1_1_2_UPDATER_RC_ACCEPTANCE.md' -Raw")
    rc_close = rc_workflow.find("RC_LINE_CLOSED_AFTER_ACCEPTANCE_V1")
    rc_url = rc_workflow.find('$url = "https://api.github.com/repos/$env:GITHUB_REPOSITORY/git/ref/tags/$tag"')
    if not (0 <= rc_gate < rc_close < rc_url):
        FAILURES.append("windows-updater-rc.yml: acceptance/closed-line/tag-check ordering is corrupt")
    if rc_workflow.count("RC_LINE_CLOSED_AFTER_ACCEPTANCE_V1") != 1:
        FAILURES.append("windows-updater-rc.yml: closed-line guard must occur exactly once")
    if "overwrite_files: false\n) {" in rc_workflow or "accepted\\s*\n          $response" in rc_workflow:
        FAILURES.append("windows-updater-rc.yml: malformed RC workflow tail detected")
    require(
        "src/main.cpp",
        "DonationPromptController",
        'QStringLiteral("donationPrompt")',
    )
    qris = ROOT / "resources" / "support" / "qris-sonkupik.png"
    if not qris.is_file() or qris.stat().st_size == 0:
        FAILURES.append("QRIS support asset missing/empty")
    else:
        PASSES.append("QRIS support asset")


def check_preset_sync() -> None:
    require(
        "src/k500/K500OfficialPresetSync.cpp",
        "api.github.com/repos/masarray/k500/contents/resources/presets?ref=main",
        "raw.githubusercontent.com",
        "validPresetBytes",
        "validateDeviceSlotCompatibility",
        "gitBlobShaHex",
        "QCryptographicHash::Sha1",
        "failed Git catalog blob integrity verification",
        "QSaveFile",
        "officialPresetLibrary/gitSha",
    )
    require(
        "src/k500/K500PresetFileBridge.h",
        "combinedPresets",
        "officialSyncBusy",
        "syncOfficialPresets",
    )
    require(
        "qml/components/MassUploadTransferWindow.qml",
        "fileBridge.combinedPresets",
        "SONKUPIK",
        "LOCAL",
        'text: "Clear plan"',
        "onClicked: root.clearTarget()",
    )
    forbid(
        "src/k500/K500OfficialPresetSync.cpp",
        "failed K500 size/checksum validation",
    )


def check_preset_semantic_safety() -> None:
    require(
        "src/k500/K500FieldContract.h",
        "AdjMannerVrOff",
        "FileOffset::AdjMannerVrOff",
        "0x0094",
        "0x008C",
        "EvidenceLevel::ProvenRoundTrip",
        "FileMutationPolicy::Writable",
    )
    require(
        "src/k500/K500PresetCodec.h",
        "validateDeviceSlotCompatibility",
        "verifyDeviceSlotProjection",
    )
    require(
        "src/k500/K500PresetCodec.cpp",
        "NativeEqQRawMax = 250",
        "NativeEqGainRawMax = 240",
        "verifyProjectionUnchecked",
        "Shadow verification failed",
    )
    forbid(
        "src/k500/K500PresetCodec.cpp",
        "std::clamp<int>(qRaw, 1, 0xff)",
        "std::min<int>(std::abs(static_cast<int>(gainRaw)), 0xff)",
    )
    require(
        "src/k500/K500PresetFileBridge.cpp",
        "deviceCompatible",
        "validateDeviceSlotCompatibility(edit.patch.bytes",
    )
    require(
        "src/k500/K500PresetEditMapper.cpp",
        "rejectNativeRange",
        "NativeRange::StartupLevelMax",
        "NativeRange::UsbRecordVolMax",
        "NativeRange::OutputDelayMaxMs",
        "NativeRange::ReverbDecayMaxMs",
        "NativeRange::EchoRepeatMax",
        "K500FieldContract::Field::AdjMannerVrOff.fileOffset",
    )
    forbid(
        "src/k500/K500PresetEditMapper.cpp",
        "std::clamp(qRound(value.toDouble()), 0, 50)",
        "b.addU8(0x0094",
    )
    require(
        "tools/k500_preset_lab.py",
        "validate_device_slot_compatibility",
        "build_device_slot_image",
        "PATCH_NATIVE_RANGES",
        "system.adjMannerVrOff",
        "slot-hash",
        "validate-library",
    )

    process = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools" / "k500_preset_lab.py"),
            "validate-library",
            str(ROOT / "resources" / "presets"),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if process.returncode != 0:
        detail = (process.stdout + "\n" + process.stderr).strip()
        FAILURES.append(f"official preset semantic compatibility failed: {detail}")
    else:
        PASSES.append("official preset bank: strict native-slot semantics")


def check_build_targets() -> None:
    require(
        "CMakeLists.txt",
        "k500_p0_perf_selftest",
        "k500_p1_state_selftest",
        "k500_p2_scheduler_selftest",
        "k500_p2_transport_lifecycle_test",
        "k500_p3_win_resource_test",
        "k500_p3_transport_shutdown_test",
        "k500_protocol_selftest",
        "k500_p2_selftest",
        "k500_p3_selftest",
        "k500_p32_corpus_test",
        "k500_p34_edit_persistence_test",
        "k500_p42_batch_test",
        "k500_donation_prompt_selftest",
        "k500_update_selftest",
    )


def workflow_events(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8")
    before_jobs = text.split("\njobs:", 1)[0]
    events: set[str] = set()
    for name in ("pull_request", "push", "schedule"):
        if re.search(rf"(?m)^\s{{2}}{name}:\s*$", before_jobs):
            events.add(name)
    return events


def check_ci_topology() -> None:
    workflows = ROOT / ".github" / "workflows"
    event_driven: dict[str, set[str]] = {}
    for path in sorted(workflows.glob("*.y*ml")):
        events = workflow_events(path)
        if events:
            event_driven[path.name] = events

    allowed = {"windows-build.yml"}
    unexpected = sorted(set(event_driven) - allowed)
    if unexpected:
        FAILURES.append(f"event-driven workflow sprawl: {unexpected}; only windows-build.yml may auto-run")
    ci_events = event_driven.get("windows-build.yml", set())
    if ci_events != {"pull_request", "push"}:
        FAILURES.append(f"windows-build.yml event contract mismatch: {sorted(ci_events)}")
    else:
        PASSES.append("CI topology: single PR/main workflow")

    workflow_count = len(list(workflows.glob("*.y*ml")))
    if workflow_count > 4:
        FAILURES.append(f"active workflow budget exceeded: {workflow_count} > 4")
    else:
        PASSES.append(f"active workflow budget: {workflow_count}/4")


def main() -> int:
    check_presets()
    check_uaudio_companions()
    check_fbe_mapping_evidence()
    check_preset_semantic_safety()
    check_protocol_and_state()
    check_ui_contracts()
    check_public_surface_contracts()
    check_release_contracts()
    check_preset_sync()
    check_build_targets()
    check_ci_topology()

    for item in PASSES:
        print(f"[PASS] {item}")
    if FAILURES:
        print("\nContract failures:", file=sys.stderr)
        for item in FAILURES:
            print(f"[FAIL] {item}", file=sys.stderr)
        return 1
    print(f"\nK500 contracts PASS ({len(PASSES)} groups)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
