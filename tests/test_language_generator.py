import tempfile
import unittest
from pathlib import Path
from tools.generate_language import ranges, emit_image


class LanguageGeneratorTests(unittest.TestCase):
    def test_ranges_cover_all_revisions_without_losing_unchanged_bytes(self):
        images = [bytes(40), b'\x01' + bytes(39), bytes(39) + b'\x02']
        spans = ranges(images)
        for source in images:
            for target in images:
                out = bytearray(source)
                for start, end in spans:
                    out[start:end] = target[start:end]
                self.assertEqual(bytes(out), target)
        with tempfile.TemporaryDirectory() as directory:
            report = emit_image(images, 'test_game', Path(directory))
            self.assertEqual(report['variants'], 3)
            self.assertTrue((Path(directory) / 'test_game_language.h').is_file())

    def test_rejects_inconsistent_layout_and_unsafe_identifier(self):
        with tempfile.TemporaryDirectory() as directory:
            for images, name in [([b'a', b'ab'], 'test'), ([b'a'], 'test'),
                                  ([b'a', b'b'], '../test')]:
                with self.assertRaises(ValueError):
                    emit_image(images, name, Path(directory))
