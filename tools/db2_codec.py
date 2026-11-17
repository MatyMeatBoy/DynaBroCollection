"""Bounded codec recovered from DB2 Special ROM $000FDE-$00107E.

Big-endian output length, one skipped byte, then literal/LZ commands.
This module does not locate, translate, or qualify game text by itself.
"""
import argparse
from pathlib import Path


class CodecError(ValueError):
    pass


def decode(data: bytes, address: int = 0) -> tuple[bytes, int]:
    start = address
    if address < 0 or address + 3 > len(data):
        raise CodecError('Truncated header')
    length = int.from_bytes(data[address:address + 2], 'big')
    address += 3
    if not length:
        raise CodecError('Zero length not qualified')
    out = bytearray()

    def byte():
        nonlocal address
        if address >= len(data):
            raise CodecError('Truncated command')
        value = data[address]
        address += 1
        return value

    while len(out) < length:
        token = byte()
        count = (token & 63) + 1 if token & 128 else token + 1
        if len(out) + count > length:
            raise CodecError('Command exceeds declared size')
        if token & 128:
            distance = byte()
            if token & 64:
                distance = (distance << 8) | byte()
            distance += 1
            if distance > len(out) or distance > 32768:
                raise CodecError('Invalid backreference')
            for _ in range(count):
                out.append(out[-distance])
        else:
            for _ in range(count):
                out.append(byte())
    return bytes(out), address - start


def encode_literals(data: bytes) -> bytes:
    if not 1 <= len(data) <= 65535:
        raise CodecError('Payload must have 1..65535 bytes')
    out = bytearray(len(data).to_bytes(2, 'big') + b'\x00')
    for start in range(0, len(data), 128):
        chunk = data[start:start + 128]
        out.append(len(chunk) - 1)
        out.extend(chunk)
    return bytes(out)


def decode_dictionary(data: bytes, address: int = 0) -> tuple[bytes, int]:
    """Dictionary/bitstream variant recovered from ROM $1080-$11B0."""
    start = address
    if address < 0 or address + 5 > len(data):
        raise CodecError('Truncated dictionary header')
    length = int.from_bytes(data[address:address + 2], 'big')
    blocks, dictionary_size = data[address + 3], data[address + 4] or 256
    address += 5
    if not length or not blocks or address + dictionary_size > len(data):
        raise CodecError('Invalid dictionary header')
    dictionary = data[address:address + dictionary_size]
    address += dictionary_size
    out = bytearray()
    # Exact index groups from the original ROM's table at $FC2.
    groups = [(0, 0), (3, 1), (4, 9), (5, 25), (6, 64), (7, 128), (3, 57)]
    for _ in range(blocks):
        if address + 2 > len(data):
            raise CodecError('Truncated block size')
        block_length = int.from_bytes(data[address:address + 2], 'big')
        address += 2
        end = len(out) + block_length
        if not block_length or end > length:
            raise CodecError('Block exceeds output size')
        bits_left = current = 0

        def bits(count):
            nonlocal address, bits_left, current
            value = 0
            for _ in range(count):
                if not bits_left:
                    if address >= len(data):
                        raise CodecError('Truncated bitstream')
                    current = data[address]
                    address += 1
                    bits_left = 8
                bits_left -= 1
                value = (value << 1) | ((current >> bits_left) & 1)
            return value

        while len(out) < end:
            token = bits(8)
            count = (token & 63) + 1 if token & 128 else token + 1
            if len(out) + count > end:
                raise CodecError('Command exceeds block')
            if token & 128:
                distance = bits(8)
                if token & 64:
                    distance = (distance << 2) | bits(2)
                distance += 1
                if distance > len(out):
                    raise CodecError('Invalid dictionary backreference')
                for _ in range(count):
                    out.append(out[-distance])
            else:
                for _ in range(count):
                    group = 0
                    while bits(1):
                        group += 1
                        if group >= len(groups):
                            raise CodecError('Invalid dictionary index group')
                    width, base = groups[group]
                    index = base + bits(width)
                    if index >= len(dictionary):
                        raise CodecError('Dictionary index out of bounds')
                    out.append(dictionary[index])
    if len(out) != length:
        raise CodecError('Dictionary output length mismatch')
    return bytes(out), address - start


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--address', type=lambda value: int(value, 0), default=0)
    args = parser.parse_args()
    data, used = decode(args.input.read_bytes(), args.address)
    with args.output.open('xb') as out:
        out.write(data)
    print(f'{used} compressed bytes -> {len(data)} bytes; output remains unqualified until validated in game')


if __name__ == '__main__':
    main()
