"""Verify the green settings surface inside the original Windows hub."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
from tools.windows_environment import portable_windows_env


def verify_hub(binary, rom, output):
    binary, rom, output = map(lambda p: Path(p).resolve(), (binary, rom, output))
    output.mkdir(parents=True, exist_ok=False)
    profile = output / 'profile'
    profile.mkdir()
    env = portable_windows_env(dict(os.environ))
    env.update(GENESIS_USER_DIR=str(profile), DYNA_HUB_SELFTEST='1', LNG_TEST_HIDDEN='1')
    commands = []
    for page in ['settings', 'controller', 'mods']:
        commands += ([] if page == 'settings' else ['view:' + page])
        commands += ['wait:3', 'shot:' + (output / (page + '.png')).as_posix()]
    env['LNG_SCRIPT'] = ';'.join(commands + ['quit'])
    startup = subprocess.STARTUPINFO()
    startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    process = subprocess.run([str(binary), '--game', 'db1', '--setup', '--rom', str(rom)],
                             env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             timeout=50, startupinfo=startup)
    log = process.stdout.decode('utf-8', errors='replace')
    (output / 'run.log').write_text(log, encoding='utf-8')
    captures = [output / (page + '.png') for page in ['settings', 'controller', 'mods']]
    valid = all(p.is_file() and p.read_bytes().startswith(b'\x89PNG\r\n\x1a\n') for p in captures)
    report = dict(passed=process.returncode == 1 and '[hub-test] PASS' in log and valid,
                  returncode=process.returncode, binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  scope='Controlled Quit cancels setup; original native HWND and renderer retained; isolated profile and portable DLL paths')
    (output / 'report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    return report


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('rom', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = verify_hub(args.binary, args.rom, args.output)
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if report['passed'] else 1)
