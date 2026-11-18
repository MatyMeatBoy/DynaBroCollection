"""Restore a private development toolkit without changing an existing checkout."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys


def environment(kit):
    env = dict(os.environ)
    for key in list(env):
        if key.startswith('GIT_') or key in ('PYTHONHOME', 'PYTHONPATH', 'CC', 'CXX'):
            env.pop(key, None)
    system = Path(env.get('SystemRoot', 'C:/Windows'))
    compiler = kit / 'toolchain/ucrt64'
    env.update(PATH=os.pathsep.join(map(str, [compiler/'bin', kit/'git/cmd',
        kit/'git/mingw64/bin', kit/'git/usr/bin', kit/'python', system/'System32', system])),
        DYNA_TOOLCHAIN_BIN=str(compiler/'bin'), CMAKE_PREFIX_PATH=str(compiler),
        CC=str(compiler/'bin/gcc.exe'), CXX=str(compiler/'bin/g++.exe'),
        GIT_CONFIG_NOSYSTEM='1', GIT_CONFIG_GLOBAL=os.devnull,
        GIT_TERMINAL_PROMPT='0', PYTHONNOUSERSITE='1', PYTHONDONTWRITEBYTECODE='1')
    return env


def run(argv, env, cwd=None):
    subprocess.run(list(map(str, argv)), env=env, cwd=cwd, check=True)


def verify(kit, manifest):
    for relative, expected in manifest['files'].items():
        name = Path(relative)
        if name.is_absolute() or '..' in name.parts:
            raise ValueError('Invalid toolkit member path')
        path = kit/name
        if not path.is_file() or path.stat().st_size != expected['size']:
            raise RuntimeError('Missing or changed toolkit file: '+relative)
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected['sha256']:
            raise RuntimeError('Toolkit checksum mismatch: '+relative)
    print('Toolkit file inventory: PASS', flush=True)


def restore(kit, destination, manifest, env):
    if destination.exists():
        raise FileExistsError('Existing work is preserved; choose a new destination')
    destination.parent.mkdir(parents=True, exist_ok=True)
    git = kit/'git/cmd/git.exe'
    run([git, 'clone', '--branch', manifest['branch'],
         kit/'bundles/dynabrothers.bundle', destination], env)
    actual = subprocess.check_output([str(git), '-C', str(destination),
        'rev-parse', 'HEAD'], env=env, text=True).strip()
    if actual != manifest['source_commit']:
        raise RuntimeError('Restored source does not match the toolkit')
    run([git, '-C', destination, 'remote', 'set-url', 'origin', manifest['origin']], env)
    run([kit/'python/python.exe', '-B', '-E', '-s',
         kit/'tools/restore_dependencies.py', '--toolkit', kit, destination], env)
    for folder, name in [('inputs', 'Dynabrothers'), ('generated', 'generated'),
                         ('cache/roms', 'local/roms')]:
        shutil.copytree(kit/folder, destination/name)
    local = destination/'local/toolkit'
    local.mkdir(parents=True, exist_ok=True)
    evidence = destination/'offline-restore-evidence.json'
    if evidence.exists():
        shutil.move(str(evidence), str(local/'dependency-restore.json'))
    for repo, patch, indexed in [
        ('segagenesisrecomp', '0001-explicit-pc-mainloop.patch', False),
        ('recomp-ui', '0002-dyna-launcher.patch', True)]:
        command = [git, '-C', destination/'upstream'/repo, 'apply']
        if indexed:
            command.append('--index')
        run(command+[destination/'patches'/patch], env)
    print('Restored source, pinned dependencies, private inputs and generated cache:', destination)


def check(kit, destination, env):
    compiler = kit/'toolchain/ucrt64'
    build = destination/'build/toolkit-check'
    cmake, ctest = compiler/'bin/cmake.exe', compiler/'bin/ctest.exe'
    run([cmake, '-S', destination, '-B', build, '-G', 'Ninja',
         '-DCMAKE_BUILD_TYPE=Release', '-DGENESIS_NATIVE_IPO=OFF',
         '-DCMAKE_C_COMPILER='+str(compiler/'bin/gcc.exe'),
         '-DCMAKE_CXX_COMPILER='+str(compiler/'bin/g++.exe'),
         '-DCMAKE_PREFIX_PATH='+str(compiler)], env)
    # Fresh CTest JSON may omit commands until executables exist. CMake's
    # generated test file already records the actual Windows target paths.
    test_file = (build/'CTestTestfile.cmake').read_text(encoding='utf-8')
    targets = sorted({Path(p.replace('\\','/')).stem for p in re.findall(r'"([^"\r\n]+\.exe)"', test_file)})
    if not targets:
        raise RuntimeError('No native contract targets were generated')
    run([cmake, '--build', build, '--target', *targets, '--parallel', '4'], env)
    run([ctest, '--test-dir', build, '--output-on-failure'], env)
    env = dict(env)
    env.update(DB1_LANGUAGE_TEST_ROOT=str(destination),
        DB1_SPEECH_TEST_ROM=str(destination/'local/roms/db1-en-950A8FC3.md'),
        DB1_ENDING_TEST_ROM=str(destination/'local/roms/db1-en-38B20F8F.md'))
    run([kit/'python/python.exe', '-B', '-E', '-s', '-m', 'unittest',
         'discover', '-s', 'tests', '-p', 'test_*.py'], env, destination)
    return len(targets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--destination', type=Path)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--check', action='store_true', help='Build native contracts and run source tests')
    args = parser.parse_args()
    kit = args.kit.resolve()
    manifest = json.loads((kit/'TOOLKIT.json').read_text(encoding='utf-8'))
    verify(kit, manifest)
    if args.verify_only:
        return
    destination = (args.destination or kit/'workspace/DynaBrothers').resolve()
    env = environment(kit)
    if not destination.exists():
        restore(kit, destination, manifest, env)
    elif not args.check:
        raise FileExistsError('Existing checkout preserved; use --check or a new destination')
    if args.check:
        count = check(kit, destination, env)
        report = dict(passed=True, source_commit=manifest['source_commit'],
            native_contracts=count, python_tests='all passed',
            toolkit_candidate_manifest_sha256=hashlib.sha256((kit/'TOOLKIT.json').read_bytes()).hexdigest(),
            scope='Offline source/dependency restore and focused source checks with relocated tools; not a full AOT regeneration')
        (destination/'local/toolkit/verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Ready. Read docs/handoff/CONTINUE-ON-ANOTHER-PC.md in the restored checkout.')


if __name__ == '__main__':
    main()
