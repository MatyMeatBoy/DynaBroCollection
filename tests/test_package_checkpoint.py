import json
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from tools.package_checkpoint import close, digest, inventory, validate_build_info, project_version


class PackageTests(unittest.TestCase):
    def test_requires_embedded_clean_revision_and_checkpoint_version(self):
        version=project_version()
        validate_build_info(f'game {version} git abcdef1234; engine git example', 'abcdef1234567890')
        for info in [f'game {version} git abcdef1234-dirty;', f'game {version} git 1234567890;',
                     'game 0.1.1 git abcdef1234;', 'game 0.0.1 git abcdef1234;']:
            with self.assertRaises(RuntimeError):validate_build_info(info,'abcdef1234567890')

    def fixture(self, root):
        candidate=root/'candidate';payload=candidate/'DynaBrothers';payload.mkdir(parents=True)
        binary=payload/'test.exe';binary.write_bytes(b'MZ-test-fixture')
        manifest=dict(version='0.1.1',status='candidate',source_commit='synthetic',files=inventory(payload))
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
            qa['binaries']['test.exe']=digest(candidate/'DynaBrothers/test.exe')
            path.write_text(json.dumps(qa),encoding='utf-8')
            (candidate/'DynaBrothers/test.exe').write_bytes(b'changed')
            with self.assertRaisesRegex(RuntimeError,'Staged package changed'):
                close(candidate,path,root/'releases')

    def test_new_version_uses_distinct_name_and_requires_extended_routes(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            with contextlib.redirect_stdout(io.StringIO()):close(candidate,path,root/'releases')
            first=root/'releases/DynaBrothers-v0.1.1-win64-checkpoint-r1.zip';before=digest(first)
            other,path,qa=self.fixture(root/'other')
            manifest_path=other/'DynaBrothers/PACKAGE.json'
            manifest=json.loads(manifest_path.read_text());manifest['version']='../invalid'
            manifest_path.write_text(json.dumps(manifest),encoding='utf-8')
            with self.assertRaises(ValueError):close(other,path,root/'releases')
            manifest['version']='0.1.2';manifest_path.write_text(json.dumps(manifest),encoding='utf-8')
            with self.assertRaises(RuntimeError):close(other,path,root/'releases')
            for name in ['db1-practice','db1-alien','db1-credits','db2-unpause']:qa['cases'][name]=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):close(other,path,root/'releases')
            self.assertEqual(digest(first),before)
            self.assertTrue((root/'releases/DynaBrothers-v0.1.2-win64-checkpoint-r1.zip').is_file())

    def test_closed_archive_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            with contextlib.redirect_stdout(io.StringIO()):
                close(candidate,path,root/'releases')
            archive=root/'releases/DynaBrothers-v0.1.1-win64-checkpoint-r1.zip';before=archive.read_bytes()
            self.assertEqual(json.loads((candidate/'CLOSED.json').read_text())['sha256'],digest(archive))
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases')
            self.assertEqual(archive.read_bytes(),before)

    def test_second_revision_preserves_first_and_requires_new_translation_routes(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            with contextlib.redirect_stdout(io.StringIO()):close(candidate,path,root/'releases')
            first=root/'releases/DynaBrothers-v0.1.1-win64-checkpoint-r1.zip';before=digest(first)
            other,qa_path,qa=self.fixture(root/'other')
            with self.assertRaises(RuntimeError):close(other,qa_path,root/'releases',revision=2)
            self.assertEqual(json.loads((other/'DynaBrothers/PACKAGE.json').read_text())['status'],'candidate')
            for name in ['db1-practice','db1-alien','db1-credits']:qa['cases'][name]=dict(passed=True)
            qa_path.write_text(json.dumps(qa),encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):close(other,qa_path,root/'releases',revision=2)
            self.assertEqual(digest(first),before)
            self.assertTrue((root/'releases/DynaBrothers-v0.1.1-win64-checkpoint-r2.zip').is_file())
            self.assertEqual(json.loads((other/'DynaBrothers/PACKAGE.json').read_text())['checkpoint_revision'],2)

    def test_rejects_invalid_revision_and_an_existing_delivery_before_mutation(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            for revision in [0,-1,True,1000]:
                with self.assertRaises(ValueError):close(candidate,path,root/'releases',revision=revision)
            releases=root/'releases';releases.mkdir()
            archive=releases/'DynaBrothers-v0.1.1-win64-checkpoint-r1.zip';archive.write_bytes(b'preserve')
            with self.assertRaises(FileExistsError):close(candidate,path,releases)
            self.assertEqual(archive.read_bytes(),b'preserve')
            self.assertEqual(json.loads((candidate/'DynaBrothers/PACKAGE.json').read_text())['status'],'candidate')

    def test_corrected_012_revision_requires_active_db2_gameplay(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            manifest_path=candidate/'DynaBrothers/PACKAGE.json'
            manifest=json.loads(manifest_path.read_text());manifest['version']='0.1.2'
            manifest_path.write_text(json.dumps(manifest),encoding='utf-8')
            for name in ['db1-practice','db1-alien','db1-credits']:qa['cases'][name]=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases',revision=2)
            qa['cases']['db2-unpause']=dict(passed=True);path.write_text(json.dumps(qa),encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):close(candidate,path,root/'releases',revision=2)
            self.assertTrue((root/'releases/DynaBrothers-v0.1.2-win64-checkpoint-r2.zip').is_file())

    def test_013_requires_expanded_view_modal_assets_and_portable_diagnostics(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            manifest_path=candidate/'DynaBrothers/PACKAGE.json'
            manifest=json.loads(manifest_path.read_text());manifest['version']='0.1.3'
            manifest_path.write_text(json.dumps(manifest),encoding='utf-8')
            for name in ['db1-practice','db1-alien','db1-credits','db2-unpause']:
                qa['cases'][name]=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with self.assertRaises(RuntimeError):close(candidate,path,root/'releases')
            for name in ['db1-wide-original','db1-wide-expanded','db1-wide-state-equality',
                         'db1-achievements-ui','db2-achievements-ui','achievement-assets',
                         'DynaDb1VideoTest','DynaDb1visibleTest','portable-debug-script']:
                qa['cases'][name]=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):close(candidate,path,root/'releases')
            self.assertTrue((root/'releases/DynaBrothers-v0.1.3-win64-checkpoint-r1.zip').is_file())


    def test_016_rejects_setup_only_evidence_without_real_home_buttons(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);candidate,path,qa=self.fixture(root)
            manifest_path=candidate/'DynaBrothers/PACKAGE.json'
            manifest=json.loads(manifest_path.read_text());manifest['version']='0.1.6'
            manifest_path.write_text(json.dumps(manifest),encoding='utf-8')
            for name in ['db1-practice','db1-alien','db1-credits','db2-unpause',
                         'db1-wide-original','db1-wide-expanded','db1-wide-state-equality',
                         'db1-achievements-ui','db2-achievements-ui','achievement-assets',
                         'DynaDb1VideoTest','DynaDb1visibleTest','portable-debug-script',
                         'db1-audio-signal','db1-lightning','host-settings-same-window',
                         'launcher-integration-info','db1-stage-intro-1440','db1-camera-1440',
                         'db1-options-1440','db1-password-1440','db1-submenus-1440',
                         'db1-tsunami-1440','green-hub-same-window','db1-intro-fade',
                         'db1-save-result','db1-hard-ending-save','db1-visor--2',
                         'db1-visor-0','db1-visor-2']:
                qa['cases'][name]=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with self.assertRaisesRegex(RuntimeError,'Missing packaged regression cases'):
                close(candidate,path,root/'releases')
            self.assertFalse((candidate/'CLOSED.json').exists())
            qa['cases']['green-hub-home-buttons']=dict(passed=True)
            path.write_text(json.dumps(qa),encoding='utf-8')
            with contextlib.redirect_stdout(io.StringIO()):close(candidate,path,root/'releases')
            self.assertTrue((root/'releases/DynaBrothers-v0.1.6-win64-checkpoint-r1.zip').is_file())
