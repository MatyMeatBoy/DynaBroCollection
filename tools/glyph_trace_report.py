"""Report the 16x16 glyph-ID strings a DB2 Special run rendered.

Run the native probe with DYNA_TRACE_GLYPH=1 (and --language en or ja), then:
    python -m tools.glyph_trace_report run.log "<DB2 Special ROM>" out.png

Output: one line per distinct string (address, glyph count, control cells, widths, whether
translations/db2special-en.json already covers it) and a PNG rendering the strings with the
original font so they can be read and translated (glyph_text entries, see
games/db2_glyph_text.c). Strings are keyed by the renderer's current word pointer, not by a
preceding NUL, because some texts sit directly after data.
"""
import argparse
import collections
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.db1_speech_translation import decode  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
GROUP_TABLE = 0x218974


def control_cells(words):
    count = prev = 0
    for word in words:
        if not word or word & 1 or word >= 0x120 or word <= prev:
            break
        prev = word
        count += 1
    return count


def parse(log: Path, rom: bytes):
    u16 = lambda a: int.from_bytes(rom[a:a + 2], 'big')
    calls = collections.OrderedDict()
    for line in log.read_text(encoding='utf-8', errors='replace').splitlines():
        m = re.match(r'\[glyph\] a1=([0-9A-F]+) word=[0-9A-F]+ width=(\d+)', line)
        if not m:
            continue
        a1, width = int(m.group(1), 16), int(m.group(2))
        if a1 >= len(rom) - 2:
            continue
        start = a1
        while start >= 2 and u16(start - 2) != 0:
            start -= 2
        end = a1
        while end < len(rom) - 2 and u16(end) != 0:
            end += 2
        calls.setdefault((start, end), set()).add(width)
    return calls


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('log', type=Path)
    parser.add_argument('rom', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    from PIL import Image, ImageDraw
    rom = args.rom.read_bytes()
    u16 = lambda a: int.from_bytes(rom[a:a + 2], 'big')
    catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
    covered = {e['address'] for e in catalog.get('glyph_text', [])} | {e['address'] for e in catalog['entries']}
    cache = {}

    def glyph(code):
        group = code >> 4
        if group not in cache:
            address = int.from_bytes(rom[GROUP_TABLE + 4 * group:GROUP_TABLE + 4 * group + 4], 'big')
            cache[group] = decode(rom, address)[0]
        return cache[group][(code & 15) * 32:(code & 15) * 32 + 32]

    rows = []
    for (start, end), widths in parse(args.log, rom).items():
        words = [u16(a) for a in range(start, end, 2)]
        if not words or len(words) > 200 or any(w >= 0x1800 for w in words):
            continue
        skip = control_cells(words)
        done = 'covered' if start in covered else 'TODO'
        print(f'{start:06X} cells={len(words)} control={skip} widths={sorted(widths)} {done}')
        rows.append((start, words[skip:], skip, done))
    height = sum(18 * max(1, (len(w) + 16) // 17) + 6 for _, w, _, _ in rows) or 1
    image = Image.new('RGB', (17 * 16 + 130, height), (0, 0, 0))
    draw = ImageDraw.Draw(image)
    y = 0
    for start, words, skip, done in rows:
        draw.text((0, y + 4), f'{start:X}+{skip} {done}', fill=(255, 200, 0) if done == 'TODO' else (120, 200, 120))
        for line in range(max(1, (len(words) + 16) // 17)):
            for k, code in enumerate(words[line * 17:(line + 1) * 17]):
                try:
                    bitmap = glyph(code)
                except Exception:
                    continue
                for yy in range(16):
                    bits = int.from_bytes(bitmap[yy * 2:yy * 2 + 2], 'big')
                    for xx in range(16):
                        if bits & (0x8000 >> xx):
                            image.putpixel((130 + k * 16 + xx, y + yy), (255, 255, 255))
            y += 18
        y += 6
    image.save(args.output)


if __name__ == '__main__':
    main()
