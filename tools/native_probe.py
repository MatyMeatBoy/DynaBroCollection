"""Finite native scene probe with isolated saves/settings and explicit failure checks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time


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
    parser.add_argument('--timeout', type=int, default=180)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error('Use a fresh output directory to preserve earlier evidence')
    output.mkdir(parents=True)
    profile = output / 'profile'
    profile.mkdir()
    env = dict(os.environ)
    env.pop('GENESIS_FORCE_INTERP', None)
    env['GENESIS_USER_DIR'] = str(profile)
    env['SDL_AUDIODRIVER'] = 'dummy'
    if Path('C:/msys64/ucrt64/bin').exists():
        env['PATH'] = 'C:/msys64/ucrt64/bin;' + env['PATH']
    argv = [str(args.binary.resolve()), str(args.rom.resolve()), '--no-launcher', '--pacing', 'accurate']
    if args.audio:
        env.update(LNG_SCRIPT='1', LNG_TEST_HIDDEN='1')
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
        run = subprocess.run(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=args.timeout)
        log = run.stdout.decode('utf-8', errors='replace')
        code = run.returncode
    except subprocess.TimeoutExpired as error:
        log = (error.stdout or b'').decode('utf-8', errors='replace')
        code = 'timeout'
    (output / 'run.log').write_text(log, encoding='utf-8')
    failures = [line for line in log.splitlines() if any(marker in line for marker in
                ('[AOT] missing', '[ILLEGAL]', 'unexpectedly', 'native main program miss', 'HALT st=',
                 'IRQ instruction guard', '[AOT-IRQ] INVALID', 'replay file write failed', 'ROM mismatch'))]
    failures.extend(missing_captures(captures))
    if env.get('DYNA_IRQ_SELFTEST') and '[AOT-IRQ] PASS level=4' not in log:
        failures.append('H-int exception-frame check was not exercised')
    summaries = re.findall(r'GENESISRECOMP_BENCHMARK (\{[^\n]+\})', log)
    measurement = json.loads(summaries[-1]) if summaries else None
    complete = bool(code == 0 and not failures and measurement and measurement.get('complete') and measurement.get('frames') == args.frames)
    report = dict(passed=complete, returncode=code, failures=failures, elapsed_seconds=time.monotonic() - start,
                  binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                  rom_sha256=hashlib.sha256(args.rom.read_bytes()).hexdigest(), frames=args.frames,
                  measurement=measurement, screenshots=[p.name for p in output.glob('*.png')],
                  scope='Finite execution only; visual/translation correctness and campaign acceptance are separate')
    (output / 'probe.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if complete else 1)


if __name__ == '__main__':
    main()
