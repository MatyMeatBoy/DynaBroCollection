"""Repair the six map/menu labels on top of the qualified DB1 v0.1 patch."""
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
    out = bytearray(source)
    out[0x1b464:0x1b4d0] = b''.join(x.to_bytes(2, 'big') for x in words)
    out[0x1b510:0x1b538] = b''.join(x.to_bytes(2, 'big') for x in font)
    checksum = sum(int.from_bytes(out[i:i+2], 'big') for i in range(512, len(out), 2)) & 65535
    out[0x18e:0x190] = checksum.to_bytes(2, 'big')
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--catalog', type=Path, default=Path(__file__).resolve().parent.parent / 'translations/db1-en.json')
    parser.add_argument('--output', type=Path, default=Path('local/roms/db1-en-v02.md'))
    parser.add_argument('--patch', type=Path, default=Path('local/patches/db1-en-v02-addon.bps'))
    args = parser.parse_args()
    source = args.rom.read_bytes()
    catalog = json.loads(args.catalog.read_text(encoding='utf-8'))
    result = translate(source, catalog)
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
