"""Build the DB2 Special English BPS for a standalone NTSC Mega Drive ROM."""

import argparse
import json
from itertools import batched
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))

from tools.db2_codec import decode, decode_dictionary, encode_literals
from tools.generate_db2_glyph_text import centered, control_cells
from tools.rompatch import apply_bps, create_bps, identity
from tools.translate_db2 import translate

ORIGIN = 0x25D300
HOOK = 0x5B124


def require(condition, message):
    if not condition:
        raise ValueError(message)


def title_tiles(rom):
    source = 0x46030
    plane, _ = decode_dictionary(rom, source)
    changed = bytearray(plane)
    require(len(plane) == 8192, 'Unexpected title pattern bank size')
    description = (
        ('PRACTICE', 10, '199 176 177 178 179 17a 17b 17c 17d 17e', '185 186 187 188 189 18a 18b 18c 18d 18e'),
        ('STORY', 14, '1db 1dc 1dd 1de 1df 1e0 1e1 1e2 1df 1e0', '1eb 1ec 1ed 1ee 1ef 1f0 1f1 1f2 1ef 1f0'),
        ('ORIGINAL', 15, '1e3 1e4 1f5 1f6 1f7 1f8 1f9 1fa 1fb 1fc', '1f3 1f4 205 206 207 208 209 20a 20b 20c'),
        ('VERSUS', 13, '1c3 1c4 1d5 1d6 1d7 1d8 1d9 1da', '1d3 1d4 1e5 1e6 1e7 1e8 1e9 1ea'),
    )
    title_code = (ROOT / 'games/db2_menu_labels.c').read_text(encoding='utf-8')
    letters = re.search(r'glyph_letters\[\]="([A-Z]+)"', title_code)[1]
    glyphs = re.findall(r'"([.c]{16})"', title_code)
    require(len(glyphs) == len(letters) * 14, 'Unexpected title font layout')
    font = {letter: glyphs[i * 14:(i + 1) * 14] for i, letter in enumerate(letters)}
    updates = {}

    for word, ink, upper, lower in description:
        tiles = [[int(item, 16) for item in row.split()] for row in (upper, lower)]
        width = len(tiles[0]) * 8
        char_width = 6 if word == 'STORY' else 8
        center = 1 if word == 'STORY' else (width - len(word) * char_width) // 2
        mask = set()
        for i, letter in enumerate(word):
            for y, row in enumerate(font[letter], 1):
                for x in range(char_width):
                    if 'c' in row[x * 16 // char_width:(x + 1) * 16 // char_width]:
                        mask.add((center + i * char_width + x, y))
        for part, tile_row in enumerate(tiles):
            for column, tile in enumerate(tile_row):
                pixels = []
                for y in range(8 * part, 8 * part + 8):
                    for x in range(8 * column, 8 * column + 8):
                        outline = any((x + dx, y + dy) in mask for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)))
                        pixels.append(ink if (x, y) in mask else 1 if outline else 2)
                packed = bytes(left << 4 | right for left, right in batched(pixels, 2))
                require(updates.setdefault(tile, packed) == packed, f'Conflicting {word} tile {tile:X}')
    for tile in (*range(0x17F, 0x185), *range(0x18F, 0x195)):
        updates[tile] = b'\x22' * 32

    for tile, pixels in updates.items():
        offset = (tile - 0x175) * 32
        require(0 <= offset <= len(changed) - 32, f'Title tile {tile:X} outside pattern bank')
        changed[offset:offset + 32] = pixels
    stream = bytearray(encode_literals(changed))
    stream[2] = 3  # Original FF10 dispatcher entry for the extended LZ format.
    require(decode(stream) == (changed, len(stream)), 'Title tile encoding roundtrip failed')
    return stream


