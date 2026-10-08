#!/usr/bin/env python3
"""Fast, deterministic repository contracts for SonKuPik K500 CI.

This intentionally checks only invariants that are not better proven by compiled
self-tests. It replaces dozens of per-feature GitHub workflows with one
cross-platform contract suite.
"""
from __future__ import annotations

import hashlib
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
        "\"Device setting · not stored in preset\"",
        "\"Saved with preset\"",
        "\"BT rename: max 8 characters · BLE read-only\"",
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
    require(
        "qml/components/TopBar.qml",
        "font.pixelSize: 10",
        "font.hintingPreference: Font.PreferFullHinting",
    )
    require(
        "qml/components/StudioKnob.qml",
        "font.pixelSize:root.premium ? 10 : 9",
    )


def check_release_contracts() -> None:
    cmake = read("CMakeLists.txt")
    if not re.search(r"project\(SonkupikStudioNative VERSION 1\.1\.1 LANGUAGES CXX\)", cmake):
        FAILURES.append("CMakeLists.txt: source version must be 1.1.1 for the patch release")
    if "V1_1_1_PATCH_RELEASE_QUALIFICATION_ANCHOR" not in cmake:
        FAILURES.append("CMakeLists.txt: v1.1.1 qualification anchor missing")

    legacy_acceptance = read("docs/V1_1_UPDATER_RC_ACCEPTANCE.md")
    for token in (
        "UPDATER_V1_1_ACCEPTANCE=accepted",
        "UPDATER_V1_1_ACCEPTED_TAG=v1.1.0-rc.5",
        "UPDATER_V1_1_ACCEPTED_COMMIT=01c545ed27d9a4316fb255765a25ae41f0e1457a",
    ):
        if token not in legacy_acceptance:
            FAILURES.append(f"historical v1.1.0 acceptance provenance changed: {token}")

    acceptance = read("docs/V1_1_1_UPDATER_RC_ACCEPTANCE.md")
    if not re.search(r"(?m)^UPDATER_V1_1_1_ACCEPTANCE=(pending|accepted)\s*$", acceptance):
        FAILURES.append("v1.1.1 updater acceptance machine-readable token missing")
    require(
        ".github/workflows/windows-updater-rc.yml",
        "v1.1.1-rc.${{ inputs.rc_number }}",
        "docs/V1_1_1_UPDATER_RC_ACCEPTANCE.md",
        "UPDATER_V1_1_1_ACCEPTANCE",
        "Stable v1.1.1 has now been promoted from the accepted RC1 bytes",
    )
    require(
        ".github/workflows/windows-updater-promote.yml",
        "docs/V1_1_1_UPDATER_RC_ACCEPTANCE.md",
        "UPDATER_V1_1_1_ACCEPTED_COMMIT",
        "$stableTag = 'v1.1.1'",
        "Public stable SonKuPik K500 v1.1.1.",
    )

    require(
        "packaging/windows/installer.iss",
        "SMART_INSTALL_LAYOUT_V2",
        "PrivilegesRequired=lowest",
        "PrivilegesRequired=admin",
        "UsePreviousAppDir=no",
        "MIGRATE_LOCALAPPDATA_INSTALL_V1",
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
        "UPDATER_V1_1_1_ACCEPTED_COMMIT",
        "Stable staging changed accepted bytes:",
        "overwrite_files: false",
    )
    require(
        "README.md",
        "**v1.1.1** is the current public stable",
        "promoted byte-identically from accepted `v1.1.1-rc.1`",
        "landing API",
    )
    forbid(
        "README.md",
        "**v1.1.0** remains the current public stable",
        "Public downloads continue to resolve v1.1.0",
    )
    require(
        "docs/RELEASES.md",
        "Current public stable: **v1.1.1**",
        "Current public stable source line: **v1.1.1**",
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
        "Download SonKuPik K500 v1.1.1",
        'data-release-template="Download SonKuPik K500 {tag}"',
        'data-release-template="Download {tag}"',
    )
    require(
        "site/features/index.html",
        "Download v1.1.1",
        'data-release-template="Download {tag}"',
    )
    require(
        "site/download/index.html",
        "Download Smart Installer v1.1.1",
        'data-release-template="Download Smart Installer {tag}"',
        'data-release-template="Smart Installer {tag}"',
    )
    require(
        "AGENTS.md",
        "Public stable line: **v1.1.1**",
    )
    forbid(
        "AGENTS.md",
        "Public stable line: **v1.1.0**",
    )
    require(
        "docs/README.md",
        "| Public stable | `v1.1.1`",
        "| Stable source line | `v1.1.1`, promoted byte-identically from accepted `v1.1.1-rc.1`",
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
    require(
        ".github/workflows/windows-updater-rc.yml",
        "RC_LINE_CLOSED_AFTER_ACCEPTANCE_V1",
        "UPDATER_V1_1_1_ACCEPTANCE=accepted",
        "v1.1.1 RC line is closed after acceptance/stable promotion",
        "Bump the source version before starting a new RC line.",
    )
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
        "Remove All",
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
