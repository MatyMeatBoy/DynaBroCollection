"""Repair guarded map and credits text on top of the qualified DB1 v0.1 patch."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.rompatch import apply_bps, create_bps, identity

SYMBOLS = 'ACDEGLMNOPRSTUY?'
FONT_INDEX = {letter: ord(letter) - ord('A') + 10 for letter in SYMBOLS if letter != '?'}
FONT_INDEX['?'] = 47
CREDITS_GLYPHS = {chr(65+i): 0x0c+i for i in range(26)}
CREDITS_GLYPHS.update({' ': 0, '.': 0x27, '?': 0x28, '!': 0x29, "'": 0x31})


def credits_edits(source, tables):
    """The credits renderer writes byte-sized tile indices directly to VRAM."""
    alphabet = CREDITS_GLYPHS
    edits = []
    occupied = [(0x18e, 0x190), (0x1b464, 0x1b4d0), (0x1b510, 0x1b538)]
    for table in tables:
        address, width, count = table['address'], table['width'], table['count']
        if (type(address) is not int or type(width) is not int or type(count) is not int
                or not 1 <= width <= 40 or not 1 <= count <= 100):
            raise ValueError('Invalid DB1 credits table dimensions')
        length = width * count
        if (address < 0x200 or address > len(source)-length
                or any(address < end and address+length > start for start, end in occupied)):
            raise ValueError('DB1 credits table is outside ROM or overlaps another edit')
        before = bytes.fromhex(table['before'])
        if len(before) != length or source[address:address+length] != before:
            raise ValueError('DB1 credits table guard mismatch')
        if len(table['lines']) != count:
            raise ValueError('DB1 credits line count mismatch')
        data = bytearray()
        for line in table['lines']:
            if len(line) > width or any(char not in alphabet for char in line):
                raise ValueError('Unsupported credits character or line exceeds fixed width')
            left = (width-len(line)) // 2
            text = ' '*left + line + ' '*(width-left-len(line))
            data.extend(alphabet[char] for char in text)
        edits.append((address, bytes(data)))
        occupied.append((address, address+length))
    return edits


def bonus_edits(source, tables, previous_edits):
    """Original bonus renderer uses different VRAM bases for body and backdrop."""
    edits = []
    occupied = [(0x18e, 0x190), (0x1b464, 0x1b4d0), (0x1b510, 0x1b538)]
    occupied.extend((address, address+len(data)) for address, data in previous_edits)

    def add(address, before, data):
        length = len(data)
        if (type(address) is not int or not length or address < 0x200
                or address > len(source)-length
                or any(address < end and address+length > start for start, end in occupied)):
            raise ValueError('DB1 bonus table is outside ROM or overlaps another edit')
        guard = bytes.fromhex(before)
        if len(guard) != length or source[address:address+length] != guard:
            raise ValueError('DB1 bonus table guard mismatch')
        edits.append((address, data));occupied.append((address, address+length))

    for table in tables:
        capacity, lines = table['capacity'], table['lines']
        if type(capacity) is not int or not 1 <= capacity <= 512 or not 1 <= len(lines) <= 6:
            raise ValueError('Invalid DB1 bonus message dimensions')
        data = bytearray()
        for line in lines:
            if not 1 <= len(line) <= 24 or any(char not in CREDITS_GLYPHS for char in line):
                raise ValueError('Unsupported bonus character or row exceeds 24 tiles')
            data.extend(0x20 if char == ' ' else CREDITS_GLYPHS[char]+0x9d for char in line)
            data.append(0)
        data.append(0)  # Original renderer terminates on the second NUL.
        if len(data) > capacity:
            raise ValueError('DB1 bonus message exceeds original byte capacity')
        add(table['address'], table['before'], bytes(data).ljust(capacity, b'\0'))
        background = table['background']
        if len(background) != 16 or any(char not in CREDITS_GLYPHS for char in background):
            raise ValueError('DB1 bonus background requires sixteen qualified tiles')
        data = bytes(0xff if char == ' ' else CREDITS_GLYPHS[char]+8 for char in background)
        add(table['background_address'], table['background_before'], data)
    return edits


def translate(source, catalog):
    if hashlib.sha256(source).hexdigest() != catalog['source_sha256']:
        raise ValueError('DB1 English v0.1 source SHA256 mismatch')
    if len(source) < 0x1b538 or len(source) & 1:
        raise ValueError('Invalid DB1 layout')
    for name, address, length in [('labels', 0x1b464, 108), ('font', 0x1b510, 40)]:
        before = bytes.fromhex(catalog[name + '_before'])
        if len(before) != length or source[address:address + length] != before:
            raise ValueError('DB1 ' + name + ' guard mismatch')
    if len(catalog['labels']) != 6:
        raise ValueError('Expected six fixed-width map labels')
    indices = {letter: index + 3 for index, letter in enumerate(SYMBOLS)}
    indices[' '] = 2  # Existing cleared tile; preserve the original first three slots.
    words = []
    for label in catalog['labels']:
        if len(label) > 9 or any(char not in indices for char in label):
            raise ValueError('Unsupported letter or label exceeds nine tiles')
        words.extend(indices[letter] for letter in label.ljust(9))
    font = [FONT_INDEX[letter] * 32 for letter in SYMBOLS]
    font.extend([0] * (20 - len(font)))
    edits = credits_edits(source, catalog.get('credits_tables', []))
    edits.extend(bonus_edits(source, catalog.get('bonus_messages', []), edits))
    out = bytearray(source)
    out[0x1b464:0x1b4d0] = b''.join(x.to_bytes(2, 'big') for x in words)
    out[0x1b510:0x1b538] = b''.join(x.to_bytes(2, 'big') for x in font)
    for address, data in edits:
        out[address:address+len(data)] = data
    checksum = sum(int.from_bytes(out[i:i+2], 'big') for i in range(512, len(out), 2)) & 65535
    out[0x18e:0x190] = checksum.to_bytes(2, 'big')
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--catalog', type=Path, default=Path(__file__).resolve().parent.parent / 'translations/db1-en.json')
    parser.add_argument('--output', type=Path, help='Defaults to a content-qualified filename under local/roms')
    parser.add_argument('--patch', type=Path, help='Defaults to a content-qualified add-on filename under local/patches')
    args = parser.parse_args()
    source = args.rom.read_bytes()
    catalog = json.loads(args.catalog.read_text(encoding='utf-8'))
    result = translate(source, catalog)
    crc = identity(result)['crc32']
    args.output = args.output or Path(f'local/roms/db1-en-{crc}.md')
    args.patch = args.patch or Path(f'local/patches/db1-en-{crc}-addon.bps')
    patch = create_bps(source, result)
    if apply_bps(source, patch) != result:
        raise RuntimeError('DB1 BPS roundtrip mismatch')
    for path, data in [(args.output, result), (args.patch, patch)]:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as stream:
            stream.write(data)
    report = dict(scope=catalog['scope'], source=identity(source), output=identity(result), patch=identity(patch))
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    header = Path(__file__).resolve().parent.parent / 'generated/db1step/translation_identity_db1.h'
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text('#pragma once\n#define DYNA_DB1_ENGLISH_CRC 0x' + report['output']['crc32'] + 'u\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
