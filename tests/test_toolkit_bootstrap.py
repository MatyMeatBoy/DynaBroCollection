import hashlib
from pathlib import Path
import tempfile
import unittest
from tools.toolkit_bootstrap import verify, restore


class ToolkitBootstrapTests(unittest.TestCase):
    def test_verify_accepts_matching_inventory(self):
        with tempfile.TemporaryDirectory() as directory:
            kit = Path(directory)
            (kit / 'a.txt').write_bytes(b'hello')
            manifest = {'files': {'a.txt': {'size': 5, 'sha256': hashlib.sha256(b'hello').hexdigest()}}}
            verify(kit, manifest)

    def test_verify_rejects_path_traversal_missing_file_and_checksum_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            kit = Path(directory)
            (kit / 'a.txt').write_bytes(b'hello')
            cases = [
                {'files': {'../evil.txt': {'size': 1, 'sha256': 'x'}}},
                {'files': {'missing.txt': {'size': 1, 'sha256': 'x'}}},
                {'files': {'a.txt': {'size': 5, 'sha256': 'wrong'}}},
            ]
            for manifest in cases:
                with self.subTest(manifest=manifest), self.assertRaises((ValueError, RuntimeError)):
                    verify(kit, manifest)

    def test_restore_preserves_an_existing_destination(self):
        with tempfile.TemporaryDirectory() as directory:
            kit = Path(directory)
            destination = kit / 'dest'
            destination.mkdir()
            with self.assertRaises(FileExistsError):
                restore(kit, destination, {}, {})
