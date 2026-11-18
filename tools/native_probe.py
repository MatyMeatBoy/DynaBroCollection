"""Finite native scene probe with isolated saves/settings and explicit failure checks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
from tools.windows_environment import portable_windows_env
from tools.audio_capture import inspect_audio_capture


def prepare_scene_script(script, output):
    lines, captures = [], []
    for line in script.splitlines():
        command = line.strip().split(maxsplit=1)
        if command and command[0] == 'EXIT':
            continue
        if command and command[0] == 'SCREENSHOT':
            if len(command) != 2:
                raise ValueError('SCREENSHOT requires a path')
            name = command[1].replace('\\', '/').rsplit('/', 1)[-1]
            if not name.lower().endswith('.png') or name in ('.png', '..png'):
                raise ValueError('Capture must have a PNG filename')
            target = output / name
            if target in captures:
                raise ValueError('Duplicate capture filename: ' + name)
            captures.append(target)
            line = 'SCREENSHOT ' + target.as_posix()
        lines.append(line)
    return '\n'.join(lines), captures


def missing_captures(captures):
    failures = []
    for path in captures:
        if not path.is_file():
            failures.append('Missing capture: ' + path.name)
        elif not path.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'):
            failures.append('Invalid PNG capture: ' + path.name)
    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--script', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=6010)
    parser.add_argument('--audio', action='store_true')
    parser.add_argument('--require-audio-signal', action='store_true', help='Require sustained non-DC mixer output (implies --audio)')
    parser.add_argument('--pacing', choices=['default','accurate','fiber'], default='default')
    parser.add_argument('--language', choices=['en', 'ja'], help='Seed the isolated profile language preference')
    parser.add_argument('--widescreen', help='Explicit game presentation mode (off, fit, stage or W:H)')
    parser.add_argument('--host-settings-test', action='store_true', help='Open embedded settings and intentionally Quit Game after captures')
    parser.add_argument('--achievement-ui-test', action='store_true', help='Exercise the paused achievement list with real SDL events')
    parser.add_argument('--lookup', choices=['binary', 'cached'], help='Compare native descriptor lookup policies')
    parser.add_argument('--cpu-core', type=int, help='Pin a Windows measurement to one logical processor')
    parser.add_argument('--replay', type=Path, help='Validate all executions from an existing diagnostic recording')
    parser.add_argument('--portable-env', action='store_true', help='Exclude development DLL directories from the child environment')
    parser.add_argument('--timeout', type=int, default=180)
    args = parser.parse_args()
    if args.require_audio_signal or args.host_settings_test: args.audio = True
    if args.replay and args.script:
        parser.error('Replay supplies its own inputs; do not combine with a scene script')
    output = args.output.resolve()
    if output.exists():
        parser.error('Use a fresh output directory to preserve earlier evidence')
    output.mkdir(parents=True)
    profile = output / 'profile'
    profile.mkdir()
    if args.language:
        (profile / 'dynabrothers-language.ini').write_text('english=' + ('1' if args.language == 'en' else '0') + '\n')
    env = dict(os.environ)
    env.pop('DYNA_ACHIEVEMENT_UI_TEST', None)
    env.pop('GENESIS_HOST_SETTINGS_TEST', None)
    if args.achievement_ui_test:
        args.audio = True
        env['DYNA_ACHIEVEMENT_UI_TEST'] = str(output / 'achievements.bmp')
    env.pop('GENESIS_FORCE_INTERP', None)
    if args.lookup:
        env['DYNA_AOT_LOOKUP'] = args.lookup
    env['GENESIS_USER_DIR'] = str(profile)
    env['SDL_AUDIODRIVER'] = 'dummy'
    if args.portable_env:
        env = portable_windows_env(env)
    elif Path('C:/msys64/ucrt64/bin').exists():
        env['PATH'] = 'C:/msys64/ucrt64/bin;' + env['PATH']
    argv = [str(args.binary.resolve()), str(args.rom.resolve()), '--no-launcher']
    if args.pacing != 'default': argv += ['--pacing', args.pacing]
    if args.widescreen:
        argv += ['--widescreen', args.widescreen]
    if args.replay:
        argv += ['--replay-diagnostic', str(args.replay.resolve())]
    if args.host_settings_test:
        env.update(GENESIS_HOST_SETTINGS_TEST='1', LNG_TEST_HIDDEN='1',
                   LNG_SCRIPT='wait:3;shot:'+(output/'same-window.png').as_posix()+';view:controller;wait:3;shot:'+(output/'controls.png').as_posix()+';quit')
    if args.audio:
        env['LNG_TEST_HIDDEN']='1'
        if not args.host_settings_test: env['LNG_SCRIPT']='1'
        argv += ['--measure-runtime', str(args.frames), '--runtime-uncapped', '--wav', str(output / 'audio.wav')]
    else:
        argv += ['--benchmark', str(args.frames)]
    captures = []
    if args.script:
        script, captures = prepare_scene_script(args.script.read_text(encoding='utf-8'), output)
        path = output / 'scene.input'
        path.write_text(script, encoding='utf-8')
        argv += ['--input-script', str(path)]
    start = time.monotonic()
    try:
        process = subprocess.Popen(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if args.cpu_core is not None:
            import ctypes
            if os.name != 'nt' or not 0 <= args.cpu_core < 64:
                process.kill();process.communicate();raise ValueError('CPU affinity requires a Windows logical processor 0..63')
            kernel = ctypes.WinDLL('kernel32', use_last_error=True)
            kernel.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
            kernel.SetProcessAffinityMask.restype = ctypes.c_int
            if not kernel.SetProcessAffinityMask(int(process._handle), 1 << args.cpu_core):
                process.kill();process.communicate();raise ctypes.WinError(ctypes.get_last_error())
        raw, _ = process.communicate(timeout=args.timeout)
        log = raw.decode('utf-8', errors='replace')
        code = process.returncode
    except subprocess.TimeoutExpired as error:
        process.kill();raw, _ = process.communicate()
        log = raw.decode('utf-8', errors='replace')
        code = 'timeout'
    (output / 'run.log').write_text(log, encoding='utf-8')
    failures = [line for line in log.splitlines() if any(marker in line for marker in
                ('[AOT] missing', '[ILLEGAL]', 'unexpectedly', 'native main program miss', 'HALT st=',
                 'IRQ instruction guard', '[AOT-IRQ] INVALID', 'replay file write failed', 'ROM mismatch'))]
    failures.extend(missing_captures(captures))
    if args.achievement_ui_test:
        for marker in ['PASS 6 modal presentations; CPU/RAM/VDP unchanged', 'PASS real SDL Back event closed modal']:
            if '[achievement-ui-test] ' + marker not in log:
                failures.append('Missing achievement modal evidence: ' + marker)
        capture = output / 'achievements.bmp'
        if not capture.is_file() or capture.read_bytes()[:2] != b'BM':
            failures.append('Missing achievement list capture')
    if env.get('DYNA_IRQ_SELFTEST') and '[AOT-IRQ] PASS level=4' not in log:
        failures.append('H-int exception-frame check was not exercised')
    if args.language:
        expected = '[language] ' + ('English' if args.language == 'en' else 'Japanese original')
        if expected not in log:
            failures.append('Requested language was not applied')
    summaries = re.findall(r'GENESISRECOMP_BENCHMARK (\{[^\n]+\})', log)
    measurement = json.loads(summaries[-1]) if summaries else None
    audio_analysis = None
    if args.audio:
        try:
            audio_analysis = inspect_audio_capture(output / 'audio.wav')
            if args.require_audio_signal and not audio_analysis['signal_present']:
                failures.append('No sustained non-DC mixer signal')
        except (OSError, ValueError, EOFError) as error:
            failures.append('Audio capture analysis failed: ' + str(error))
    complete = bool(code == 0 and not failures and measurement and measurement.get('complete') and measurement.get('frames') == args.frames)
    if args.host_settings_test:
        captures.extend([output/'same-window.png',output/'controls.png'])
        failures.extend(missing_captures(captures))
        marker='[host-settings-test] PASS same game window/renderer; CPU/RAM/VDP unchanged'
        if marker not in log: failures.append('Missing integrated host settings preservation evidence')
        complete=bool(code==2 and not failures and marker in log and measurement and measurement.get('frames')==30 and not measurement.get('complete'))
    matched = re.findall(r'\[feedback-replay\] all (\d+) recorded executions matched', log)
    if args.replay:
        complete = bool(code == 0 and not failures and matched and measurement and measurement.get('frames') == int(matched[-1]))
    report = dict(passed=complete, returncode=code, failures=failures, elapsed_seconds=time.monotonic() - start,
                  binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                  rom_sha256=hashlib.sha256(args.rom.read_bytes()).hexdigest(), frames=args.frames,
                  language_request=args.language or 'default',
                  lookup_request=args.lookup or 'default',
                  cpu_core=args.cpu_core,
                  replay_executions=int(matched[-1]) if matched else None,
                  portable_environment=args.portable_env,
                  pacing_request=args.pacing, audio_analysis=audio_analysis,
                  measurement=measurement, screenshots=[p.name for p in output.glob('*.png')],
                  scope=('Controlled Quit at 30 frames verifies host surface and guest immutability; finite benchmark intentionally incomplete' if args.host_settings_test else 'Finite execution only; visual/translation correctness and campaign acceptance are separate'))
    (output / 'probe.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if complete else 1)


if __name__ == '__main__':
    main()
