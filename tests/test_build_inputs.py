from pathlib import Path
import tempfile
import unittest
from tools.build import default_input_root


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
