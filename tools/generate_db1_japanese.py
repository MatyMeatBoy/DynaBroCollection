"""Emit Japanese-only native operations; reuse byte-identical English entries."""
import csv
import os
from pathlib import Path
import subprocess
from tools.rompatch import identity


def generate(source: Path, root: Path):
    original = source.read_bytes()
    if identity(original)['sha256'] != 'b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c':
        raise ValueError('Japanese DB1 identity mismatch')
    english = (root / 'local/roms/db1-en-v01.md').read_bytes()
    output = root / 'generated/db1jp'
    output.mkdir(parents=True, exist_ok=True)
    covered, entries = [], []
    with (root / 'generated/db1step/db1step_coverage.csv').open() as stream:
        for row in csv.DictReader(stream):
            pc, words, cycles = int(row['pc'], 16), int(row['words']), int(row['nominal_cycles'])
            if row['kind'] not in ('native', 'illegal'):
                continue
            raw = english[pc:pc + words * 2]
            if raw == original[pc:pc + words * 2]:
                numbers = [hex(pc), str(words), str(cycles)] + [hex(int.from_bytes(raw[i:i + 2], 'big')) for i in range(0, len(raw), 2)]
                covered.append(' '.join(numbers))
            else:
                entries.append(hex(pc))
    (output / 'covered.txt').write_text('\n'.join(covered))
    (output / 'entries.txt').write_text('\n'.join(entries))
    env = dict(os.environ)
    if Path('C:/msys64/ucrt64/bin').exists():
        env['PATH'] = 'C:/msys64/ucrt64/bin;' + env['PATH']
    subprocess.run([str(root / 'build/recompiler/M68kStepRecomp.exe'), str(source), str(output), 'db1jp', '0',
                    '--entries-file', str(output / 'entries.txt'), '--covered-entries-file', str(output / 'covered.txt')], env=env, check=True)


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('rom', type=Path)
    args = p.parse_args()
    generate(args.rom.resolve(), Path(__file__).resolve().parent.parent)
