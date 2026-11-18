import unittest
from tools.rompatch import apply_bps, create_bps, PatchError, LIMIT


class WriterTests(unittest.TestCase):
    def test_roundtrip_replacement_growth_and_shrink(self):
        for source, target in [(b'abcDEFghi', b'abcXYZghi'), (b'abc', b'abc123'),
                               (b'abc123', b'abc'), (b'', b'abc'), (b'abc', b''),
                               (bytes(range(256)), bytes(range(255, -1, -1)))]:
            with self.subTest(source=source, target=target):
                self.assertEqual(apply_bps(source, create_bps(source, target)), target)

    def test_create_bps_rejects_images_over_the_size_limit(self):
        oversized = bytes(LIMIT + 1)
        with self.assertRaises(PatchError):
            create_bps(oversized, b'')
        with self.assertRaises(PatchError):
            create_bps(b'', oversized)
