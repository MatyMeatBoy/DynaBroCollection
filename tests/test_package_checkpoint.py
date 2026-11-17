import json
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from tools.package_checkpoint import close, digest, inventory, validate_build_info


class PackageTests(unittest.TestCase):
    def test_requires_embedded_clean_revision_and_checkpoint_version(self):
        validate_build_info('game 0.1.1 git abcdef1234; engine git example', 'abcdef1234567890')
        for info in ['game 0.1.1 git abcdef1234-dirty;', 'game 0.1.1 git 1234567890;', 'game 0.0.1 git abcdef1234;']:
            with self.assertRaises(RuntimeError):validate_build_info(info,'abcdef1234567890')

    def fixture(self, root):
        candidate=root/'candidate';payload=candidate/'DynaBrothers';payload.mkdir(parents=True)
        binary=payload/'test.exe';binary.write_bytes(b'MZ-test-fixture')
        manifest=dict(status='candidate',source_commit='synthetic',files=inventory(payload))
        (payload/'PACKAGE.json').write_text(json.dumps(manifest),encoding='utf-8')
        cases={name:dict(passed=True) for name in ['db1-en','db1-ja','db2-en','db2-ja','replay-record','replay-play','hint']}
        qa=dict(passed=True,binaries={'test.exe':digest(binary)},cases=cases)
        path=root/'qa.json';path.write_text(json.dumps(qa),encoding='utf-8')
        return candidate,path,qa

    def test_rejects_changed_package_or_wrong_qa_before_closure(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            qa['binaries']['test.exe']='wrong';path.write_text(json.dumps(qa),encoding='utf-8')
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases')
            self.assertEqual(json.loads((candidate/'DynaBrothers/PACKAGE.json').read_text())['status'],'candidate')
            (candidate/'DynaBrothers/test.exe').write_bytes(b'changed')
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases')

    def test_closed_archive_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            with contextlib.redirect_stdout(io.StringIO()):
                close(candidate,path,root/'releases')
            archive=root/'releases/DynaBrothers-v0.1.1-win64-checkpoint-r1.zip';before=archive.read_bytes()
            self.assertEqual(json.loads((candidate/'CLOSED.json').read_text())['sha256'],digest(archive))
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases')
            self.assertEqual(archive.read_bytes(),before)
