import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.r36s_debug import BENCH, compare, summarize


class R36SDebugTests(unittest.TestCase):
    def test_scope_and_phase_windows(self):
        run = {"game": "DynaBrothers1AOT", "workload_scope": "paced-runtime", "frames": 360,
               "complete": True, "state_fnv1a64": "state", "audio_state_fnv1a64": "audio", "fps": 60}
        with TemporaryDirectory() as tmp:
            log = Path(tmp) / "port.log"
            log.write_text("\n".join([
                "[HOST-WINDOW] end_frame=179 count=180 avg_us=16688 phase_avg_us=0,1000,0,0,0,0,0,0,15688",
                "[HOST-WINDOW] end_frame=359 count=180 avg_us=20000 phase_avg_us=0,15000,0,0,0,0,0,0,5000",
                "[HOST-HITCH] frame=359 total_us=51000 phase_us=0,50000,0,0,0,0,0,0,1000",
                "[R36S-LIVE] presentations=180 elapsed_us=3000000",
                "[R36S-LIVE] presentations=180 elapsed_us=3600000",
                BENCH + json.dumps(run),
                BENCH + "{truncated",
            ]), encoding="utf-8")
            outcome = summarize(log, 180)
            self.assertEqual(outcome["presentation_windows"]["median_fps"], 50)
            self.assertEqual(outcome["engine_windows"]["frames"], 180)
            self.assertEqual(outcome["engine_windows"]["phase_average_ms"]["simulation"], 15)
            self.assertEqual(outcome["retained_hitches"]["max_ms"], 51)
            self.assertEqual(outcome["invalid_benchmark_lines"], 1)
            self.assertTrue(all(compare(outcome, outcome)["hash_matches"].values()))
            other_scope = {"benchmarks": [run | {"workload_scope": "uncapped-runtime"}]}
            comparison = compare(outcome, other_scope)
            self.assertFalse(comparison["same_scope"])
            self.assertIsNone(comparison["fps_ratio"])


if __name__ == "__main__":
    unittest.main()
