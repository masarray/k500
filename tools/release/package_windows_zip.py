#!/usr/bin/env python3
"""Package an existing accepted Windows installer as a ZIP without rebuilding it.

The release's SHA256SUMS.txt must already list the exact installer bytes.
Use deterministic ZIP_STORED entries so the archive is reproducible and fast
to extract (Inno Setup EXE payload is already compressed).
"""
from __future__ import annotations

import argparse
import hashlib
import re
from pathlib import Path
from zipfile import ZipFile, ZipInfo, ZIP_STORED

TAG = re.compile(r"^v(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)$")
SHA_LINE = re.compile(r"^([a-fA-F0-9]{64})\s+\*?(.+?)\s*$")
README = """SonKuPik K500 - Official Windows Smart Installer
================================================

1. In Windows File Explorer, right-click the ZIP and choose Extract All.
2. Open the extracted folder.
3. Double-click the SonKuPik-K500-...-Windows-Smart-Installer.exe.
4. Follow the installer instructions.

The installer EXE is byte-for-byte identical to the EXE released separately
at https://github.com/masarray/k500/releases .
Its SHA-256 can be checked against the included SHA256SUMS.txt.

Windows SmartScreen or antivirus may warn about unsigned downloads.
Do not disable security protection or use password-protected archives.
If your security software reports a threat, verify the official release
and report possible false positives to the security vendor.

This ZIP is an alternative download format, NOT a portable app.
"""


def verified_sha256(installer: Path, sums: Path) -> str:
    matches = {}
    for line in sums.read_text(encoding="utf-8-sig").splitlines():
        m = SHA_LINE.match(line)
        if m:
            if m.group(2) in matches:
                raise ValueError("Duplicate SHA256SUMS entry: " + m.group(2))
            matches[m.group(2)] = m.group(1).lower()
    expected = matches.get(installer.name)
    if not expected:
        raise ValueError("Installer missing from official SHA256SUMS.txt")
    digest = hashlib.sha256()
    with installer.open("rb") as fh:
        for block in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(block)
    actual = digest.hexdigest()
    if actual != expected:
        raise ValueError("Installer SHA-256 mismatch (refuse to package)")
    return actual


def archive_info(name: str) -> ZipInfo:
    info = ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
    info.compress_type = ZIP_STORED
    info.external_attr = 0o644 << 16
    info.create_system = 3
    return info


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--tag", required=True)
    p.add_argument("--installer", required=True, type=Path)
    p.add_argument("--sums", required=True, type=Path)
    p.add_argument("--output-dir", required=True, type=Path)
    args = p.parse_args()
    tag = args.tag
    if not TAG.fullmatch(tag):
        raise ValueError("Must package a stable semantic version tag")
    expected_name = "SonKuPik-K500-" + tag + "-Windows-Smart-Installer.exe"
    if args.installer.name != expected_name:
        raise ValueError("Unexpected stable Smart Installer filename")
    digest = verified_sha256(args.installer, args.sums)
    if not args.installer.is_file() or args.installer.stat().st_size == 0:
        raise ValueError("Missing/empty installer")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    destination = args.output_dir / ("SonKuPik-K500-" + tag + "-Windows.zip")
    if destination.exists():
        raise FileExistsError("Existing ZIP must not be overwritten: " + str(destination))
    try:
        with ZipFile(destination, "w", allowZip64=True) as archive:
            with archive.open(archive_info(args.installer.name), "w", force_zip64=True) as dest:
                with args.installer.open("rb") as src:
                    for block in iter(lambda: src.read(1024 * 1024), b""):
                        dest.write(block)
            archive.writestr(archive_info("README.txt"), README)
            archive.writestr(archive_info("SHA256SUMS.txt"), args.sums.read_bytes())
        with ZipFile(destination) as check:
            if check.namelist() != [expected_name, "README.txt", "SHA256SUMS.txt"]:
                raise ValueError("ZIP entry manifest mismatch")
            if check.testzip() is not None:
                raise ValueError("ZIP integrity check failed")
            if check.getinfo(expected_name).file_size != args.installer.stat().st_size:
                raise ValueError("Packaged installer length mismatch")
    except Exception:
        destination.unlink(missing_ok=True)
        raise
    print("Verified original installer SHA-256:", digest)
    print("Created official ZIP:", destination.name, destination.stat().st_size, "bytes")


if __name__ == "__main__":
    main()
