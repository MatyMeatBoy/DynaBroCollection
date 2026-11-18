import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parent.parent


class Db2RomIdentityConsistencyTests(unittest.TestCase):
    def test_the_three_hardcoded_db2_special_sha256_copies_agree(self):
        catalog_sha = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))['source_sha256']
        build_sha = re.search(r"DB2_SHA = '([0-9a-f]+)'", (ROOT / 'tools/build.py').read_text(encoding='utf-8')).group(1)
        discover_sha = re.search(r"SHA256 = '([0-9a-f]+)'", (ROOT / 'tools/discover_db2.py').read_text(encoding='utf-8')).group(1)
        self.assertEqual(len(catalog_sha), 64)
        self.assertEqual(catalog_sha, build_sha)
        self.assertEqual(catalog_sha, discover_sha)
