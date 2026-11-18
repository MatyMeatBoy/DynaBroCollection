"""Reproduce local ROM preparation and experimental native builds."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.rompatch import apply_bps, create_bps

ROOT = Path(__file__).resolve().parent.parent
ENGINE_SHA = '4b2247d2b33a45623a686043a48c2021744c39ab'
UI_SHA = 'd55bf8304c455dbc5773a000c919f6ba0a26d6bb'
CONTROLLER_DB_SHA = '4e87e3c6c2574f49e1cdf8526b6f829b61e5ccb3'
DB1_SHA = 'b4918a9156365d500ff415ce2a5197798d7529c3be91ce8370c6900df57d893c'
DB2_SHA = '1d3f08e235fa8e51b9656820fabfb5b703acaf36bc34d5378a3f9613d453bb76'
PATCH_SHA = '01be7ab1e20d5603bca9da531e18f9f0baf178d0e64aa5c049c3a1f3a92000d2'


def verify_engine_checkout(engine: Path, expected_sha: str, patch: Path):
    def git(*args):
        return subprocess.check_output(['git', '-C', str(engine), *args])

    if git('rev-parse', 'HEAD').decode().strip() != expected_sha:
        raise RuntimeError('Engine commit differs from the qualified pin')
    if git('diff', '--binary', 'HEAD', '--') != patch.read_bytes():
        raise RuntimeError('Engine has changes beyond the exact approved adapter patch; existing work was preserved')
    status = git('status', '--porcelain', '--untracked-files=normal').decode()
    if any(line.startswith('??') for line in status.splitlines()):
        raise RuntimeError('Engine has untracked files; preserve and inspect them before building')
    submodules = git('submodule', 'status', '--recursive').decode()
    if any(line and line[0] != ' ' for line in submodules.splitlines()):
        raise RuntimeError('Engine submodules differ from their pinned commits or are not initialized')
    dirty = git('submodule', 'foreach', '--quiet', '--recursive', 'git status --porcelain --untracked-files=normal')
    if dirty.strip():
        raise RuntimeError('Engine submodule contains local changes; existing work was preserved')


def find_by_hash(root, digest, extensions):
    for path in root.rglob('*'):
        if path.is_file() and path.suffix.lower() in extensions and hashlib.sha256(path.read_bytes()).hexdigest() == digest:
            return path
    raise RuntimeError(f'Required input SHA256 {digest} not found in {root}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inputs', type=Path, default=ROOT.parent / 'Dynabrothers')
    parser.add_argument('--target', choices=['db1', 'db2', 'both'], default='both')
    parser.add_argument('--jobs', type=int, default=6)
    args = parser.parse_args()
    os.chdir(ROOT)
    # Select the available toolchain without changing machine-wide PATH.
    if Path('C:/msys64/ucrt64/bin/cmake.exe').exists():
        os.environ['PATH'] = 'C:/msys64/ucrt64/bin;' + os.environ['PATH']
    cmake = shutil.which('cmake')
    if not cmake:
        raise RuntimeError('CMake and a C/C++ toolchain are required')
    logdir = ROOT / 'local/evidence/build'
    logdir.mkdir(parents=True, exist_ok=True)

    def run(name, argv, cwd=ROOT):
        print(name, flush=True)
        with (logdir / (name + '.log')).open('wb') as log:
            result = subprocess.run([str(x) for x in argv], cwd=cwd, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(f'{name} failed ({result.returncode}); see {logdir / (name + ".log")}')

    engine = ROOT / 'upstream/segagenesisrecomp'
    if not engine.exists():
        engine.parent.mkdir(exist_ok=True)
        run('clone-engine', ['git', 'clone', '--no-checkout', 'https://github.com/mstan/segagenesisrecomp.git', engine])
        run('pin-engine', ['git', 'checkout', '--detach', ENGINE_SHA], cwd=engine)
        run('engine-submodules', ['git', 'submodule', 'update', '--init', '--recursive'], cwd=engine)
    actual = subprocess.check_output(['git', '-C', str(engine), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != ENGINE_SHA:
        raise RuntimeError(f'Engine checkout differs from qualified pin: {actual}; preserve and inspect existing work')
    patch = ROOT / 'patches/0001-explicit-pc-mainloop.patch'
    applied = subprocess.run(['git', 'apply', '--reverse', '--check', str(patch)], cwd=engine, capture_output=True).returncode == 0
    if not applied:
        run('check-engine-adapter', ['git', 'apply', '--check', patch], cwd=engine)
        run('apply-engine-adapter', ['git', 'apply', patch], cwd=engine)
    verify_engine_checkout(engine, ENGINE_SHA, patch)
    ui = ROOT / 'upstream/recomp-ui'
    if not ui.exists():
        run('clone-ui', ['git', 'clone', '--no-checkout', 'https://github.com/mstan/recomp-ui.git', ui])
        run('pin-ui', ['git', 'checkout', '--detach', UI_SHA], cwd=ui)
    ui_head = subprocess.check_output(['git', '-C', str(ui), 'rev-parse', 'HEAD'], text=True).strip()
    ui_dirty = subprocess.check_output(['git', '-C', str(ui), 'status', '--porcelain', '--untracked-files=normal'], text=True)
    if ui_head != UI_SHA or ui_dirty:
        raise RuntimeError('Shared UI differs from the qualified clean pin; existing work was preserved')
    controllers = ROOT / 'upstream/controller-db'
    if not controllers.exists():
        run('clone-controller-db', ['git', 'clone', '--no-checkout', 'https://github.com/mdqinc/SDL_GameControllerDB.git', controllers])
        run('pin-controller-db', ['git', 'checkout', '--detach', CONTROLLER_DB_SHA], cwd=controllers)
    if subprocess.check_output(['git', '-C', str(controllers), 'rev-parse', 'HEAD'], text=True).strip() != CONTROLLER_DB_SHA or subprocess.check_output(['git', '-C', str(controllers), 'status', '--porcelain'], text=True):
        raise RuntimeError('Controller database differs from the qualified clean pin')
    db1 = find_by_hash(args.inputs, DB1_SHA, {'.md', '.bin'})
    db2 = find_by_hash(args.inputs, DB2_SHA, {'.md', '.bin'})
    bps = find_by_hash(args.inputs, PATCH_SHA, {'.bps'})
    translated = apply_bps(db1.read_bytes(), bps.read_bytes())
    english = ROOT / 'local/roms/db1-en-v01.md'
    english.parent.mkdir(parents=True, exist_ok=True)
    if english.exists():
        if english.read_bytes() != translated:
            raise RuntimeError('Existing English ROM differs; preserve it before rebuilding')
    else:
        with english.open('xb') as out:
            out.write(translated)
    from tools.translate_db2 import translate
    import json
    import zlib
    from tools.translate_db1 import translate as translate_db1
    catalog_db1 = json.loads((ROOT / 'translations/db1-en.json').read_text(encoding='utf-8'))
    db1_en = translate_db1(translated, catalog_db1)
    db1_crc = f'{zlib.crc32(db1_en):08X}'
    db1_cache = ROOT / f'local/roms/db1-en-{db1_crc}.md'
    if db1_cache.exists():
        if db1_cache.read_bytes() != db1_en:
            raise RuntimeError('Existing DB1 English cache differs')
    else:
        with db1_cache.open('xb') as out:
            out.write(db1_en)
    db1_step_dir = ROOT / 'generated/db1step'
    db1_step_dir.mkdir(parents=True, exist_ok=True)
    (db1_step_dir / 'translation_identity_db1.h').write_text('#pragma once\n#define DYNA_DB1_ENGLISH_CRC 0x' + db1_crc + 'u\n')
    catalog = json.loads((ROOT / 'translations/db2special-en.json').read_text(encoding='utf-8'))
    db2_en = translate(db2.read_bytes(), catalog)
    from tools.generate_language import emit_image
    previous_db1 = translate_db1(translated, {key: value for key, value in catalog_db1.items() if key != 'credits_tables'})
    if hashlib.sha256(previous_db1).hexdigest() != '04d8be46c5e0597efa3315537a694e8cc314e66c0e519420cab9ba222195a4c4':
        raise RuntimeError('DB1 v0.2 compatibility image differs from its qualified identity')
    emit_image([db1.read_bytes(), db1_en, translated, previous_db1], 'db1', ROOT / 'generated/language')
    emit_image([db2.read_bytes(), db2_en], 'db2', ROOT / 'generated/language')
    for name, original, target, crc in [('db1', db1.read_bytes(), db1_en, db1_crc),
                                         ('db2special', db2.read_bytes(), db2_en, f'{zlib.crc32(db2_en):08X}')]:
        cumulative = create_bps(original, target)
        if apply_bps(original, cumulative) != target:
            raise RuntimeError('Cumulative translation patch roundtrip failed')
        patch_path = ROOT / f'local/patches/{name}-en-{crc}.bps'
        patch_path.parent.mkdir(parents=True, exist_ok=True)
        if patch_path.exists():
            if patch_path.read_bytes() != cumulative:
                raise RuntimeError('Existing cumulative patch differs')
        else:
            with patch_path.open('xb') as out:
                out.write(cumulative)
    db2_crc = f'{zlib.crc32(db2_en):08X}'
    db2_cache = ROOT / f'local/roms/db2special-en-{db2_crc}.md'
    if db2_cache.exists():
        if db2_cache.read_bytes() != db2_en:
            raise RuntimeError('Existing DB2 English cache differs')
    else:
        with db2_cache.open('xb') as out:
            out.write(db2_en)
    overlay_dir = ROOT / 'generated/db2overlay'
    overlay_dir.mkdir(parents=True, exist_ok=True)
    (overlay_dir / 'translation_identity.h').write_text('#pragma once\n#define DYNA_DB2_ENGLISH_CRC 0x' + db2_crc + 'u\n')
    run('configure-recompiler', [cmake, '-S', engine / 'recompiler', '-B', ROOT / 'build/recompiler', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release'])
    run('build-recompiler', [cmake, '--build', ROOT / 'build/recompiler', '--parallel', args.jobs])
    compiler = ROOT / 'build/recompiler/GenesisRecomp.exe'
    step = ROOT / 'build/recompiler/M68kStepRecomp.exe'
    # Both legacy catalogs satisfy the shared runtime's dispatch ABI; AOT
    # targets execute explicit-PC modules instead of these function bodies.
    for prefix, rom in [('db1', english), ('db2special', db2)]:
        output = ROOT / 'generated' / prefix
        output.mkdir(parents=True, exist_ok=True)
        run('functions-' + prefix, [compiler, rom, '--game', ROOT / 'games' / prefix / 'game.toml', '--output-dir', output])
    for prefix, legacy, rom in [('db1step', 'db1', english), ('db2step', 'db2special', db2)]:
        if args.target != 'both' and (args.target == 'db1') != (prefix == 'db1step'):
            continue
        output = ROOT / 'generated' / prefix
        output.mkdir(parents=True, exist_ok=True)
        if prefix == 'db2step':
            # Conservative code-address catalog; includes data candidates.
            # Counts are native operations, never recovered game functions.
            pcs = set(range(0x200, 0x62000, 2))
        else:
            # Include every aligned candidate in the observed program region.
            # Menu/map branches need entries beyond the boot trace. Runtime
            # opcode guards still reject stale or different instructions.
            pcs = set(range(0x200, 0x30000, 2))
            for part in (ROOT / 'generated' / legacy).glob('*part*.c'):
                pcs.update(int(x, 16) for x in re.findall(r'/\* \$([0-9A-F]{6}) \*/', part.read_text(encoding='utf-8')))
            pcs.update(int(x, 16) for x in (ROOT / 'games/db1/boot-gameplay-pcs.txt').read_text().splitlines() if x and not x.startswith('#'))
        entries = output / 'entries.txt'
        entries.write_text('\n'.join(hex(x) for x in sorted(pcs) if not x & 1 and x < rom.stat().st_size))
        run('operations-' + prefix, [step, rom, output, prefix, '0', '--entries-file', entries])
        if prefix == 'db1step':
            from tools.generate_db1_japanese import generate
            generate(db1, ROOT)
        else:
            from tools.db2_codec import decode
            overlay, consumed = decode(db2.read_bytes(), 0xE852)
            overlay_dir = ROOT / 'generated/db2overlay'
            overlay_dir.mkdir(exist_ok=True)
            (overlay_dir / 'overlay.bin').write_bytes(overlay)
            run('operations-db2-overlay', [step, overlay_dir / 'overlay.bin', overlay_dir, 'db2overlay', '0xFFE294', '--all'])
    run('configure-game', [cmake, '-S', ROOT, '-B', ROOT / 'build/dev', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DGENESIS_NATIVE_IPO=OFF'])
    targets = ['DynaBrothers1AOT', 'DynaBrothers2AOT'] if args.target == 'both' else ['DynaBrothers1AOT' if args.target == 'db1' else 'DynaBrothers2AOT']
    if os.name == 'nt' and args.target == 'both':
        targets.append('DynaBrothersLauncher')
    run('build-game', [cmake, '--build', ROOT / 'build/dev', '--target', *targets, '--parallel', args.jobs])
    print('Experimental native build complete. Translation completion and whole-game validation remain separate.')


if __name__ == '__main__':
    main()
