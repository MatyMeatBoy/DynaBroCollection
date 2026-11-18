import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from tools.package_checkpoint import digest
from tools.package_toolkit import close, inventory


class PackageToolkitTests(unittest.TestCase):
    def make_candidate(self, root, source_commit='abc123', status='candidate', name='candidate'):
        candidate = root / name
        candidate.mkdir()
        (candidate / 'file.txt').write_bytes(b'hello')
        manifest_path = candidate / 'TOOLKIT.json'
        manifest = dict(status=status, source_commit=source_commit, files=inventory(candidate))
        manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
        return candidate, manifest_path

    def make_proof(self, root, manifest_path, source_commit='abc123', passed=True, manifest_sha=None):
        proof = root / 'proof.json'
        proof.write_text(json.dumps(dict(
            passed=passed, source_commit=source_commit,
            toolkit_candidate_manifest_sha256=manifest_sha or digest(manifest_path))), encoding='utf-8')
        return proof

    def test_closes_a_valid_candidate_into_a_verified_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            candidate, manifest_path = self.make_candidate(root)
            proof = self.make_proof(root, manifest_path)
            archive = root / 'out.zip'
            close(candidate, proof, archive)
            self.assertTrue(archive.exists())
            self.assertEqual(digest(archive), archive.with_suffix('.sha256').read_text(encoding='ascii').split()[0])
            self.assertEqual(json.loads(manifest_path.read_text(encoding='utf-8'))['status'], 'ready')
            with zipfile.ZipFile(archive) as z:
                self.assertEqual(z.read('DynaBrothers-Development-Toolkit/file.txt'), b'hello')

    def test_rejects_changed_candidate_wrong_evidence_and_existing_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            candidate, manifest_path = self.make_candidate(root, status='ready', name='c-status')
            proof = self.make_proof(root, manifest_path)
            with self.assertRaises(RuntimeError):
                close(candidate, proof, root / 'a.zip')

            candidate, manifest_path = self.make_candidate(root, name='c-tampered')
            (candidate / 'file.txt').write_bytes(b'tampered')
            proof = self.make_proof(root, manifest_path)
            with self.assertRaises(RuntimeError):
                close(candidate, proof, root / 'b.zip')

            candidate, manifest_path = self.make_candidate(root, name='c-failed')
            proof = self.make_proof(root, manifest_path, passed=False)
            with self.assertRaises(RuntimeError):
                close(candidate, proof, root / 'c.zip')

            candidate, manifest_path = self.make_candidate(root, name='c-wrong-commit')
            proof = self.make_proof(root, manifest_path, source_commit='other')
            with self.assertRaises(RuntimeError):
                close(candidate, proof, root / 'd.zip')

            candidate, manifest_path = self.make_candidate(root, name='c-wrong-sha')
            proof = self.make_proof(root, manifest_path, manifest_sha='0' * 64)
            with self.assertRaises(RuntimeError):
                close(candidate, proof, root / 'e.zip')

            candidate, manifest_path = self.make_candidate(root, name='c-existing-archive')
            proof = self.make_proof(root, manifest_path)
            existing = root / 'f.zip'
            existing.write_bytes(b'already here')
            with self.assertRaises(FileExistsError):
                close(candidate, proof, existing)
