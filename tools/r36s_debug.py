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
HITCH = re.compile(r"\[(HOST-HITCH|HOST-WORST)\] frame=(\d+) total_us=(\d+) phase_us=([\d,]+)")
HITCH_AUDIO = re.compile(r" lock_us=(\d+) callback_us=(\d+) fill_us=(\d+) conceals=(\d+) repeated=(\d+)")
HITCH_SWAP = re.compile(r" swap_calls=(\d+) swap_wall_us=(\d+) swap_cpu_us=(\d+)")
SWAP_WINDOW = re.compile(r"\[HOST-SWAP\] end_frame=(\d+) calls=(\d+) avg_wall_us=(\d+) "
                         r"avg_cpu_us=(\d+) max_wall_us=(\d+)")
GRAPHICS_WINDOW = re.compile(r"\[HOST-GRAPHICS\] end_frame=(\d+) samples=(\d+) prepare_us=(\d+) "
                             r"clear_us=(\d+) texture_us=(\d+) draw_us=(\d+)")
HOST_MEMORY = re.compile(r"\[HOST-MEMORY\] rss_kib=(\d+) peak_rss_kib=(\d+)")
HOST_PERCENTILES = re.compile(r"\[HOST-PERCENTILES\] frames=(\d+) bucket_us=(\d+) overflow_from_us=(\d+) "
                              r"p50_us=(-?\d+) p95_us=(-?\d+) p99_us=(-?\d+)")
VIDEO = re.compile(r"\[VIDEO\] renderer name=(\S+) vsync=(\d+)")
WINDOW = re.compile(r"\[HOST-WINDOW\] end_frame=(\d+) count=(\d+) avg_us=(\d+) phase_avg_us=([\d,]+)")
WINDOW_PEAKS = re.compile(r" max_us=(\d+) frames_ge_25ms=(\d+)")
BUILD = re.compile(r"\[evidence\] build exe=([\da-f]+) rom=([\da-f]+)", re.I)
SLOW_SIM = re.compile(r"\[HOST-SLOW-SIM\] frame=(\d+) us=(\d+)(?: cpu_us=(\d+))? "
                      r"instructions=(\d+) pc_start=([\da-f]+) pc_end=([\da-f]+)", re.I)
SLOW_SIM_SUMMARY = re.compile(r"\[HOST-SLOW-SIM-SUMMARY\] events=(\d+) retained=(\d+)")
M68K_PEAK = re.compile(r"\[R36S-68K-PEAK\] frame=(\d+) line=(\d+) us=(\d+) "
                       r"instructions=(\d+) pc_start=([\da-f]+) pc_end=([\da-f]+)", re.I)
RASTER = re.compile(r"\[R36S-RASTER\] frame=(\d+) m68k_us=(\d+) z80_us=(\d+) video_us=(\d+) irq_us=(\d+)")


def percentile(values, fraction):
    points = sorted(values)
    return points[max(0, math.ceil(fraction * len(points)) - 1)]


