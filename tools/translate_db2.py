"""Generate the scoped Special English patch without applying the retail IPS."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.rompatch import create_bps, apply_bps, identity


def translate(source: bytes, catalog: dict) -> bytes:
    if hashlib.sha256(source).hexdigest() != catalog['source_sha256']:
        raise ValueError('Translation source SHA256 mismatch')
    if len(source) < 512 or len(source) & 1:
        raise ValueError('Invalid Mega Drive image size')
    edits, used = [], set()
    for entry in catalog['entries']:
        address, capacity = entry['address'], entry['capacity']
        before = bytes.fromhex(entry['before'])
        english = entry['english'].encode('ascii')
        if address < 512 or capacity != len(before) or address + capacity > len(source):
            raise ValueError('Translation edit out of bounds')
        if source[address:address + capacity] != before:
            raise ValueError(f'Original text guard failed at {address:06X}')
        if len(english) != capacity or any(x < 32 and x != 1 for x in english):
            raise ValueError('Text must exactly fill its guarded slot and contain no unsupported controls')
        positions = set(range(address, address + capacity))
        if positions & used:
            raise ValueError('Overlapping translation edits')
        used |= positions
        edits.append((address, english))
    out = bytearray(source)
    for address, payload in edits:
        out[address:address + len(payload)] = payload
    checksum = sum(struct.unpack('>' + str((len(out) - 512) // 2) + 'H', out[512:])) & 65535
    out[0x18e:0x190] = checksum.to_bytes(2, 'big')
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--catalog', type=Path, default=Path(__file__).resolve().parent.parent / 'translations/db2special-en.json')
    parser.add_argument('--output', type=Path, default=Path('local/roms/db2special-en-prompts.md'))
    parser.add_argument('--patch', type=Path, default=Path('local/patches/db2special-en-prompts.bps'))
    args = parser.parse_args()
    source = args.rom.read_bytes()
    catalog = json.loads(args.catalog.read_text(encoding='utf-8'))
    output = translate(source, catalog)
    patch = create_bps(source, output)
    if apply_bps(source, patch) != output:
        raise RuntimeError('BPS roundtrip mismatch')
    for path, data in [(args.output, output), (args.patch, patch)]:
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open('xb') as stream:
            stream.write(data)
    report = dict(scope=catalog['scope'], entries=len(catalog['entries']), source=identity(source), output=identity(output),
                  patch=identity(patch), verification='binary guards, checksum and BPS roundtrip; scene validation pending')
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    header = Path(__file__).resolve().parent.parent / 'generated/db2overlay/translation_identity.h'
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text('#pragma once\n#define DYNA_DB2_ENGLISH_CRC 0x' + report['output']['crc32'] + 'u\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
