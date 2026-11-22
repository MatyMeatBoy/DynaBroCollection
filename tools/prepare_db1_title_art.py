"""Sample DB1's title PNGs at the exact coordinates used by the compositor."""

from pathlib import Path

from PIL import Image

ASSETS = Path(__file__).resolve().parents[1] / 'assets/img'


def prepare(source: str, target: str, size: tuple[int, int], visible_rows: int):
    with Image.open(ASSETS / source) as original:
        rgba = original.convert('RGBA')
    width, height = size
    pixels = rgba.load()
    sampled = Image.new('RGBA', size)
    sampled.paste(Image.frombytes('RGBA', (width, visible_rows), bytes(
        component
        for y in range(visible_rows)
        for x in range(width)
        for component in pixels[x * rgba.width // width, y * rgba.height // height]
    )))
    sampled.save(ASSETS / target, optimize=True)


if __name__ == '__main__':
    prepare('db1-title-background-512x288.png', 'db1-title-background-native.png', (512, 288), 32)
    prepare('dyna-logo-v1.png', 'db1-title-logo-native.png', (224, 120), 120)