def summarize(path, start_frame=0, retain_windows=False):
    data = {"file": path.name, "benchmarks": [], "renderer": [], "diagnostics": []}
    rates, hitches, windows, window_peaks, slow_sim = [], [], [], [], []
    m68k_peaks = []
    live_frame = 0
    if retain_windows:
        data["indexed_windows"] = {}
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
            if match := BUILD.search(line):
                data["build"] = {"exe": match[1].lower(), "rom": match[2].lower()}
            if match := LIVE.search(line):
                frames, delta = map(int, match.groups())
                if live_frame >= start_frame and delta:
                    rates.append(frames * 1_000_000 / delta)
                live_frame += frames
            if match := HITCH.search(line):
                tag, index, interval, phases = match.groups()
                frame, total = int(index), int(interval)
                parts = tuple(map(int, phases.split(",")))
                if frame >= start_frame and len(parts) == len(PHASES):
                    if tag == "HOST-WORST":
                        data["session_worst_frame"] = {
                            "frame": frame, "ms": total / 1000,
                            "phase_ms": {name: value / 1000 for name, value in zip(PHASES, parts)},
                        }
                        if audio := HITCH_AUDIO.match(line, match.end()):
                            lock, callback, fill, conceals, repeated = map(int, audio.groups())
                            data["session_worst_frame"]["audio"] = {
                                "lock_ms": lock / 1000, "callback_ms": callback / 1000, "fill_ms": fill / 1000,
                                "conceal_events_to_date": conceals, "concealed_frames_to_date": repeated,
                            }
                            if swap := HITCH_SWAP.match(line, audio.end()):
                                calls, wall, cpu = map(int, swap.groups())
                                data["session_worst_frame"]["swap"] = {
                                    "calls": calls, "wall_ms": wall / 1000, "cpu_ms": cpu / 1000,
                                }
                    else:
                        hitches.append((frame, total, parts))
            if start_frame == 0 and (match := HOST_PERCENTILES.search(line)):
                frames, bucket, overflow, *bounds = map(int, match.groups())
                data["session_frame_percentiles"] = {
                    "frames": frames, "bucket_ms": bucket / 1000, "overflow_from_ms": overflow / 1000,
                    "upper_bounds_ms": {
                        name: value / 1000 if value >= 0 else None
                        for name, value in zip(("p50", "p95", "p99"), bounds)
                    },
                }
            if start_frame == 0 and (match := SLOW_SIM_SUMMARY.search(line)):
                events, retained = map(int, match.groups())
                data["session_slow_simulation"] = {"events": events, "retained": retained}
            if match := SWAP_WINDOW.search(line):
                frame, calls, wall, cpu, peak = map(int, match.groups())
                if frame - 179 >= start_frame:
                    data.setdefault("swap_windows", []).append({
                        "end_frame": frame, "calls": calls, "avg_wall_ms": wall / 1000,
                        "avg_cpu_ms": cpu / 1000, "max_wall_ms": peak / 1000,
                    })
            if match := GRAPHICS_WINDOW.search(line):
                frame, samples, *times = map(int, match.groups())
                if frame - 179 >= start_frame:
                    data.setdefault("graphics_windows", []).append({
                        "end_frame": frame, "samples": samples,
                        "average_ms": dict(zip(("prepare", "clear", "texture", "draw"),
                                               (value / 1000 for value in times))),
                    })
            if start_frame == 0 and (match := HOST_MEMORY.search(line)):
                rss, peak = map(int, match.groups())
                data["process_memory"] = {"rss_kib": rss, "peak_rss_kib": peak}
            if match := WINDOW.search(line):
                frame, count, total = map(int, match.groups()[:3])
                parts = tuple(map(int, match[4].split(",")))
                if count and frame - count + 1 >= start_frame and len(parts) == len(PHASES):
                    windows.append((count, total, parts))
                    if peaks := WINDOW_PEAKS.match(line, match.end()):
                        window_peaks.append(tuple(map(int, peaks.groups())))
                    if retain_windows:
                        data["indexed_windows"][f"{frame - count + 1}-{frame}"] = (total, *parts)
            if (match := SLOW_SIM.search(line)) and int(match[1]) >= start_frame:
                slow_sim.append({"frame": int(match[1]), "wall_ms": round(int(match[2]) / 1000, 3),
                                 "cpu_ms": round(int(match[3]) / 1000, 3) if match[3] and int(match[3]) else None,
                                 "instructions": int(match[4]), "pc_start": match[5], "pc_end": match[6]})
            if (match := M68K_PEAK.search(line)) and int(match[1]) >= start_frame:
                m68k_peaks.append({"frame": int(match[1]), "scanline": int(match[2]),
                                   "wall_ms": int(match[3]) / 1000, "instructions": int(match[4]),
                                   "pc_start": match[5], "pc_end": match[6]})
            if (match := RASTER.search(line)) and int(match[1]) >= start_frame:
                data.setdefault("raster_frames", {})[match[1]] = {
                    name: int(value) / 1000
                    for name, value in zip(("m68k", "z80", "video", "irq"), match.groups()[1:])
                }
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


def compare_windows(reference, candidate):
    same_build = bool(reference.get("build") and reference["build"] == candidate.get("build"))
    first = reference["indexed_windows"]
    second = candidate["indexed_windows"]
    common = [key for key in first if key in second]
    labels = ("frame", *PHASES)
    return {
        "same_executable_and_rom_fingerprints": same_build,
        "same_reported_renderer": bool(reference["renderer"] and reference["renderer"] == candidate["renderer"]),
        "matched_frame_ranges": common,
        "matched_raster_frames": {
            frame: {"reference_ms": phases, "candidate_ms": candidate["raster_frames"][frame]}
            for frame, phases in reference.get("raster_frames", {}).items()
            if frame in candidate.get("raster_frames", {})
        },
        "median_ms": {
            side: {name: round(statistics.median(source[key][i] for key in common) / 1000, 3)
                   for i, name in enumerate(labels)}
            for side, source in (("reference", first), ("candidate", second))
        } if common else None,
    }


