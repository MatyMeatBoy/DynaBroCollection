import pathlib
import subprocess
import tempfile
import unittest
from tools.build import verify_engine_checkout


class EnginePinTests(unittest.TestCase):
    def test_only_the_exact_approved_patch_is_accepted(self):
        scratch = pathlib.Path('build/engine-tests').resolve()
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            root = pathlib.Path(directory)

            def git(*args):
                return subprocess.check_output(['git', '-C', str(root), *args], stderr=subprocess.DEVNULL)

            git('init', '-q')
            (root / 'approved.c').write_text('original\n')
            (root / 'other.c').write_text('original\n')
            git('add', '.')
            git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-qm', 'fixture')
            sha = git('rev-parse', 'HEAD').decode().strip()
            (root / 'approved.c').write_text('approved\n')
            patch = root.parent / (root.name + '.patch')
            patch.write_bytes(git('diff', '--binary', 'HEAD', '--'))
            try:
                verify_engine_checkout(root, sha, patch)
                (root / 'other.c').write_text('unapproved\n')
                with self.assertRaises(RuntimeError):
                    verify_engine_checkout(root, sha, patch)
                (root / 'other.c').write_text('original\n')
                (root / 'untracked.c').write_text('unexpected source\n')
                with self.assertRaises(RuntimeError):
                    verify_engine_checkout(root, sha, patch)
            finally:
                patch.unlink()
