import unittest
from tools.db2_codec import decode, encode_literals, decode_dictionary, CodecError


class CodecTests(unittest.TestCase):
    def test_dictionary_literal_indices(self):
        packed = bytes.fromhex('000402010241420004034100')
        self.assertEqual(decode_dictionary(packed), (b'ABAB', len(packed)))

    def test_dictionary_overlapping_copy(self):
        packed = bytes.fromhex('000402010141000400410000')
        self.assertEqual(decode_dictionary(packed), (b'AAAA', len(packed)))

    def test_literal_and_overlapping_backreference(self):
        self.assertEqual(decode(b'\x00\x06\x00\x01AB\x83\x01'), (b'ABABAB', 8))

    def test_long_distance(self):
        self.assertEqual(decode(b'\x00\x04\x00\x01AB\xc1\x00\x01'), (b'ABAB', 9))

    def test_literal_encoder_roundtrip_boundaries(self):
        for count in [1, 127, 128, 129, 65535]:
            source = bytes(i % 251 for i in range(count))
            packed = encode_literals(source)
            self.assertEqual(decode(packed), (source, len(packed)))

    def test_literal_encoder_rejects_empty_and_oversized_payloads(self):
        for data in [b'', bytes(65536)]:
            with self.subTest(size=len(data)), self.assertRaises(CodecError):
                encode_literals(data)

    def test_rejects_truncated_and_invalid_backreferences(self):
        for packed in [b'', b'\x00\x00\x00', b'\x00\x01\x00', b'\x00\x01\x00\x80\x00', b'\x00\x01\x00\x01AB']:
            with self.subTest(packed=packed), self.assertRaises(CodecError):
                decode(packed)

    def test_dictionary_rejects_truncated_and_invalid_input(self):
        for packed in [
            b'',
            bytes.fromhex('000100000141'),
            bytes.fromhex('0001000105'),
            bytes.fromhex('0001000101'),
            bytes.fromhex('00010001014100'),
            bytes.fromhex('0001000101410002'),
            bytes.fromhex('000100010141000100'),
            bytes.fromhex('00010001014100010080'),
            bytes.fromhex('000100010141000100fe'),
            bytes.fromhex('0002000101410001 0000'.replace(' ', '')),
        ]:
            with self.subTest(packed=packed), self.assertRaises(CodecError):
                decode_dictionary(packed)