def calibrate_ga36(hardware_reference, vm_reference, hardware_candidate, vm_candidate):
    """Fit the scripted DB1 route on the reference and check a separate build."""
    pairs = ((hardware_reference, vm_reference), (hardware_candidate, vm_candidate))
    for hardware, vm in pairs:
        comparison = compare(hardware, vm)
        if (not hardware.get("build") or hardware["build"] != vm.get("build") or not comparison
                or not comparison["same_scope"] or not comparison["both_complete"]
                or not all(comparison["hash_matches"].values())
                or hardware["benchmarks"][-1]["workload_scope"] != "paced-runtime"):
            raise ValueError("Each hardware/VM pair needs matching builds and completed paced state and audio")
    if hardware_reference["build"]["rom"] != hardware_candidate["build"]["rom"]:
        raise ValueError("Reference and candidate need the same ROM")
    ref_hw, ref_vm, new_hw, new_vm = (item["indexed_windows"] for pair in pairs for item in pair)
    common = ref_hw.keys() & ref_vm.keys() & new_hw.keys() & new_vm.keys()
    bounds = {key: tuple(map(int, key.split("-"))) for key in common}
    def remaining(row):
        # The residual includes graphics and other host work.
        return row[0] - row[2] - row[3]

    def direction(value, reference):
        return (value > reference) - (value < reference)

    sections = (("boot", 180, 2159), ("menu", 2160, 4499),
                ("transition", 4500, 5219), ("battle", 5220, 6119))
    report = {}
    for name, start, end in sections:
        keys = [key for key, (first, last) in bounds.items() if start <= first <= last <= end]
        keys = [key for key in keys if ref_vm[key][2] > 0]
        if not keys:
            raise ValueError(f"No matched DB1 timing windows in {name}")
        scale = statistics.median(ref_hw[key][2] / ref_vm[key][2] for key in keys)
        offset = statistics.median(remaining(ref_hw[key]) - remaining(ref_vm[key]) for key in keys)
        predicted = statistics.median(max(1_000_000 / 59.9227, remaining(new_vm[key])
                                          + scale * new_vm[key][2] + offset) for key in keys)
        reference = statistics.median(ref_hw[key][0] for key in keys)
        observed = statistics.median(new_hw[key][0] for key in keys)
        raw = statistics.median(new_vm[key][0] for key in keys)
        report[name] = {
            "windows": len(keys), "simulation_scale": round(scale, 4),
            "other_work_offset_ms": round(offset / 1000, 3),
            "reference_hardware_ms": round(reference / 1000, 3),
            "candidate_vm_ms": round(raw / 1000, 3),
            "candidate_estimated_ms": round(predicted / 1000, 3),
            "candidate_hardware_ms": round(observed / 1000, 3),
            "candidate_absolute_error_ms": round(abs(predicted - observed) / 1000, 3),
            "predicts_direction": direction(predicted, reference) == direction(observed, reference),
        }
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("logs", type=Path, nargs="+")
    parser.add_argument("--from-frame", type=int, default=0)
    parser.add_argument("--match-windows", action="store_true",
                        help="Compare only identical frame ranges from two timing logs")
    parser.add_argument("--calibrate-ga36", action="store_true",
                        help="Fit DB1 Normal 1 on hardware/VM reference logs and validate on hardware/VM candidate logs")
    args = parser.parse_args()
    if args.match_windows and len(args.logs) != 2:
        parser.error("--match-windows requires exactly two logs")
    if args.calibrate_ga36:
        if len(args.logs) != 4 or args.match_windows or args.from_frame:
            parser.error("--calibrate-ga36 needs four logs in order: hardware reference, VM reference, hardware candidate, VM candidate")
        try:
            reports = [summarize(path, retain_windows=True) for path in args.logs]
            print(json.dumps(calibrate_ga36(*reports), indent=2))
        except (ValueError, OSError) as error:
            parser.error(str(error))
        return
    reports = [summarize(path, args.from_frame, args.match_windows) for path in args.logs]
    result = {"logs": reports}
    if len(reports) == 2:
        result["comparison"] = compare(*reports)
    if args.match_windows:
        result["matched_windows"] = compare_windows(*reports)
        for report in reports:
            del report["indexed_windows"]
    print(json.dumps(result, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