def font_data(rom):
    raw, _ = decode_dictionary(rom, 0x10730)
    require(len(raw) == 5440, 'Unexpected menu font size')
    glyphs = bytearray(128 * 8)
    for char in range(33, 127):
        code = ord(chr(char).upper())
        tile = (int.from_bytes(rom[0xFCB6 + 2 * code:0xFCB8 + 2 * code], 'big') & 0x7FF) * 32
        if 0x40 <= tile < 0xEA0:
            offset = tile - 0x40
        elif 0xF000 <= tile < 0xF6E0:
            offset = 0xE60 + tile - 0xF000
        else:
            continue
        for row in range(8):
            word = raw[offset + 4 * row:offset + 4 * row + 4].hex()
            glyphs[char * 8 + row] = int(''.join('0' if ink == '0' else '1' for ink in word), 2)
    return glyphs


def data_source(catalog, biographies, rom):
    ranges, lines_source = {}, []
    font = font_data(rom)

    def bitmaps(line):
        for group in batched(line.ljust((len(line) + 2) // 3 * 3), 3):
            rows = bytearray(32)
            for y in range(8):
                row = 0
                for char in group:
                    source = font[ord(char) * 8 + y]
                    for mask in (0xc0, 0x20, 0x18, 0x04, 0x03):
                        row = (row << 1) | bool(source & mask)
                rows[8 + 2 * y:10 + 2 * y] = (row << 1).to_bytes(2, 'big')
            yield rows

    def add_lines(label, lines, prefix='', narrow=False):
        labels = [f'{label}_line{i}' for i in range(len(lines))]
        lines_source.extend(('        even', f'{label}:', prefix, f'        dc.w {len(lines) | (0x8000 if narrow else 0)}',
                             '        dc.l ' + ','.join(labels)))
        for line_label, line in zip(labels, lines):
            if narrow:
                cells = list(bitmaps(line))
                lines_source.extend(('        even', f'{line_label}: dc.w {len(cells)}'))
                lines_source.extend('        dc.b ' + ','.join(map(str, cell)) for cell in cells)
            else:
                text = line.encode('ascii')
                lines_source.append(f'{line_label}: dc.b ' + ','.join(map(str, bytes([len(text)]) + text)))

    for entry in sorted(catalog['glyph_text'], key=lambda row: row['address']):
        start = entry['address'] + 2 * control_cells(entry['before'])
        end = entry['address'] + len(bytes.fromhex(entry['before']))
        if start >= end:
            continue
        n = len(ranges)
        ranges[start] = f'        dc.l ${start:06x},${end:06x},string{n}'
        lines = entry['lines']
        if entry.get('align') == 'center':
            lines = [centered(line, entry) for line in lines]
        add_lines(f'string{n}', lines)

    # The biography renderer calls the same cell routine with three fixed
    # rows of nine cells. Preserve the original allocation and live palette.
    for i, field in enumerate(biographies):
        start = field['address']
        lines = field['lines']
        guard = bytes.fromhex(field['before'])
        end = start + len(guard)
        require(start not in ranges and rom[start:end + 2] == guard + bytes(2),
                f'Biography guard mismatch at {start:06X}')
        original_cells = len(guard) // 2
        stride = 9 if len(lines) == 3 else original_cells
        for row, line in enumerate(lines):
            require((len(line) + 2) // 3 <= min(stride, original_cells - row * stride),
                    f'Biography exceeds native width at {start:06X}')
        tag = f'bio{i}'
        ranges[start] = f'        dc.l ${start:06x},${end:06x},{tag}'
        add_lines(tag, lines, narrow=True)

    buckets = [[] for _ in range(256)]
    for n, entry in enumerate(catalog['ram_text']):
        words = [int(word, 16) for word in entry['words'].split()]
        head = [1 if w == 2 else w for w in words[:6]] + [0xffff] * max(0, 6 - len(words))
        skip = entry.get('skip', -1)
        if skip < 0:
            skip = control_cells(''.join(f'{word:04x}' for word in words))
        tag = f'ram_string{n}'
        buckets[head[0] & 255].append(tag)
        add_lines(tag, entry['lines'], '        dc.w ' + ','.join(f'${w:04x}' for w in (*head, len(words), skip)))
    bucket_source = ['        even', 'ram_buckets:']
    for i, bucket in enumerate(buckets):
        bucket_source.append('        dc.l ' + (f'ram_bucket{i}' if bucket else '0'))
    for i, bucket in enumerate(buckets):
        if bucket:
            bucket_source.append(f'ram_bucket{i}: dc.l ' + ','.join((*bucket, '0')))
    source = '\n'.join([f'glyph_range_count equ {len(ranges)}', 'glyph_ranges:',
                        *(ranges[i] for i in sorted(ranges)), *bucket_source, *lines_source,
                        'font_ascii:', '        incbin "font.bin"', ''])
    return source, font


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--assembler', required=True, help='Path to vasmm68k_mot')
    parser.add_argument('--patch', type=Path, default=ROOT / 'local/patches/db2special-en-console.bps')
    parser.add_argument('--rom-output', type=Path, help='Optional locally patched ROM')
    args = parser.parse_args()
    assembler = shutil.which(args.assembler) or str(Path(args.assembler).resolve())
    rom = args.rom.read_bytes()
    catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
    result = bytearray(translate(rom, catalog, native_only=True))
    biographies = json.loads((ROOT / 'translations/db2special-bios-en.json').read_text(encoding='utf-8'))
    source, font = data_source(catalog, biographies, rom)
    with TemporaryDirectory(prefix='dyna-db2-console-') as directory:
        build = Path(directory)
        (build / 'rom-glyph-data.inc').write_text(source, encoding='ascii')
        (build / 'font.bin').write_bytes(font)
        shutil.copyfile(ROOT / 'platform/megadrive/db2_glyphs.s', build / 'rom-glyph.s')
        subprocess.run([assembler, '-m68000', '-Fbin', '-o', 'rom-glyph.bin', 'rom-glyph.s'], cwd=build, check=True)
        payload = (build / 'rom-glyph.bin').read_bytes()
    require(rom[ORIGIN:ORIGIN + len(payload)] == b'\xff' * len(payload), 'Native glyph payload exceeds reserved ROM')
    require(result[HOOK:HOOK + 6] == bytes.fromhex('30110c400001'), 'Unexpected glyph renderer entry')
    result[HOOK:HOOK + 6] = bytes.fromhex('4ef9') + ORIGIN.to_bytes(4, 'big')
    require(result[0x5b0fe:0x5b102] == bytes.fromhex('4e560000'), 'Unexpected glyph renderer stack frame')
    result[0x5b100:0x5b102] = bytes.fromhex('fff8')
    result[ORIGIN:ORIGIN + len(payload)] = payload
    tiles = title_tiles(rom)
    title_at = (ORIGIN + len(payload) + 1) & ~1
    require(result[title_at:title_at + len(tiles)] == b'\xff' * len(tiles), 'Title pattern bank exceeds reserved ROM')
    require(result[0x445c2:0x445c6] == (0x46030).to_bytes(4, 'big'), 'Unexpected title pattern pointer')
    result[0x445c2:0x445c6] = title_at.to_bytes(4, 'big')
    result[title_at:title_at + len(tiles)] = tiles
    checksum = sum(word[0] for word in struct.iter_unpack('>H', result[512:])) & 0xffff
    result[0x18e:0x190] = checksum.to_bytes(2, 'big')
    patch = create_bps(rom, result)
    require(apply_bps(rom, patch) == result, 'Console BPS roundtrip failed')
    files = [(args.patch, patch)]
    if args.rom_output:
        files.append((args.rom_output, result))
    for path, data in files:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as output:
            output.write(data)
    print(json.dumps(dict(source=identity(rom), target=identity(result), patch=identity(patch),
                          text_entries=len(catalog['entries']), rom_glyph_entries=len(catalog['glyph_text']),
                          dialogue_entries=len(catalog['ram_text']), biography_fields=len(biographies),
                          title_labels=4), indent=2))


if __name__ == '__main__':
    main()
