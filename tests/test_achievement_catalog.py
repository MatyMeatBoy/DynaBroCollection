import json
from pathlib import Path
import shutil
import tempfile
import unittest
from tools.generate_achievement_catalog import generate

ROOT = Path(__file__).resolve().parent.parent


class AchievementCatalogTests(unittest.TestCase):
    def test_embedded_catalogs_match_reviewed_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'assets/achievements').mkdir(parents=True)
            for game in ['db1', 'db2']:
                shutil.copy2(ROOT / f'assets/achievements/{game}.json', root / f'assets/achievements/{game}.json')
            generate(root)
            for game in ['db1', 'db2']:
                relative = f'games/achievement_catalog/{game}.inc'
                self.assertEqual((root / relative).read_bytes(), (ROOT / relative).read_bytes())

    def test_complete_sets_have_all_color_and_locked_badges(self):
        for game, count, points in [('db1', 76, 470), ('db2', 99, 1025)]:
            source = ROOT / 'assets/achievements'
            catalog = json.loads((source / f'{game}.json').read_text(encoding='utf-8'))
            entries = catalog['achievements']
            self.assertEqual(len(entries), count)
            self.assertEqual(len({entry['id'] for entry in entries}), count)
            self.assertEqual(sum(entry['points'] for entry in entries), points)
            for entry in entries:
                for key in ['badge_file_locked', 'badge_file_unlocked']:
                    path = (source / entry[key]).resolve()
                    self.assertTrue(path.is_relative_to(source.resolve()))
                    png = path.read_bytes()
                    self.assertTrue(png.startswith(b'\x89PNG\r\n\x1a\n'))
                    self.assertEqual((int.from_bytes(png[16:20], 'big'), int.from_bytes(png[20:24], 'big')), (64, 64))
            if game == 'db2':
                self.assertFalse(catalog['official_set'])
                self.assertEqual(sum(bool(e.get('requires_all')) for e in entries), 6)

    def test_db2_rejects_invalid_ids_and_milestone_dependencies(self):
        def mutate(data):
            data['achievements'][1]['id'] = data['achievements'][0]['id']

        def negative_id(data):
            data['achievements'][0]['id'] = -1

        def missing_dependency(data):
            data['achievements'][0]['requires_all'] = [999999999]

        def self_dependency(data):
            data['achievements'][0]['requires_all'] = [data['achievements'][0]['id']]

        def duplicate_dependency(data):
            entry = next(e for e in data['achievements'] if e.get('requires_all'))
            entry['requires_all'] = entry['requires_all'] * 2

        def nested_milestone(data):
            first, second, *_ = (e for e in data['achievements'] if e.get('requires_all'))
            second['requires_all'] = [first['id']]

        for mutator in [mutate, negative_id, missing_dependency, self_dependency, duplicate_dependency, nested_milestone]:
            with self.subTest(mutator=mutator.__name__), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                (root / 'assets/achievements').mkdir(parents=True)
                for game in ['db1', 'db2']:
                    shutil.copy2(ROOT / f'assets/achievements/{game}.json', root / f'assets/achievements/{game}.json')
                data = json.loads((root / 'assets/achievements/db2.json').read_text(encoding='utf-8'))
                mutator(data)
                (root / 'assets/achievements/db2.json').write_text(json.dumps(data), encoding='utf-8')
                with self.assertRaises(ValueError):
                    generate(root)
