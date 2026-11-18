import tempfile
import hashlib
import unittest
from pathlib import Path
from tools.generate_language import ranges, emit_image, MAX_LANGUAGE_VARIANTS


class LanguageGeneratorTests(unittest.TestCase):
    def test_retains_four_qualified_language_revisions(self):
        images = [bytes(40), b'\x01' + bytes(39), bytes(39) + b'\x02', bytes(20)+b'\x03'+bytes(19)]
        with tempfile.TemporaryDirectory() as directory:
            report = emit_image(images, 'db1', Path(directory))
            self.assertEqual(report['variants'], 4)
            for source in images:
                for target in images:
                    out = bytearray(source)
                    for start, end in ranges(images):
                        out[start:end] = target[start:end]
                    self.assertEqual(bytes(out), target)

    def test_emits_ordered_sha256_metadata_separate_from_c_image(self):
        images=[bytes([i])+bytes(39) for i in range(5)]
        with tempfile.TemporaryDirectory() as directory:
            report=emit_image(images,'db1',Path(directory))
            header=(Path(directory)/'db1_language.h').read_text()
            expected='static const char *const db1_language_sha256[]={'+','.join('"'+hashlib.sha256(image).hexdigest()+'"' for image in images)+'};'
            self.assertIn(expected,header)
            self.assertEqual([item['sha256'] for item in report['images']],[hashlib.sha256(image).hexdigest() for image in images])
            self.assertIn('static const DynaLanguageImage db1_language_image={40,5,',header)

    def test_generator_and_c_capacity_match(self):
        header=(Path(__file__).resolve().parents[1]/'games/language_patch.h').read_text()
        self.assertIn(f'#define DYNA_LANGUAGE_VARIANTS {MAX_LANGUAGE_VARIANTS}\n',header)

    def test_five_and_eight_variants_reverse_exactly(self):
        for count in (5,8):
            images=[bytes([variant])+bytes(39) for variant in range(count)]
            with tempfile.TemporaryDirectory() as directory:
                report=emit_image(images,'expanded',Path(directory))
                self.assertEqual(report['variants'],count)
                for source in images:
                    for target in images:
                        result=bytearray(source)
                        for start,end in ranges(images):result[start:end]=target[start:end]
                        self.assertEqual(bytes(result),target)

    def test_nine_variants_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):
                emit_image([bytes([i]) for i in range(9)],'overflow',Path(directory))
            self.assertFalse(list(Path(directory).iterdir()))

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
