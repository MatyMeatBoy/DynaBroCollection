import json
from pathlib import Path
import unittest
from tools.generate_db2_glyph_text import render, patched_words, fnv32, c_string

ROOT = Path(__file__).resolve().parent.parent


class GenerateDb2GlyphTextTests(unittest.TestCase):
    def test_renders_a_sorted_row_with_patched_fnv_and_c_strings(self):
        catalog = {'glyph_text': [{'address': 0x2000, 'before': '0080', 'lines': ['B']},
                                   {'address': 0x1000, 'before': '00010080', 'lines': ['A']}]}
        text = render(catalog)
        self.assertLess(text.index('0x001000'), text.index('0x002000'))
        self.assertIn('0x%08Xu' % fnv32(patched_words('00010080')), text)
        self.assertIn('{"A"}', text)

    def test_patched_words_replaces_blank_cells_with_the_placeholder(self):
        self.assertEqual(patched_words('00010080'), bytes.fromhex('00020080'))

    def test_c_string_rejects_non_ascii_and_control_or_quote_characters(self):
        for bad in ['h\xe9llo', 'a\nb', 'a"b', 'a\\b']:
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                c_string(bad)

    def test_rejects_invalid_entry_size_and_line_count(self):
        cases = [
            {'address': 0, 'before': '00', 'lines': ['A']},
            {'address': 0, 'before': '', 'lines': ['A']},
            {'address': 0, 'before': '00' * 2 * 65, 'lines': ['A']},
            {'address': 0, 'before': '0080', 'lines': []},
            {'address': 0, 'before': '0080', 'lines': ['A'] * 7},
        ]
        for entry in cases:
            with self.subTest(entry=entry), self.assertRaises(ValueError):
                render({'glyph_text': [entry]})

    def test_committed_glyph_table_matches_the_real_catalog(self):
        catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
        expected = (ROOT / 'games/db2_glyph_text_table.inc').read_text(encoding='utf-8')
        self.assertEqual(render(catalog), expected.replace('\r\n', '\n'))
