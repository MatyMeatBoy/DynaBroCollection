"""Read Linux port logs and compare their performance and deterministic state."""

import argparse
import json
import math
from pathlib import Path
import re
import statistics

BENCH = "GENESISRECOMP_BENCHMARK "
PHASES = ("input", "simulation", "deadline_wait", "post", "audio", "persistence", "present", "layout", "pacer")
LIVE = re.compile(r"\[R36S-LIVE\] presentations=(\d+) elapsed_us=(\d+)")
HITCH = re.compile(r"\[HOST-HITCH\] frame=(\d+) total_us=(\d+) phase_us=([\d,]+)")
VIDEO = re.compile(r"\[VIDEO\] renderer name=(\S+) vsync=(\d+)")
WINDOW = re.compile(r"\[HOST-WINDOW\] end_frame=(\d+) count=(\d+) avg_us=(\d+) phase_avg_us=([\d,]+)")
WINDOW_PEAKS = re.compile(r" max_us=(\d+) frames_ge_25ms=(\d+)")
SLOW_SIM = re.compile(r"\[HOST-SLOW-SIM\] frame=(\d+) us=(\d+)(?: cpu_us=(\d+))? "
                      r"instructions=(\d+) pc_start=([\da-f]+) pc_end=([\da-f]+)", re.I)
M68K_PEAK = re.compile(r"\[R36S-68K-PEAK\] frame=(\d+) line=(\d+) us=(\d+) "
                       r"instructions=(\d+) pc_start=([\da-f]+) pc_end=([\da-f]+)", re.I)


def percentile(values, fraction):
    points = sorted(values)
    return points[max(0, math.ceil(fraction * len(points)) - 1)]


def summarize(path, start_frame=0):
    data = {"file": path.name, "benchmarks": [], "renderer": [], "diagnostics": []}
    rates, hitches, windows, window_peaks, slow_sim = [], [], [], [], []
    m68k_peaks = []
    live_frame = 0
    with path.open(encoding="utf-8-sig", errors="replace") as source:
        for entry in source:
            line = entry.strip()
            if line.startswith(BENCH):
                try:
                    benchmark = json.loads(line[len(BENCH):])
                    if not isinstance(benchmark, dict):
                        raise ValueError("Expected a benchmark object")
                    data["benchmarks"].append(benchmark)
                except ValueError:
                    data.setdefault("invalid_benchmark_lines", 0)
                    data["invalid_benchmark_lines"] += 1
            if match := VIDEO.search(line):
                data["renderer"].append({"backend": match[1], "vsync": bool(int(match[2]))})
            if match := LIVE.search(line):
                frames, delta = map(int, match.groups())
                if live_frame >= start_frame and delta:
                    rates.append(frames * 1_000_000 / delta)
                live_frame += frames
            if match := HITCH.search(line):
                frame, total = int(match[1]), int(match[2])
                parts = tuple(map(int, match[3].split(",")))
                if frame >= start_frame and len(parts) == len(PHASES):
                    hitches.append((frame, total, parts))
            if match := WINDOW.search(line):
                frame, count, total = map(int, match.groups()[:3])
                parts = tuple(map(int, match[4].split(",")))
                if count and frame - count + 1 >= start_frame and len(parts) == len(PHASES):
                    windows.append((count, total, parts))
                    if peaks := WINDOW_PEAKS.match(line, match.end()):
                        window_peaks.append(tuple(map(int, peaks.groups())))
            if (match := SLOW_SIM.search(line)) and int(match[1]) >= start_frame:
                slow_sim.append({"frame": int(match[1]), "wall_ms": round(int(match[2]) / 1000, 3),
                                 "cpu_ms": round(int(match[3]) / 1000, 3) if match[3] and int(match[3]) else None,
                                 "instructions": int(match[4]), "pc_start": match[5], "pc_end": match[6]})
            if (match := M68K_PEAK.search(line)) and int(match[1]) >= start_frame:
                m68k_peaks.append({"frame": int(match[1]), "scanline": int(match[2]),
                                   "wall_ms": int(match[3]) / 1000, "instructions": int(match[4]),
                                   "pc_start": match[5], "pc_end": match[6]})
            if line.startswith(("[evidence]", "[progress-autosave]", "SDL_Init:", "SDL_CreateWindow:",
                                "SDL_CreateRenderer:", "[SDL-FAULT]", "[HOST-TIMING]")):
                if len(data["diagnostics"]) < 40 and line not in data["diagnostics"]:
                    data["diagnostics"].append(line)
    if rates:
        data["presentation_windows"] = {
            "count": len(rates), "minimum_fps": round(min(rates), 2),
            "median_fps": round(statistics.median(rates), 2),
            "below_58": sum(rate < 58 for rate in rates),
        }
    if windows:
        samples = sum(row[0] for row in windows)
        data["engine_windows"] = {
            "windows": len(windows), "frames": samples,
            "average_ms": round(sum(row[0] * row[1] for row in windows) / samples / 1000, 3),
            "phase_average_ms": {
                name: round(sum(row[0] * row[2][i] for row in windows) / samples / 1000, 3)
                for i, name in enumerate(PHASES)
            },
        }
    if hitches:
        rows = sorted(hitches, key=lambda row: row[1], reverse=True)
        data["retained_hitches"] = {
            "count": len(rows), "max_ms": round(rows[0][1] / 1000, 3),
            "p95_ms_of_retained_hitches": round(percentile([row[1] for row in rows], .95) / 1000, 3),
            "phase_median_ms": {
                name: round(statistics.median(row[2][i] for row in rows) / 1000, 3)
                for i, name in enumerate(PHASES)
            },
            "worst": [{"frame": index, "ms": round(total / 1000, 3),
                       "phase_ms": {name: round(value / 1000, 3) for name, value in zip(PHASES, parts)}}
                      for index, total, parts in rows[:5]],
        }
    if window_peaks:
        data["engine_windows"].update(
            peak_windows=len(window_peaks),
            max_ms=round(max(row[0] for row in window_peaks) / 1000, 3),
            frames_ge_25ms=sum(row[1] for row in window_peaks),
        )
    if slow_sim:
        data["slow_simulation"] = sorted(slow_sim, key=lambda row: row["wall_ms"], reverse=True)[:10]
    if m68k_peaks:
        data["m68k_scanline_peaks"] = m68k_peaks
    return data


def compare(reference, candidate):
    a, b = reference["benchmarks"], candidate["benchmarks"]
    if not a or not b:
        return None
    a, b = a[-1], b[-1]
    paired = all(a.get(key) is not None and a.get(key) == b.get(key)
                 for key in ("game", "frames", "workload_scope"))
    hashes = ("state_fnv1a64", "audio_state_fnv1a64")
    return {"same_scope": paired,
            "both_complete": bool(a.get("complete") and b.get("complete")),
            "hash_matches": {key: bool(paired and a.get(key) and a[key] == b.get(key)) for key in hashes},
            "fps_ratio": round(b["fps"] / a["fps"], 3) if paired and a.get("fps") and b.get("fps") else None}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", type=Path, nargs="+")
    parser.add_argument("--from-frame", type=int, default=0)
    args = parser.parse_args()
    reports = [summarize(path, args.from_frame) for path in args.logs]
    result = {"logs": reports}
    if len(reports) == 2:
        result["comparison"] = compare(*reports)
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
