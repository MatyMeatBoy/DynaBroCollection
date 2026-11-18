from pathlib import Path
import tempfile
import os
import json
import zlib
import unittest
from tools.build import default_input_root, db1_language_images
from tools.generate_language import ranges


class BuildInputTests(unittest.TestCase):
    def test_project_input_folder_wins_over_legacy_sibling(self):
        with tempfile.TemporaryDirectory() as folder:
            project = Path(folder) / 'project'
            project.mkdir()
            legacy = project.parent / 'Dynabrothers'
            legacy.mkdir()
            private = project / 'Dynabrothers'
            private.mkdir()
            self.assertEqual(default_input_root(project), private)

    def test_legacy_layout_remains_available(self):
        with tempfile.TemporaryDirectory() as folder:
            project = Path(folder) / 'project'
            project.mkdir()
            self.assertEqual(default_input_root(project), project.parent / 'Dynabrothers')


class Db1LanguagePipelineTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('DB1_LANGUAGE_TEST_ROOT'),'private ROM qualification opt-in')
    def test_new_default_preserves_all_old_inputs_and_exact_reverse(self):
        root=Path(os.environ['DB1_LANGUAGE_TEST_ROOT'])
        original=(root/'Dynabrothers/Dyna Brothers dumped/Dyna Brothers (Japan).md').read_bytes()
        supplied=(root/'local/roms/db1-en-v01.md').read_bytes()
        catalog=json.loads((root/'translations/db1-en.json').read_text(encoding='utf-8'))
        old_current=(root/'local/roms/db1-en-950A8FC3.md').read_bytes()
        before=original,supplied,old_current
        images=db1_language_images(original,supplied,catalog)
        self.assertEqual(len(images),5)
        self.assertEqual(images[0],original);self.assertEqual(images[2],supplied)
        self.assertEqual(images[4],old_current);self.assertNotEqual(images[1],old_current)
        self.assertEqual([f'{zlib.crc32(x):08X}' for x in images[2:]],['C1E5C0DA','3BE23E09','950A8FC3'])
        spans=ranges(images)
        for source in images:
            for target in images:
                result=bytearray(source)
                for start,end in spans:result[start:end]=target[start:end]
                self.assertEqual(bytes(result),target)
        self.assertEqual((original,supplied,old_current),before)
        with self.assertRaises(ValueError):db1_language_images(bytes(len(original)),supplied,catalog)
