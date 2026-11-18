import hashlib
from pathlib import Path
import tempfile
import unittest
from tools.build import find_by_hash


class FindByHashTests(unittest.TestCase):
    def test_finds_matching_file_by_content_recursively_and_case_insensitive_extension(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'sub').mkdir()
            match = root / 'sub/rom.MD'
            match.write_bytes(b'db2-special-dump')
            (root / 'other.bin').write_bytes(b'different-content')
            (root / 'wrong-extension.txt').write_bytes(b'db2-special-dump')
            digest = hashlib.sha256(b'db2-special-dump').hexdigest()
            self.assertEqual(find_by_hash(root, digest, {'.md', '.bin'}), match)

    def test_raises_when_no_file_matches(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'rom.md').write_bytes(b'something else')
            with self.assertRaises(RuntimeError):
                find_by_hash(root, hashlib.sha256(b'expected').hexdigest(), {'.md', '.bin'})
