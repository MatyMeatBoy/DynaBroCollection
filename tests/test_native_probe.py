import tempfile
import unittest
from pathlib import Path

from tools.native_probe import prepare_scene_script, missing_captures


class NativeProbeTests(unittest.TestCase):
    def test_relocates_nested_captures_and_removes_only_exit_command(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            script, captures = prepare_scene_script(
                'WAIT 8\nSCREENSHOT D:/old/evidence/scene/frame.png\nEXIT\n', output)
            self.assertEqual(captures, [output / 'frame.png'])
            self.assertEqual(script, f'WAIT 8\nSCREENSHOT {(output / "frame.png").as_posix()}')
            self.assertEqual(missing_captures(captures), ['Missing capture: frame.png'])
            (output / 'frame.png').write_bytes(b'\x89PNG\r\n\x1a\nimage')
            self.assertEqual(missing_captures(captures), [])

    def test_rejects_duplicate_names_and_invalid_images(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            with self.assertRaises(ValueError):
                prepare_scene_script('SCREENSHOT a/frame.png\nSCREENSHOT b/frame.png', output)
            path = output / 'frame.png'
            path.write_bytes(b'not a png')
            self.assertEqual(missing_captures([path]), ['Invalid PNG capture: frame.png'])
