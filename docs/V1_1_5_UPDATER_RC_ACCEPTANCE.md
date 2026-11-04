# SonKuPik K500 v1.1.5 — adaptive-laptop UI PATCH release gate
# PATCH_115_RC_ACCEPTANCE_V1. Owner explicitly tested the immutable
# v1.1.5-rc.1 installer and approved public stable promotion on 2026-11-05.
# Never reuse v1.1.4-rc.3 or overwrite its publicly distributed bytes.
UPDATER_V1_1_5_ACCEPTANCE=accepted
UPDATER_V1_1_5_ACCEPTED_TAG=v1.1.5-rc.1
UPDATER_V1_1_5_ACCEPTED_COMMIT=89a1e59535c891190d226b9eb636a5c967dd4c73
UPDATER_V1_1_5_USER_SHA256=72651326cd871ae3278b92b713b2bb9db03dd3761970fab2e7ec0d5bba62cb3f

# Acceptance evidence: visual QRIS dismiss, 1366x768 full fader access,
# 1920x1080 @ 150% Windows DPI, System tabs, source/preset state intact,
# installer no-admin/no-UAC and GitHub immutable SHA/manifest.
# Only Windows x64 USB HID is qualified. Bluetooth remains experimental.

# Owner acceptance: installed/tested RC and confirmed adaptive UI,
# accessible QRIS dismissal, System tabs and faders. Precise 150%-DPI
# multi-monitor hardware QA is not claimed beyond evidence received.
# Public stable installer copies exactly these RC bytes, preserving the
# existing -PerUser filename required by the in-app updater; the
# friendly Setup.exe filename is provided INSIDE the verified public ZIP.
