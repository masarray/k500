#!/usr/bin/env python3
"""Fast, deterministic repository contracts for SonKuPik K500 CI.

This intentionally checks only invariants that are not better proven by compiled
self-tests. It replaces dozens of per-feature GitHub workflows with one
cross-platform contract suite.
"""
from __future__ import annotations

import hashlib
import re
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


def check_presets() -> None:
    expected = {
        "01_KONSER_NYANYI.k500": ("KONSER NYANYI", "761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4"),
        "02_MC_HOST_RADIO.k500": ("MC HOST RADIO", "05debfccaa1fd7cdbe4847e441e7e86d4caffbce7e612bbf7694db654165bf3b"),
        "03_KAR_DANGDUT.k500": ("KAR DANGDUT", "ec683042962c635d8d87d262512a694797bd62842c07775e53eb497f34d329bc"),
        "04_POP_ROCK_BALLAD.k500": ("POP ROCK BALLAD", "d18c9ddf4d8ba9d5d5fa6f027b97cc31784138a510c1e35778b8995e380172f5"),
        "05_POP_KENANGAN_V2.k500": ("POP KENANGAN V2", "e0d6e985f068576a37ea776c1ed730b63bafac44d3afc80ab31f8541ff6398b5"),
        "06_SHOLAWAT_SYAHDU.k500": ("SHOLAWAT SYAHDU", "e7854512443699f6b3202db488a9d2d137162b4d4e756b4f7c34950a541f909c"),
        "07_JAZZ_LOUNGE.k500": ("JAZZ LOUNGE", "934caa877de5e8cb7ef2dfe8d12a99a4b804989807357ce62805e419b0a6bae7"),
        "08_BLUES_CLUB.k500": ("BLUES CLUB", "741bfa917a8d491070d18d223e4be7a5010d7c4edb171f2e0e65f95cbbe22146"),
        "09_ACOUSTIC_NATURAL.k500": ("ACOUSTIC NATURAL", "8a5ce6f8b310d24d47d755f99c5acf9b9d992f9a85a2e13ccb7156c0f1d8b81c"),
        "10_REGGAE_DUB.k500": ("REGGAE DUB", "1984d309db61c7258329765306e3e573e64730b04a5cd4472568c3f332742c90"),
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
        "src/k500/K500ResponseParser.cpp",
        "tryDecodeAdjMannerVrOff",
        "response.data.at(19)",
        "(flags & 0x01u) == 0",
    )
    require(
        "src/k500/K500PresetManager.cpp",
        "ReadbackPurpose::AdjManner",
        "startReadback(ReadbackPurpose::AdjManner)",
        "939-byte ownership resync complete",
    )
    require(
        "src/k500/K500DeviceManager.cpp",
        "K500Protocol::ActiveMemorySize",
        "K500Protocol::ActiveMemoryBlockSize",
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
    )
    require_count("qml/components/RackFaderPanel.qml", "model: root.channels ? root.channels.length : 0", 2)
    require(
        "qml/components/SystemWorkspaceImpl.qml",
        "SYSTEM_LIVE_STABLE_DELEGATE_V1",
        "root.recordingChannels.length",
        "root.micTriggerChannels.length",
        "FRONT VR ACTIVE",
        "SOFTWARE CONTROL",
    )
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

    # Presentation may never bypass the native controller/engine boundary.
    for path in (ROOT / "qml").rglob("*.qml"):
        data = path.read_text(encoding="utf-8")
        if "K500Controller" in data or "K500WinIo" in data:
            FAILURES.append(f"{path.relative_to(ROOT)}: direct native I/O reference from QML")
    PASSES.append("QML/native ownership boundary")


def check_release_contracts() -> None:
    cmake = read("CMakeLists.txt")
    if not re.search(r"project\(SonkupikStudioNative VERSION 1\.1\.0 LANGUAGES CXX\)", cmake):
        FAILURES.append("CMakeLists.txt: source version must remain 1.1.0 for RC5")
    if "V1_1_RC5_PUBLIC_RELEASE_QUALIFICATION_ANCHOR" not in cmake:
        FAILURES.append("CMakeLists.txt: RC5 qualification anchor missing")

    acceptance = read("docs/V1_1_UPDATER_RC_ACCEPTANCE.md")
    if not re.search(r"(?m)^UPDATER_V1_1_ACCEPTANCE=(pending|accepted)\s*$", acceptance):
        FAILURES.append("updater acceptance machine-readable token missing")

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
        "prerelease: true",
        "overwrite_files: false",
    )
    require(
        ".github/workflows/windows-updater-promote.yml",
        "byte-identical-rc-artifact-promotion",
        "UPDATER_V1_1_ACCEPTED_COMMIT",
        "Stable staging changed accepted bytes:",
        "overwrite_files: false",
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
    check_protocol_and_state()
    check_ui_contracts()
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
