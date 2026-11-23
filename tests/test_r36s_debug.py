import json
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from tools.r36s_debug import BENCH, compare, compare_windows, summarize


class R36SDebugTests(unittest.TestCase):
    def test_session_summary_keeps_old_peak_outside_hitch_ring(self):
        with TemporaryDirectory() as tmp:
            log = Path(tmp) / "port.log"
            log.write_text("\n".join([
                "[HOST-PERCENTILES] frames=360 bucket_us=512 overflow_from_us=261632 p50_us=16895 p95_us=25599 p99_us=-1",
                "[HOST-WORST] frame=2 total_us=416800 phase_us=0,410000,0,0,6800,0,0,0,0",
                "[HOST-HITCH] frame=359 total_us=51000 phase_us=0,50000,0,0,0,0,0,0,1000",
            ]), encoding="utf-8")
            result = summarize(log)
            self.assertEqual(result["session_worst_frame"]["ms"], 416.8)
            self.assertEqual(result["session_worst_frame"]["phase_ms"]["simulation"], 410)
            self.assertEqual(result["session_frame_percentiles"]["upper_bounds_ms"],
                             {"p50": 16.895, "p95": 25.599, "p99": None})
            self.assertEqual(result["retained_hitches"]["max_ms"], 51)
            filtered = summarize(log, start_frame=180)
            self.assertNotIn("session_worst_frame", filtered)
            self.assertNotIn("session_frame_percentiles", filtered)

    def test_match_windows_and_raster_by_frame(self):
        with TemporaryDirectory() as tmp:
            logs = [Path(tmp) / name for name in ("hardware.log", "vm.log")]
            for i, log in enumerate(logs):
                log.write_text("\n".join([
                    "[evidence] build exe=abcdef rom=123456",
                    f"[VIDEO] renderer name={'gles' if i else 'software'} vsync=0",
                    f"[HOST-WINDOW] end_frame={179 + i * 360} count=180 avg_us=99000 phase_avg_us=0,0,0,0,0,0,0,0,0",
                    f"[HOST-WINDOW] end_frame=359 count=180 avg_us={30000 + i * 10000} phase_avg_us=0,24000,0,0,0,0,0,0,0",
                    f"[R36S-RASTER] frame=300 m68k_us={1000 + i * 500} z80_us=2000 video_us=3000 irq_us=4000",
                ]), encoding="utf-8")
            comparison = compare_windows(*(summarize(log, retain_windows=True) for log in logs))
            self.assertEqual(comparison["matched_frame_ranges"], ["180-359"])
            self.assertEqual(comparison["median_ms"]["candidate"]["frame"], 40)
            self.assertEqual(comparison["matched_raster_frames"]["300"]["candidate_ms"]["m68k"], 1.5)
            self.assertTrue(comparison["same_executable_and_rom_fingerprints"])
            self.assertFalse(comparison["same_reported_renderer"])

    def test_scope_and_phase_windows(self):
        run = {"game": "DynaBrothers1AOT", "workload_scope": "paced-runtime", "frames": 360,
               "complete": True, "state_fnv1a64": "state", "audio_state_fnv1a64": "audio", "fps": 60}
        with TemporaryDirectory() as tmp:
            log = Path(tmp) / "port.log"
            log.write_text("\n".join([
                "[HOST-WINDOW] end_frame=179 count=180 avg_us=16688 phase_avg_us=0,1000,0,0,0,0,0,0,15688",
                "[HOST-WINDOW] end_frame=359 count=180 avg_us=20000 phase_avg_us=0,15000,0,0,0,0,0,0,5000"
                " max_us=51000 frames_ge_25ms=12",
                "[HOST-HITCH] frame=359 total_us=51000 phase_us=0,50000,0,0,0,0,0,0,1000",
                "[HOST-SLOW-SIM] frame=11 us=45000 instructions=10000 pc_start=000000 pc_end=000200",
                "[HOST-SLOW-SIM] frame=359 us=50000 cpu_us=47000 instructions=12000 pc_start=009a14 pc_end=0098d0",
                "[R36S-68K-PEAK] frame=359 line=72 us=3000 instructions=32 pc_start=0098cc pc_end=0098d4",
                "[R36S-LIVE] presentations=180 elapsed_us=3000000",
                "[R36S-LIVE] presentations=180 elapsed_us=3600000",
                BENCH + json.dumps(run),
                BENCH + "{truncated",
            ]), encoding="utf-8")
            outcome = summarize(log, 180)
            self.assertEqual(outcome["presentation_windows"]["median_fps"], 50)
            self.assertEqual(outcome["engine_windows"]["frames"], 180)
            self.assertEqual(outcome["engine_windows"]["phase_average_ms"]["simulation"], 15)
            self.assertEqual(outcome["engine_windows"]["frames_ge_25ms"], 12)
            self.assertEqual(outcome["slow_simulation"][0]["cpu_ms"], 47)
            self.assertEqual(outcome["m68k_scanline_peaks"], [{
                "frame": 359, "scanline": 72, "wall_ms": 3, "instructions": 32,
                "pc_start": "0098cc", "pc_end": "0098d4",
            }])
            self.assertEqual(len(outcome["slow_simulation"]), 1)
            self.assertEqual(outcome["retained_hitches"]["max_ms"], 51)
            self.assertEqual(outcome["invalid_benchmark_lines"], 1)
            self.assertTrue(all(compare(outcome, outcome)["hash_matches"].values()))
            other_scope = {"benchmarks": [run | {"workload_scope": "uncapped-runtime"}]}
            comparison = compare(outcome, other_scope)
            self.assertFalse(comparison["same_scope"])
            self.assertIsNone(comparison["fps_ratio"])


if __name__ == "__main__":
    unittest.main()
