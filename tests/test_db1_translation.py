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

    def test_credits_tables_use_renderer_font_and_keep_fixed_capacity(self):
        source, catalog = self.fixture()
        catalog['credits_tables'] = [dict(address=0x1c000, width=10, count=2,
            before='00' * 20, lines=['STEGO', 'P CHAN'])]
        result = translate(source, catalog)
        self.assertEqual(result[0x1c000:0x1c00a], bytes([0, 0, 30, 31, 16, 18, 26, 0, 0, 0]))
        self.assertEqual(result[0x1c00a:0x1c014], bytes([0, 0, 27, 0, 14, 19, 12, 25, 0, 0]))
        self.assertEqual(len(result), len(source))

    def test_credits_reject_guard_overflow_alphabet_and_invalid_layout(self):
        source, catalog = self.fixture()
        table = dict(address=0x1c000, width=10, count=1, before='00' * 10, lines=['STEGO'])
        for change in [dict(before='01' * 10), dict(lines=['TOO LONG NAME']),
                       dict(lines=['steGo']), dict(address=len(source)-4),
                       dict(lines=[]), dict(address=0x1b464), dict(width=0)]:
            invalid = copy.deepcopy(catalog)
            invalid['credits_tables'] = [dict(table, **change)]
            with self.assertRaises(ValueError):
                translate(source, invalid)

    def test_credits_punctuation_matches_loaded_font(self):
        source, catalog = self.fixture()
        catalog['credits_tables'] = [dict(address=0x1c000, width=3, count=1,
            before='000000', lines=['.!?'])]
        self.assertEqual(translate(source, catalog)[0x1c000:0x1c003], bytes([0x27, 0x29, 0x28]))

    def test_bonus_message_encoding_matches_distinct_vram_font_bases(self):
        source, catalog = self.fixture()
        catalog['bonus_messages'] = [dict(address=0x4000, capacity=10, before='00'*10,
            lines=['A Z!', 'B'], background_address=0x4020, background_before='00'*16,
            background='CONGRATS! GREAT!')]
        result = translate(source, catalog)
        self.assertEqual(result[0x4000:0x400a], bytes([0xa9,0x20,0xc2,0xc6,0,0xaa,0,0,0,0]))
        self.assertEqual(result[0x4020], 0x16)  # C: font tile $85 minus background base $6F
        self.assertEqual(result[0x4028], 0x31)  # !: loaded font index $27 + $79 - $6F
        self.assertEqual(result[0x4029], 0xff)  # renderer's explicit blank sentinel

    def test_bonus_messages_reject_overflow_glyphs_guards_and_overlaps(self):
        source, catalog = self.fixture()
        table = dict(address=0x4000, capacity=10, before='00'*10, lines=['HELLO'],
            background_address=0x4020, background_before='00'*16, background='CONGRATS! GREAT!')
        for change in [dict(lines=['A'*25]),dict(lines=['HELLO WORLD']),dict(lines=['Hi']),
                       dict(before='01'*10),dict(background_before='01'*16),
                       dict(background='A'*17),dict(background_address=0x4000),
                       dict(address=0x1b464),dict(lines=['']*7)]:
            invalid = copy.deepcopy(catalog)
            invalid['bonus_messages'] = [dict(table, **change)]
            with self.assertRaises(ValueError):translate(source, invalid)
