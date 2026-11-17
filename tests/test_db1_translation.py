import copy
import hashlib
import unittest

from tools.translate_db1 import translate


class Db1TranslationTests(unittest.TestCase):
    def fixture(self):
        source = bytes(0x20000)
        return source, dict(source_sha256=hashlib.sha256(source).hexdigest(),
            labels_before='00' * 108, font_before='00' * 40,
            labels=['CREATURES', 'LAND MAP', 'EGG MAP', 'SURRENDER', 'NO', 'REALLY?'])

    def test_preserves_layout_and_changes_only_qualified_tables_and_checksum(self):
        source, catalog = self.fixture()
        result = translate(source, catalog)
        self.assertEqual(len(result), len(source))
        allowed = set(range(0x1b464, 0x1b4d0)) | set(range(0x1b510, 0x1b538)) | {0x18e, 0x18f}
        self.assertTrue(all(a == b or i in allowed for i, (a, b) in enumerate(zip(source, result))))
        labels = [int.from_bytes(result[i:i+2], 'big') for i in range(0x1b464, 0x1b4d0, 2)]
        self.assertTrue(all(2 <= x <= 22 for x in labels))
        self.assertEqual(labels[17], 2)  # padding is an existing blank tile
        words = [int.from_bytes(result[i:i+2], 'big') for i in range(0x200, len(result), 2)]
        self.assertEqual(sum(words) & 65535, int.from_bytes(result[0x18e:0x190], 'big'))

    def test_rejects_wrong_image_guard_long_labels_and_unsupported_letters(self):
        source, catalog = self.fixture()
        for key, value in [('source_sha256', 'bad'), ('font_before', '01' * 40),
                           ('labels', ['TOO LONG LABEL'] * 6), ('labels', ['Z'] * 6)]:
            invalid = copy.deepcopy(catalog)
            invalid[key] = value
            with self.assertRaises(ValueError):
                translate(source, invalid)
