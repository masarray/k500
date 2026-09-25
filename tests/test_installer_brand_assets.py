"""Regression tests for the small Inno Setup wizard logo clipping."""
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "packaging" / "windows"))
from generate_brand_assets import BACKGROUND, branded_bitmap, generate_assets  # noqa: E402


class InstallerBrandAssetsTests(unittest.TestCase):
    def setUp(self):
        # A fully opaque square exposes any clipped bottom edge.
        self.logo = Image.new("RGBA", (256, 256), (255, 255, 255, 255))

    def test_small_logo_is_centered_and_has_safe_margins(self):
        image = branded_bitmap(
            self.logo, (55, 55), (40, 40), center_vertically=True
        )
        bounds = ImageChops.difference(
            image, Image.new("RGB", image.size, BACKGROUND)
        ).getbbox()
        self.assertEqual((7, 7, 47, 47), bounds)
        self.assertEqual((55, 55), image.size)

    def test_asset_files_are_generated_without_cropping(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "source.png"
            self.logo.save(source)
            icon = root / "package" / "SonKuPik-K500.ico"
            wizard = root / "build" / "wizard.bmp"
            small = root / "build" / "wizard-small.bmp"
            generate_assets(source, icon, wizard, small)
            with Image.open(small) as image:
                self.assertEqual((55, 55), image.size)
                self.assertEqual((7, 7, 47, 47), ImageChops.difference(
                    image.convert("RGB"),
                    Image.new("RGB", image.size, BACKGROUND)
                ).getbbox())
            with Image.open(wizard) as image:
                self.assertEqual((164, 314), image.size)
            with Image.open(icon) as image:
                self.assertEqual((256, 256), image.size)


if __name__ == "__main__":
    unittest.main()
