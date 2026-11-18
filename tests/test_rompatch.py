import pathlib
import os
import struct
import unittest
import zlib
from tools.rompatch import apply_bps, apply_ips, PatchError, LIMIT


class PatchTests(unittest.TestCase):
    @staticmethod
    def bps(source, target, stream):
        body = b'BPS1' + bytes([0x80 | len(source), 0x80 | len(target), 0x80]) + stream
        body += struct.pack('<II', zlib.crc32(source), zlib.crc32(target))
        return body + struct.pack('<I', zlib.crc32(body))

    @staticmethod
    def varint(value):
        out = bytearray()
        while True:
            byte = value & 127
            value >>= 7
            if not value:
                out.append(byte | 128)
                return bytes(out)
            out.append(byte)
            value -= 1

    def test_bps_rejects_declared_target_size_over_the_limit(self):
        source = b'abc'
        body = b'BPS1' + self.varint(len(source)) + self.varint(LIMIT + 1) + self.varint(0)
        body += struct.pack('<II', zlib.crc32(source), 0)
        patch = body + struct.pack('<I', zlib.crc32(body))
        with self.assertRaises(PatchError):
            apply_bps(source, patch)

    def test_bps_overlapping_target_copy(self):
        patch = self.bps(b'', b'AAAA', b'\x81A\x8b\x80')
        self.assertEqual(apply_bps(b'', patch), b'AAAA')

    def test_bps_bounds_with_valid_checksums(self):
        for source, target, stream in [(b'abc', b'abc', b'\x8c'),
                                        (b'', b'A', b'\x83\x80'),
                                        (b'', b'A', b'\x82\x80'),
                                        (b'', b'A', b'\x81')]:
            with self.subTest(stream=stream), self.assertRaises(PatchError):
                apply_bps(source, self.bps(source, target, stream))

    def test_real_db1_crc_and_wrong_source(self):
        from tools.build import default_input_root
        root = pathlib.Path(os.environ.get('DYNA_TEST_INPUTS', str(default_input_root())))
        if not root.exists():
            self.skipTest('Private cartridge fixture unavailable; public CI uses synthetic fixtures')
        from tools.build import find_by_hash, DB1_SHA, PATCH_SHA
        rom = find_by_hash(root, DB1_SHA, {'.md', '.bin'}).read_bytes()
        patch = find_by_hash(root, PATCH_SHA, {'.bps'}).read_bytes()
        result = apply_bps(rom, patch)
        self.assertEqual(zlib.crc32(result), 0xC1E5C0DA)
        with self.assertRaises(PatchError):
            apply_bps(bytes(len(rom)), patch)

    def test_corrupt_bps(self):
        with self.assertRaises(PatchError):
            apply_bps(b'', b'BPS1' + bytes(20))

    @staticmethod
    def varint(value):
        out = bytearray()
        while True:
            byte = value & 127
            value >>= 7
            if not value:
                out.append(byte | 128)
                return bytes(out)
            out.append(byte)
            value -= 1

    def test_bps_rejects_truncated_integer_and_plain_source_size_mismatch(self):
        v = self.varint
        padding = v(300000)  # multi-byte, fully terminated; just pads the body length
        truncated_body = b'BPS1' + padding + bytes([0, 0, 0])  # 3 non-terminating bytes, then nothing
        truncated_body += struct.pack('<II', zlib.crc32(b''), 0)
        truncated = truncated_body + struct.pack('<I', zlib.crc32(truncated_body))

        mismatch_body = b'BPS1' + v(5) + v(0) + v(0)
        mismatch_body += struct.pack('<II', zlib.crc32(b''), 0)
        mismatch = mismatch_body + struct.pack('<I', zlib.crc32(mismatch_body))

        for name, patch in [('truncated_integer', truncated), ('source_size_mismatch', mismatch)]:
            with self.subTest(case=name), self.assertRaises(PatchError):
                apply_bps(b'', patch)

    def test_ips_rejects_source_over_the_limit(self):
        with self.assertRaises(PatchError):
            apply_ips(bytes(LIMIT + 1), b'PATCHEOF')

    def test_ips_literal_rle_and_truncation(self):
        patch = b'PATCH\x00\x00\x01\x00\x02AB\x00\x00\x04\x00\x00\x00\x03ZEOF'
        self.assertEqual(apply_ips(b'1234', patch), b'1AB4ZZZ')
        for cut in range(len(patch)):
            with self.assertRaises(PatchError):
                apply_ips(b'1234', patch[:cut])

    def test_ips_resize_and_limit(self):
        self.assertEqual(apply_ips(b'1234', b'PATCHEOF\x00\x00\x02'), b'12')
        with self.assertRaises(PatchError):
            apply_ips(b'', b'PATCH\xff\xff\xfe\x00\x02ABEOF')

    def test_ips_rejects_resize_over_the_limit(self):
        with self.assertRaises(PatchError):
            apply_ips(b'1234', b'PATCHEOF' + (LIMIT + 1).to_bytes(3, 'big'))

    def test_ips_rejects_unexplained_trailer(self):
        with self.assertRaises(PatchError):
            apply_ips(b'123', b'PATCHEOF\x00')


if __name__ == '__main__':
    unittest.main()
