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


def require_exact_count(rel: str, token: str, expected: int) -> None:
    count = read(rel).count(token)
    if count != expected:
        FAILURES.append(f"{rel}: expected exactly {expected} occurrences of {token!r}, found {count}")
    else:
        PASSES.append(f"{rel}: {token!r} exactly x{count}")


def check_presets() -> None:
    expected = {
        "01_KONSER_NYANYI.k500": ("KONSER NYANYI", "761d0ecf1f470ce433fcf760d7ee1317e994dbefbb16fc71e8498aea9d99d6c4"),
        "02_MC_HOST_RADIO.k500": ("MC HOST RADIO", "8b18cdbd1f4e1ae409c882d84955bfa34ca770e459615524591732182f14339c"),
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
        "FRONT VR ACTIVE",
        "SOFTWARE CONTROL",
        "SYSTEM_DEFERRED_AUTHORITATIVE_SYNC_V1",
    )
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
