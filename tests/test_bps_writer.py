import unittest
from tools.rompatch import apply_bps, create_bps


class WriterTests(unittest.TestCase):
    def test_roundtrip_replacement_growth_and_shrink(self):
        for source, target in [(b'abcDEFghi', b'abcXYZghi'), (b'abc', b'abc123'),
                               (b'abc123', b'abc'), (b'', b'abc'), (b'abc', b''),
                               (bytes(range(256)), bytes(range(255, -1, -1)))]:
            with self.subTest(source=source, target=target):
                self.assertEqual(apply_bps(source, create_bps(source, target)), target)
