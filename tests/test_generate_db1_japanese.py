from pathlib import Path
import tempfile
import unittest
from tools.generate_db1_japanese import generate


class GenerateDb1JapaneseTests(unittest.TestCase):
    def test_rejects_rom_that_does_not_match_the_qualified_db1_japanese_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'rom.md'
            source.write_bytes(b'not the real Japanese DB1 dump')
            with self.assertRaises(ValueError):
                generate(source, Path(directory))
