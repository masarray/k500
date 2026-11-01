#!/usr/bin/env python3
"""Publish-friendly aliases for the EXACT qualified stable installer.

Do not rebuild or modify the signed/unsigned payload. Verify the original
published SHA256SUMS.txt first, then create easy-to-recognize EXE + ZIP names.
All outputs are additive; original versioned GitHub Release assets stay intact.
"""
from __future__ import annotations

import argparse
import hashlib
import re
import shutil
from pathlib import Path
from zipfile import ZIP_STORED, ZipFile, ZipInfo

TAG = re.compile(r"^v(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)\.(?:0|[1-9]\d*)$")
SUM_LINE = re.compile(r"^([a-fA-F0-9]{64})\s+\*?(.+?)\s*$")
SHORT_EXE = "sonkupik-k500-setup.exe"
SHORT_ZIP = "sonkupik-k500-setup.zip"


def sha256_file(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def verify_original(source: Path, sums: Path, tag: str) -> str:
    # v1.1.4+ publishes ONE Per-User installer. Preserve v1.1.3 legacy.
    numbers = tuple(int(x) for x in tag[1:].split(".")) if TAG.fullmatch(tag) else (0, 0, 0)
    suffix = "Setup-PerUser" if numbers >= (1, 1, 4) else "Smart-Installer"
    expected = f"SonKuPik-K500-{tag}-Windows-{suffix}.exe"
    if not TAG.fullmatch(tag) or source.name != expected:
        raise ValueError("Not a published stable Windows Per-User/legacy installer filename")
    if not source.is_file() or source.stat().st_size == 0:
        raise ValueError("Missing or empty installer")
    records: dict[str, str] = {}
    for line in sums.read_text(encoding="utf-8-sig").splitlines():
        match = SUM_LINE.fullmatch(line)
        if match:
            if match.group(2) in records:
                raise ValueError("Duplicate checksum record")
            records[match.group(2)] = match.group(1).lower()
    original_hash = records.get(expected)
    if original_hash is None:
        raise ValueError("Published checksum for original installer missing")
    if sha256_file(source) != original_hash:
        raise ValueError("Original stable installer SHA-256 mismatch")
    return original_hash


def zi(name: str) -> ZipInfo:
    info = ZipInfo(name, (2026, 1, 1, 0, 0, 0))
    info.compress_type = ZIP_STORED
    info.create_system = 3
    info.external_attr = 0o644 << 16
    return info


def package(tag: str, source: Path, sums: Path, output: Path) -> None:
    source_hash = verify_original(source, sums, tag)
    output.mkdir(parents=True, exist_ok=True)
    exe = output / SHORT_EXE
    zpath = output / SHORT_ZIP
    planned = [exe, zpath, output / (SHORT_EXE + ".sha256"), output / (SHORT_ZIP + ".sha256")]
    if any(p.exists() for p in planned):
        raise FileExistsError("Refusing to overwrite an existing download asset")
    # Both the renamed EXE and the installer inside ZIP MUST retain identical bytes.
    readme = (
        "SonKuPik K500 - Official Windows Setup\n"
        "=====================================\n\n"
        "1. Right-click the ZIP and choose Extract All in Windows File Explorer.\n"
        f"2. Run {SHORT_EXE} from the extracted folder.\n"
        "3. Follow the installation prompts.\n\n"
        f"Official release: https://github.com/masarray/k500/releases/tag/{tag}\n"
        f"Original asset: {source.name}\n"
        f"Original installer SHA-256: {source_hash}\n\n"
        "This installer is byte-for-byte identical to the original stable EXE.\n"
        "The ZIP is NOT a portable edition, and does NOT bypass antivirus or\n"
        "Windows SmartScreen. Never disable security protection to install.\n"
    )
    inner_sums = f"{source_hash}  {SHORT_EXE}\n"
    try:
        shutil.copyfile(source, exe)
        if sha256_file(exe) != source_hash:
            raise ValueError("Renamed copy changed the installer bytes")
        with ZipFile(zpath, "w", allowZip64=True) as archive:
            with archive.open(zi(SHORT_EXE), "w", force_zip64=True) as target:
                with source.open("rb") as original:
                    for block in iter(lambda: original.read(1024 * 1024), b""):
                        target.write(block)
            archive.writestr(zi("README.txt"), readme.encode("utf-8"))
            archive.writestr(zi("SHA256SUMS.txt"), inner_sums.encode("ascii"))
        with ZipFile(zpath) as archive:
            if archive.namelist() != [SHORT_EXE, "README.txt", "SHA256SUMS.txt"]:
                raise ValueError("Unexpected archive entries")
            if archive.testzip() is not None:
                raise ValueError("ZIP CRC check failed")
            if archive.getinfo(SHORT_EXE).file_size != source.stat().st_size:
                raise ValueError("ZIP installer length mismatch")
            with archive.open(SHORT_EXE) as bundled:
                digest = hashlib.sha256()
                for block in iter(lambda: bundled.read(1024 * 1024), b""):
                    digest.update(block)
                if digest.hexdigest() != source_hash:
                    raise ValueError("ZIP installer hash mismatch")
        for artifact in (exe, zpath):
            (output / (artifact.name + ".sha256")).write_text(
                f"{sha256_file(artifact)}  {artifact.name}\n", encoding="ascii"
            )
    except Exception:
        for path in planned:
            path.unlink(missing_ok=True)
        raise
    print(f"Verified source {tag}: {source_hash}")
    print("Published aliases:", ", ".join(p.name for p in planned))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tag", required=True)
    parser.add_argument("--installer", required=True, type=Path)
    parser.add_argument("--sums", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    package(args.tag, args.installer, args.sums, args.output_dir)


if __name__ == "__main__":
    main()
