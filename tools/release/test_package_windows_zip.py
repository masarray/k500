#!/usr/bin/env python3
"""Fast offline stable ZIP regression. No GitHub network or Windows build."""
import hashlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from zipfile import ZipFile

SCRIPT = Path(__file__).with_name("package_windows_zip.py")


class StableZipTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.tag = "v1.1.3"
        self.exe = self.root / ("SonKuPik-K500-" + self.tag + "-Windows-Smart-Installer.exe")
        self.payload = b"verified existing installer fixture\x00\x01\x02"
        self.exe.write_bytes(self.payload)
        self.sums = self.root / "SHA256SUMS.txt"
        self.sums.write_text(hashlib.sha256(self.payload).hexdigest() + "  " + self.exe.name + "\n")
        self.output = self.root / "dist"

    def run_script(self, tag=None):
        return subprocess.run([
            sys.executable, str(SCRIPT),
            "--tag", tag or self.tag,
            "--installer", str(self.exe),
            "--sums", str(self.sums),
            "--output-dir", str(self.output),
        ], capture_output=True, text=True)

    def test_archive_is_exact_and_reproducible(self):
        first = self.run_script()
        self.assertEqual(first.returncode, 0, first.stderr)
        archive = self.output / ("SonKuPik-K500-" + self.tag + "-Windows.zip")
        with ZipFile(archive) as z:
            self.assertEqual(z.testzip(), None)
            self.assertEqual(z.namelist(), [self.exe.name, "README.txt", "SHA256SUMS.txt"])
            self.assertEqual(z.read(self.exe.name), self.payload)
            self.assertIn(b"Extract All", z.read("README.txt"))
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        archive.unlink()
        self.assertEqual(self.run_script().returncode, 0)
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(), digest)

    def test_refuses_tampered_executable(self):
        self.exe.write_bytes(b"tampered binary")
        result = self.run_script()
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.output.exists() and list(self.output.glob("*.zip")))

    def test_refuses_rc_tag(self):
        result = self.run_script("v1.1.3-rc.1")
        self.assertNotEqual(result.returncode, 0)

    def test_refuses_overwrite(self):
        self.assertEqual(self.run_script().returncode, 0)
        self.assertNotEqual(self.run_script().returncode, 0)


if __name__ == "__main__":
    unittest.main()
