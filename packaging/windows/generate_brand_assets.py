"""Generate the icon and wizard artwork from the authoritative K500 logo.

Keep the small logo entirely inside the bitmap: the Inno Setup header clips
anything drawn beyond its 55 x 55 source image, especially the bottom edge.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image

BRAND_LOGO = Path("assets/SonKuPik-k500-logo.png")
BACKGROUND = (13, 18, 22)
ICON_SIZES = [(16, 16), (24, 24), (32, 32), (48, 48),
              (64, 64), (128, 128), (256, 256)]


def branded_bitmap(
    source: Image.Image,
    size: tuple[int, int],
    logo_box: tuple[int, int],
    *,
    center_vertically: bool = False,
) -> Image.Image:
    """Composite the unmodified logo with an explicitly checked safe margin."""
    canvas = Image.new("RGB", size, BACKGROUND)
    logo = source.copy().convert("RGBA")
    logo.thumbnail(logo_box, Image.Resampling.LANCZOS)
    x = (size[0] - logo.width) // 2
    y = ((size[1] - logo.height) // 2 if center_vertically
         else max(12, (size[1] - logo.height) // 3))
    if x < 0 or y < 0 or x + logo.width > size[0] or y + logo.height > size[1]:
        raise ValueError("Installer artwork would be clipped by its bitmap canvas")
    canvas.paste(logo, (x, y), logo)
    return canvas


def generate_assets(
    source_path: Path, icon_path: Path, wizard_path: Path, small_path: Path
) -> None:
    source = Image.open(source_path).convert("RGBA")
    for path in (icon_path, wizard_path, small_path):
        path.parent.mkdir(parents=True, exist_ok=True)

    source.save(icon_path, format="ICO", sizes=ICON_SIZES)
    branded_bitmap(source, (164, 314), (132, 132)).save(wizard_path, format="BMP")
    # Old generator placed a 48px logo at y=12 in a 55px bitmap (bottom=60):
    # five pixels were cut off. Center a 40px logo with >=7px on every side.
    branded_bitmap(
        source, (55, 55), (40, 40), center_vertically=True
    ).save(small_path, format="BMP")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=BRAND_LOGO)
    parser.add_argument("--icon", type=Path, required=True)
    parser.add_argument("--wizard", type=Path, required=True)
    parser.add_argument("--wizard-small", type=Path, required=True)
    args = parser.parse_args()
    generate_assets(args.source, args.icon, args.wizard, args.wizard_small)


if __name__ == "__main__":
    main()
