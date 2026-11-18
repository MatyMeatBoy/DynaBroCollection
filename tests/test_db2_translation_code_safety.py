import json
from pathlib import Path
import tomllib
import unittest

ROOT = Path(__file__).resolve().parent.parent


class Db2TranslationCodeSafetyTests(unittest.TestCase):
    def test_catalog_text_ranges_never_overlap_known_code_addresses(self):
        catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
        game = tomllib.loads((ROOT / 'games/db2special/game.toml').read_text(encoding='utf-8'))
        flines = tomllib.loads((ROOT / 'games/db2special/flines.toml').read_text(encoding='utf-8'))
        code_addresses = [site['addr'] for site in game.get('widescreen_site', [])]
        code_addresses += game.get('functions', {}).get('extra', [])
        code_addresses += flines.get('functions', {}).get('extra', [])
        self.assertGreater(len(code_addresses), 500)
        text_ranges = [(entry['address'], entry['address'] + entry['capacity']) for entry in catalog['entries']]
        for address in code_addresses:
            for low, high in text_ranges:
                with self.subTest(address=hex(address), range=(hex(low), hex(high))):
                    self.assertFalse(low <= address < high,
                                      f'translation edit [{low:06X},{high:06X}) overlaps known code address {address:06X}')
