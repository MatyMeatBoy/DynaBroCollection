"""Assemble and close a private, offline continuation toolkit. Never publish it."""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path
import zipfile
from tools.package_checkpoint import ROOT, digest


def inventory(root):
    return {p.relative_to(root).as_posix():dict(size=p.stat().st_size,sha256=digest(p))
            for p in sorted(root.rglob('*')) if p.is_file() and p.name != 'TOOLKIT.json'}


def local_tool_copy(source, target):
    # Local staging only: delivered ZIP members are independent byte copies.
    # Link immutable tool binaries to avoid recopying thousands of files.
    try:
        os.link(source, target)
        return target
    except OSError:
        return shutil.copy2(source, target)


def stage(destination, runtime):
    if subprocess.check_output(['git','-C',str(ROOT),'status','--porcelain'],text=True).strip():
        raise RuntimeError('Commit source before freezing toolkit')
    destination.mkdir(parents=True,exist_ok=False)
    deps=ROOT/'local/toolkit-deps-018'; chain=ROOT/'local/toolkit-toolchain-018'
    for src,target in [(deps/'portable-git','git'),(deps/'bundles','bundles'),
        (chain/'ucrt64','toolchain/ucrt64'),(chain/'python','python'),
        (ROOT/'generated','generated'),(ROOT/'Dynabrothers','inputs'),
        (ROOT/'local/roms','cache/roms'),(runtime,'play')]:
        shutil.copytree(src,destination/target,ignore=shutil.ignore_patterns('__pycache__','*.pyc'),
            copy_function=local_tool_copy if target in ('git','toolchain/ucrt64','python') else shutil.copy2)
    metadata=destination/'metadata';metadata.mkdir()
    for src,name in [(deps/'portable-git-files.json','portable-git-files.json'),
        (deps/'verification.json','dependency-staging-verification.json'),
        (chain/'stage-provenance.json','toolchain-provenance.json'),
        (chain/'PAYLOAD-MANIFEST.json','toolchain-payload.json')]:shutil.copy2(src,metadata/name)
    pins=json.loads((deps/'dependencies.json').read_text(encoding='utf-8'))
    pins['portable_git']['path']='git'
    (metadata/'dependencies.json').write_text(json.dumps(pins,indent=2),encoding='utf-8')
    tools=destination/'tools';tools.mkdir()
    shutil.copy2(deps/'restore_dependencies.py',tools/'restore_dependencies.py')
    shutil.copy2(ROOT/'tools/toolkit_bootstrap.py',tools/'toolkit_bootstrap.py')
    revision=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip()
    branch=subprocess.check_output(['git','-C',str(ROOT),'branch','--show-current'],text=True).strip()
    subprocess.run(['git','-C',str(ROOT),'bundle','create',str(destination/'bundles/dynabrothers.bundle'),branch],check=True)
    for folder in ['db2-english-inventory','db2-bio-resource','db2-quit-proof','db2-visor-research']:
        src=ROOT/'local'/folder
        if src.exists():
            out=destination/'evidence'/folder
            for f in src.rglob('*'):
                if f.is_file() and f.suffix.lower() in {'.md','.txt','.json','.py','.input','.ram','.vram','.png'} and 'bin' not in f.parts and f.stat().st_size<8*1024*1024:
                    target=out/f.relative_to(src);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,target)
    out=destination/'evidence/db2-player';out.mkdir(parents=True)
    for name in ['final018-result.json','player-normal2-portable.input']:
        shutil.copy2(ROOT/'local/db2-world-agent'/name,out/name)
    shutil.copy2(ROOT/'docs/handoff/CONTINUE-ON-ANOTHER-PC.md',destination/'START-HERE.md')
    command='@echo off\ncd /d "%~dp0"\n"%~dp0python\\python.exe" -B -E -s "%~dp0tools\\toolkit_bootstrap.py"'
    for name,extra in [('Restore-Workspace.cmd',''),('Check-Workspace.cmd',' --check')]:
        (destination/name).write_text(command+extra+'\nif errorlevel 1 pause\n',encoding='ascii')
    (destination/'Open-Dev-Shell.cmd').write_text('@echo off\ncd /d "%~dp0"\nset "PATH=%~dp0toolchain\\ucrt64\\bin;%~dp0git\\cmd;%~dp0git\\mingw64\\bin;%~dp0git\\usr\\bin;%~dp0python;%SystemRoot%\\System32;%SystemRoot%"\nset "DYNA_TOOLCHAIN_BIN=%~dp0toolchain\\ucrt64\\bin"\nset "CMAKE_PREFIX_PATH=%~dp0toolchain\\ucrt64"\nset "GIT_CONFIG_NOSYSTEM=1"\nset "GIT_CONFIG_GLOBAL=NUL"\nif exist "workspace\\DynaBrothers" cd /d "workspace\\DynaBrothers"\ncmd /k\n',encoding='ascii')
    manifest=dict(version=(ROOT/'VERSION').read_text().strip(),private=True,status='candidate',
        source_commit=revision,branch=branch,origin='https://github.com/MatyMeatBoy/DynaBrothers1-2.git',
        runtime_source_commit=json.loads((runtime/'PACKAGE.json').read_text(encoding='utf-8'))['source_commit'],
        scope='Offline development continuation; original dumps and derived code included privately',files=inventory(destination))
    (destination/'TOOLKIT.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps(dict(candidate=str(destination),files=len(manifest['files']),bytes=sum(f['size'] for f in manifest['files'].values())),indent=2))


def close(candidate,proof,archive):
    manifest_path=candidate/'TOOLKIT.json';manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest['status']!='candidate' or inventory(candidate)!=manifest['files']:raise RuntimeError('Toolkit candidate changed')
    evidence=json.loads(proof.read_text(encoding='utf-8'))
    if not evidence['passed'] or evidence['source_commit']!=manifest['source_commit'] or evidence['toolkit_candidate_manifest_sha256']!=digest(manifest_path):raise RuntimeError('Wrong toolkit verification')
    if archive.exists():raise FileExistsError('Preserve delivered bytes')
    manifest.update(status='ready',verification=evidence)
    manifest_path.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    with zipfile.ZipFile(archive,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in sorted(candidate.rglob('*')):
            if f.is_file():z.write(f,'DynaBrothers-Development-Toolkit/'+f.relative_to(candidate).as_posix())
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:raise RuntimeError('Archive CRC failure')
        for name,item in manifest['files'].items():
            if hashlib.sha256(z.read('DynaBrothers-Development-Toolkit/'+name)).hexdigest()!=item['sha256']:raise RuntimeError('Archive member hash mismatch: '+name)
    result=dict(private=True,status='closed',archive=str(archive),sha256=digest(archive),bytes=archive.stat().st_size,source_commit=manifest['source_commit'])
    archive.with_suffix('.sha256').write_text(result['sha256']+'  '+archive.name+'\n',encoding='ascii')
    archive.with_suffix('.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result,indent=2))


def main():
    p=argparse.ArgumentParser(description=__doc__);s=p.add_subparsers(dest='command',required=True)
    a=s.add_parser('stage');a.add_argument('destination',type=Path);a.add_argument('--runtime',type=Path,required=True)
    a=s.add_parser('close');a.add_argument('candidate',type=Path);a.add_argument('--proof',type=Path,required=True);a.add_argument('--archive',type=Path,required=True)
    a=p.parse_args()
    if a.command=='stage':stage(a.destination.resolve(),a.runtime.resolve())
    else:close(a.candidate.resolve(),a.proof.resolve(),a.archive.resolve())

if __name__=='__main__':main()
