"""Bounded ROM patch decoding. IPS output alone does not qualify compatibility."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import zlib

LIMIT = 8 * 1024 * 1024


class PatchError(ValueError):
    pass


def crc(data):
    return zlib.crc32(data) & 0xffffffff


def apply_bps(source: bytes, patch: bytes) -> bytes:
    if len(patch) < 19 or patch[:4] != b'BPS1':
        raise PatchError('Invalid BPS header')
    source_crc, target_crc, patch_crc = struct.unpack('<III', patch[-12:])
    if crc(patch[:-4]) != patch_crc or crc(source) != source_crc:
        raise PatchError('BPS source or patch CRC mismatch')
    pos, end = 4, len(patch) - 12

    def number():
        nonlocal pos
        value, shift = 0, 1
        for _ in range(10):
            if pos >= end:
                raise PatchError('Truncated BPS integer')
            x = patch[pos]
            pos += 1
            value += (x & 127) * shift
            if x & 128:
                return value
            shift <<= 7
            value += shift
        raise PatchError('Oversized BPS integer')

    source_size, target_size, metadata_size = number(), number(), number()
    if source_size != len(source) or target_size > LIMIT:
        raise PatchError('BPS size mismatch or output limit exceeded')
    pos += metadata_size
    if pos > end:
        raise PatchError('Truncated BPS metadata')
    out = bytearray()
    source_offset = target_offset = 0
    while len(out) < target_size:
        action = number()
        count, kind = (action >> 2) + 1, action & 3
        if len(out) + count > target_size:
            raise PatchError('BPS action exceeds target')
        if kind == 0:
            start = len(out)
            if start + count > len(source):
                raise PatchError('BPS source read out of bounds')
            out.extend(source[start:start + count])
        elif kind == 1:
            if pos + count > end:
                raise PatchError('Truncated BPS literal')
            out.extend(patch[pos:pos + count])
            pos += count
        else:
            relative = number()
            delta = -(relative >> 1) if relative & 1 else relative >> 1
            if kind == 2:
                source_offset += delta
                if source_offset < 0 or source_offset + count > len(source):
                    raise PatchError('BPS source copy out of bounds')
                out.extend(source[source_offset:source_offset + count])
                source_offset += count
            else:
                target_offset += delta
                if target_offset < 0 or target_offset >= len(out):
                    raise PatchError('BPS target copy out of bounds')
                for _ in range(count):
                    out.append(out[target_offset])
                    target_offset += 1
    if pos != end or crc(out) != target_crc:
        raise PatchError('BPS target CRC or stream length mismatch')
    return bytes(out)


def create_bps(source: bytes, target: bytes) -> bytes:
    """Build a CRC-qualified delta using source reads and literal target runs."""
    if len(source) > LIMIT or len(target) > LIMIT:
        raise PatchError('BPS image limit exceeded')
    out = bytearray(b'BPS1')

    def number(value):
        while True:
            byte = value & 127
            value >>= 7
            if not value:
                out.append(byte | 128)
                return
            out.append(byte)
            value -= 1

    number(len(source))
    number(len(target))
    number(0)
    pos = 0
    while pos < len(target):
        same = pos < len(source) and source[pos] == target[pos]
        end = pos + 1
        while end < len(target) and (end < len(source) and source[end] == target[end]) == same:
            end += 1
        number(((end - pos - 1) << 2) | (0 if same else 1))
        if not same:
            out.extend(target[pos:end])
        pos = end
    out.extend(struct.pack('<II', crc(source), crc(target)))
    out.extend(struct.pack('<I', crc(out)))
    return bytes(out)


def ips_records(patch: bytes):
    if patch[:5] != b'PATCH':
        raise PatchError('Invalid IPS header')
    pos = 5
    records = []
    while True:
        if pos + 3 > len(patch):
            raise PatchError('Missing IPS EOF')
        raw = patch[pos:pos + 3]
        pos += 3
        if raw == b'EOF':
            break
        address = int.from_bytes(raw, 'big')
        if pos + 2 > len(patch):
            raise PatchError('Truncated IPS record')
        count = int.from_bytes(patch[pos:pos + 2], 'big')
        pos += 2
        if count:
            if pos + count > len(patch):
                raise PatchError('Truncated IPS literal')
            payload = patch[pos:pos + count]
            pos += count
        else:
            if pos + 3 > len(patch):
                raise PatchError('Truncated IPS RLE')
            count = int.from_bytes(patch[pos:pos + 2], 'big')
            payload = patch[pos + 2:pos + 3] * count
            pos += 3
            if not count:
                raise PatchError('Empty IPS RLE')
        if address + count > LIMIT:
            raise PatchError('IPS output limit exceeded')
        records.append((address, payload))
    tail = patch[pos:]
    if len(tail) not in (0, 3):
        raise PatchError('Invalid IPS trailer')
    size = int.from_bytes(tail, 'big') if tail else None
    if size is not None and size > LIMIT:
        raise PatchError('IPS resize exceeds limit')
    return records, size


def apply_ips(source: bytes, patch: bytes) -> bytes:
    if len(source) > LIMIT:
        raise PatchError('Source exceeds limit')
    records, size = ips_records(patch)
    out = bytearray(source)
    for address, payload in records:
        needed = address + len(payload)
        if needed > len(out):
            out.extend(bytes(needed - len(out)))
        out[address:needed] = payload
    if size is not None:
        del out[size:]
        if len(out) < size:
            out.extend(bytes(size - len(out)))
    return bytes(out)


def identity(data):
    return dict(size=len(data), crc32=f'{crc(data):08X}', sha256=hashlib.sha256(data).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('patch', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--source-sha256', required=True)
    parser.add_argument('--experimental-ips', action='store_true', help='Acknowledge IPS has no source validation; output is unqualified')
    args = parser.parse_args()
    source, patch = args.source.read_bytes(), args.patch.read_bytes()
    if identity(source)['sha256'] != args.source_sha256.lower():
        parser.error('Source SHA256 mismatch')
    if args.output.resolve() in (args.source.resolve(), args.patch.resolve()):
        parser.error('Output must not overwrite input')
    if patch[:4] == b'BPS1':
        output = apply_bps(source, patch)
        qualification = 'binary-verified; gameplay-unverified'
    elif patch[:5] == b'PATCH' and args.experimental_ips:
        output = apply_ips(source, patch)
        qualification = 'experimental; edition-compatibility-unverified'
    else:
        parser.error('IPS requires --experimental-ips; otherwise use BPS')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation prevents replacing earlier outputs and original files.
    with args.output.open('xb') as stream:
        stream.write(output)
    report = dict(source=identity(source), patch=identity(patch), output=identity(output), qualification=qualification)
    args.output.with_suffix(args.output.suffix + '.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
