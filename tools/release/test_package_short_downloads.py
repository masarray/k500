#!/usr/bin/env python3
"""Offline checksum, filename, ZIP integrity and provenance tests."""
import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

SCRIPT = Path(__file__).with_name("package_short_downloads.py")
SHORT = "sonkupik-k500-setup"


class AliasTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.tag = "v1.1.4"
        self.payload = b"untouched K500 installer fixture \x00\x01"
        self.source = self.root / f"SonKuPik-K500-{self.tag}-Windows-Setup-PerUser.exe"
        self.source.write_bytes(self.payload)
        self.sums = self.root / "SHA256SUMS.txt"
        self.hash = hashlib.sha256(self.payload).hexdigest()
        self.sums.write_text(f"{self.hash}  {self.source.name}\n")
        self.out = self.root / "output"

    def run_pack(self, tag=None):
        return subprocess.run([
            sys.executable, str(SCRIPT),
            "--tag", tag or self.tag, "--installer", str(self.source),
            "--sums", str(self.sums), "--output-dir", str(self.out),
        ], capture_output=True, text=True)

    def test_aliases_identical_and_integrity(self):
        result = self.run_pack()
        self.assertEqual(result.returncode, 0, result.stderr)
        exe = self.out / (SHORT + ".exe")
        archive = self.out / (SHORT + ".zip")
        self.assertEqual(exe.read_bytes(), self.payload)
        self.assertEqual(self.source.read_bytes(), self.payload)
        with ZipFile(archive) as z:
            self.assertEqual(z.namelist(), [SHORT + ".exe", "README.txt", "SHA256SUMS.txt"])
            self.assertEqual(z.testzip(), None)
            self.assertEqual(z.read(SHORT + ".exe"), self.payload)
            self.assertIn(self.hash.encode(), z.read("SHA256SUMS.txt"))
        for path in (exe, archive):
            checksum = (self.out / (path.name + ".sha256")).read_text().split()[0]
            self.assertEqual(checksum, hashlib.sha256(path.read_bytes()).hexdigest())

    def test_deterministic_zip(self):
        self.assertEqual(self.run_pack().returncode, 0)
        archive = self.out / (SHORT + ".zip")
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        for item in self.out.iterdir():
            item.unlink()
        self.assertEqual(self.run_pack().returncode, 0)
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), digest)

    def test_tampering_rejected(self):
        self.source.write_bytes(b"modified")
        self.assertNotEqual(self.run_pack().returncode, 0)
        self.assertFalse(list(self.out.glob("*")) if self.out.exists() else False)

    def test_rc_rejected(self):
        self.assertNotEqual(self.run_pack("v1.1.4-rc.1").returncode, 0)

    def test_missing_original_hash_rejected(self):
        self.sums.write_text("0" * 64 + "  unexpected.exe\n")
        self.assertNotEqual(self.run_pack().returncode, 0)

    def test_overwrite_rejected(self):
        self.assertEqual(self.run_pack().returncode, 0)
        self.assertNotEqual(self.run_pack().returncode, 0)


if __name__ == "__main__":
    unittest.main()
