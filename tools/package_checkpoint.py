"""Stage, verify identity and close an immutable local checkpoint ZIP."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from tools.windows_environment import portable_windows_env
from tools.sdl_runtime import stage_sdl3
BIN=Path(os.environ.get('DYNA_TOOLCHAIN_BIN','C:/msys64/ucrt64/bin'))
EXECUTABLES=['DynaBrothersLauncher.exe','DynaBrothers1AOT.exe','DynaBrothers2AOT.exe']
DIAGNOSTICS=['DynaDb1VideoTest.exe','DynaDb1visibleTest.exe']


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def git(*args):return subprocess.check_output(['git','-C',str(ROOT),*args],text=True).strip()


def package_version(value):
    if not isinstance(value,str) or not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+',value) or len(value)>32:
        raise ValueError('Invalid package version')
    return value


def project_version():return package_version((ROOT/'VERSION').read_text(encoding='ascii').strip())


def validate_build_info(text,revision,version=None):
    version=package_version(version or project_version())
    match=re.search(r'\bgame '+re.escape(version)+r' git ([0-9a-f]{7,40});',text)
    if not match or not revision.startswith(match.group(1)):
        raise RuntimeError('Packaged executable does not identify the clean checkpoint source')


def content(root):
    """Package layout: DynaBrothers.exe + data/ + roms/ at the root; everything
    the launcher and games need lives in data/. Older candidates are flat."""
    return root/'data' if (root/'data').is_dir() else root


def inventory(folder):
    return {p.relative_to(folder).as_posix():dict(size=p.stat().st_size,sha256=digest(p))
            for p in sorted(folder.rglob('*')) if p.is_file() and p.name!='PACKAGE.json'}


def stage(candidate):
    if git('status','--porcelain'):raise RuntimeError('Commit the source checkpoint before staging')
    sys.path.insert(0,str(ROOT))
    from tools.build import verify_engine_checkout, ENGINE_SHA, UI_SHA, CONTROLLER_DB_SHA
    verify_engine_checkout(ROOT/'upstream/segagenesisrecomp',ENGINE_SHA,ROOT/'patches/0001-explicit-pc-mainloop.patch')
    verify_engine_checkout(ROOT/'upstream/recomp-ui',UI_SHA,ROOT/'patches/0002-dyna-launcher.patch')
    for name,expected in [('controller-db',CONTROLLER_DB_SHA)]:
        if git('-C',str(ROOT/'upstream'/name),'rev-parse','HEAD')!=expected or git('-C',str(ROOT/'upstream'/name),'status','--porcelain'):
            raise RuntimeError('Unqualified package dependency: '+name)
    revision=git('rev-parse','HEAD');version=project_version()
    readme=ROOT/f'docs/releases/v{version}-checkpoint.md'
    if not readme.is_file():raise RuntimeError('Missing checkpoint readme: '+str(readme))
    env=dict(os.environ);env['PATH']=str(BIN)+';'+env.get('PATH','')
    subprocess.run([str(BIN/'cmake.exe'),'--build',str(ROOT/'build/dev'),'--target',
        'DynaBrothers','DynaBrothersLauncher','DynaBrothers1AOT','DynaBrothers2AOT',
        *[Path(name).stem for name in DIAGNOSTICS],'--parallel','6'],env=env,check=True)
    if git('status','--porcelain'):raise RuntimeError('Source changed while building the checkpoint')
    candidate.mkdir(parents=True,exist_ok=False)
    root=candidate/'DynaBrothers';root.mkdir();payload=root/'data';payload.mkdir()
    build=ROOT/'build/dev'
    for target in EXECUTABLES:
        source=build/target
        if source.read_bytes()[:2]!=b'MZ':raise RuntimeError('Invalid executable: '+target)
        shutil.copy2(source,payload/target)
    shutil.copytree(ROOT/'debug',payload/'debug')
    for target in DIAGNOSTICS:shutil.copy2(build/target,payload/'debug'/target)
    for file in (build/'assets').rglob('*'):
        if file.is_file() and file.name!='boxart_sonic1.tga':
            target=payload/'assets'/file.relative_to(build/'assets');target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(file,target)
    media_runtime=stage_sdl3(payload,ROOT/'local/sdl-runtime')
    system=set();queue=[payload/x for x in EXECUTABLES]+[payload/'debug'/x for x in DIAGNOSTICS]
    queue.extend(payload/name for name in ('SDL2.dll','SDL3.dll'))
    seen=set()
    while queue:
        source=queue.pop()
        if source.name.lower() in seen:continue
        seen.add(source.name.lower())
        text=subprocess.check_output([str(BIN/'objdump.exe'),'-p',str(source)],text=True)
        for name in re.findall(r'DLL Name:\s*(\S+)',text):
            dependency=next((p for p in payload.glob('*.dll') if p.name.lower()==name.lower()),BIN/name)
            if dependency.is_file():
                target=payload/dependency.name
                if dependency!=target and not target.exists():shutil.copy2(dependency,target)
                queue.append(target)
            else:system.add(name)
    # Diagnostics live one directory below the games. Windows does not search
    # the parent directory for their DLLs; keep each test independently runnable.
    for dependency in payload.glob('*.dll'):
        shutil.copy2(dependency,payload/'debug'/dependency.name)
    embedded={}
    portable_env=portable_windows_env(env)
    for target in ['DynaBrothers1AOT.exe','DynaBrothers2AOT.exe']:
        text=subprocess.check_output([str(payload/target),'--build-info'],env=portable_env,text=True).strip()
        validate_build_info(text,revision,version);embedded[target]=text
    licenses=payload/'licenses'
    sources={
        'GenesisRecomp.txt':ROOT/'upstream/segagenesisrecomp/LICENSE.md',
        'GenesisRecomp-third-party.md':ROOT/'upstream/segagenesisrecomp/THIRD-PARTY-LICENSES.md',
        'MC68000-core.txt':ROOT/'upstream/segagenesisrecomp/external/m68k-recomp-core/LICENSE',
        'rbengine.txt':ROOT/'upstream/segagenesisrecomp/external/rbengine/LICENSE',
        'recomp-ui.txt':ROOT/'upstream/recomp-ui/LICENSE',
        'stb_image-source-and-license.h':ROOT/'upstream/recomp-ui/src/third_party/stb_image.h',
        'Fonts.md':ROOT/'upstream/recomp-ui/assets/common/fonts/NOTICE.md',
        'Controller-database.txt':ROOT/'upstream/controller-db/LICENSE',
        'RecompAchievements.txt':ROOT/'third_party/recomp-achievements/LICENSE',
        'RecompAchievements-cJSON.txt':ROOT/'third_party/recomp-achievements/sdk/third_party/cjson/LICENSE',
        'RecompAchievements-rcheevos.txt':ROOT/'third_party/recomp-achievements/sdk/third_party/rcheevos/LICENSE',
        'GCC-runtime-exception.txt':BIN.parent/'share/licenses/gcc/COPYING.RUNTIME',
        'GPL-3.0.txt':BIN.parent/'share/licenses/gcc/COPYING3',
        'winpthreads.txt':BIN.parent/'share/licenses/winpthreads/COPYING',
        'MinGW-runtime.txt':BIN.parent/'share/licenses/crt/COPYING.MinGW-w64-runtime.txt'
    }
    for name,source in sources.items():shutil.copy2(source,licenses/name)
    shutil.copy2(ROOT/'THIRD_PARTY_NOTICES.md',licenses/'DynaBrothers-notices.md')
    shutil.copy2(readme,payload/'READ-ME.md')
    stub=build/'DynaBrothers.exe'
    if stub.read_bytes()[:2]!=b'MZ':raise RuntimeError('Invalid executable: DynaBrothers.exe')
    shutil.copy2(stub,root/'DynaBrothers.exe')
    shutil.copy2(ROOT/'recomplauncher.json',root/'recomplauncher.json')
    (root/'roms').mkdir()
    (root/'roms'/'PUT-YOUR-ROMS-HERE.txt').write_text(
        'Place your own Dyna Brothers / Dyna Brothers 2 Special dumps (.md/.bin) in this folder,\n'
        'then start DynaBrothers.exe. ROM images are not included.\n',encoding='ascii')
    manifest=dict(version=version,channel='checkpoint',status='candidate',source_commit=revision,
        engine_commit=git('-C',str(ROOT/'upstream/segagenesisrecomp'),'rev-parse','HEAD'),
        engine_patch_sha256=digest(ROOT/'patches/0001-explicit-pc-mainloop.patch'),
        ui_commit=git('-C',str(ROOT/'upstream/recomp-ui'),'rev-parse','HEAD'),
        ui_patch_sha256=digest(ROOT/'patches/0002-dyna-launcher.patch'),
        controller_database_commit=git('-C',str(ROOT/'upstream/controller-db'),'rev-parse','HEAD'),
        embedded_build_info=embedded,media_runtime=media_runtime,
        target='Windows x64',compiler=subprocess.check_output([str(BIN/'gcc.exe'),'--version'],text=True).splitlines()[0],
        system_libraries=sorted(system),files=inventory(root),proprietary_input_images_included=False,
        single_entry_point=True,primary_executable='DynaBrothers.exe',launcher_executable='data/DynaBrothersLauncher.exe',
        internal_executables=['data/DynaBrothers1AOT.exe','data/DynaBrothers2AOT.exe'])
    (payload/'PACKAGE.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps(dict(candidate=str(candidate),source_commit=revision,files=len(manifest['files'])),indent=2))


def close(candidate,qa_path,releases,revision=1):
    if type(revision) is not int or not 1 <= revision <= 999:
        raise ValueError('Checkpoint revision must be an integer from 1 to 999')
    if (candidate/'CLOSED.json').exists():raise RuntimeError('Checkpoint is already closed; preserve delivered bytes')
    payload=candidate/'DynaBrothers';manifest_path=content(payload)/'PACKAGE.json'
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    version=package_version(manifest['version'])
    if inventory(payload)!=manifest['files']:raise RuntimeError('Staged package changed after its identity was recorded')
    qa=json.loads(qa_path.read_text(encoding='utf-8'))
    if not qa.get('passed'):raise RuntimeError('Packaged native regressions have not passed')
    for name,sha in qa['binaries'].items():
        if digest(content(payload)/name)!=sha:raise RuntimeError('QA belongs to another executable')
    required={'db1-en','db1-ja','db2-en','db2-ja','replay-record','replay-play','hint'}
    if revision >= 2 or tuple(map(int,version.split('.'))) >= (0,1,2):required.update({'db1-practice','db1-alien','db1-credits'})
    if tuple(map(int,version.split('.'))) >= (0,1,2):required.add('db2-unpause')
    if tuple(map(int,version.split('.'))) >= (0,1,3):required.update({
        'db1-wide-original','db1-wide-expanded','db1-wide-state-equality',
        'db1-achievements-ui','db2-achievements-ui','achievement-assets',
        'DynaDb1VideoTest','DynaDb1visibleTest','portable-debug-script'})
    if tuple(map(int,version.split('.'))) >= (0,1,4):required.update({'db1-audio-signal','db1-lightning','host-settings-same-window','launcher-integration-info'})
    if tuple(map(int,version.split('.'))) >= (0,1,5):required.update({'db1-stage-intro-1440','db1-camera-1440','db1-options-1440','db1-password-1440','db1-submenus-1440','db1-tsunami-1440','green-hub-same-window'})
    if tuple(map(int,version.split('.'))) >= (0,1,6):required.update({'green-hub-home-buttons','db1-intro-fade','db1-save-result','db1-hard-ending-save','db1-visor--2','db1-visor-0','db1-visor-2','db1-save-unicode'})
    if tuple(map(int,version.split('.'))) >= (0,1,7):required.update({'db1-title-extras','db1-title-level','db1-emi-save'})
    if tuple(map(int,version.split('.'))) >= (0,1,8):required.update({'db2-player-6button-original','db2-player-6button-wide','db2-player-6button-state-equality'})
    if tuple(map(int,version.split('.'))) >= (0,5,0):required.update({'sdl3-runtime','recomp-achievements-lists'})
    if tuple(map(int,version.split('.'))) >= (0,5,5):required.add('db1-title-level-normal1')
    if not required.issubset(qa['cases']) or not all(x['passed'] for x in qa['cases'].values()):
        raise RuntimeError('Missing packaged regression cases')
    destination=releases/f'DynaBrothers-v{version}-win64-checkpoint-r{revision}.zip'
    portable=releases/destination.stem
    if destination.exists() or portable.exists():
        raise FileExistsError('Preserve existing delivery; choose a new checkpoint revision')
    manifest['status']='ready';manifest['checkpoint_revision']=revision;manifest['verification']=qa
    manifest_path.write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    releases.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(destination,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
        for file in sorted(payload.rglob('*')):
            if file.is_file():archive.write(file,'DynaBrothers/'+file.relative_to(payload).as_posix())
    shutil.copytree(payload,portable)
    if inventory(portable)!=inventory(payload) or digest(content(portable)/'PACKAGE.json')!=digest(manifest_path):
        raise RuntimeError('Portable folder differs from the closed ZIP inputs')
    report=dict(status='closed',archive=str(destination.resolve()),portable_folder=str(portable.resolve()),sha256=digest(destination),size=destination.stat().st_size,
        source_commit=manifest['source_commit'])
    (candidate/'CLOSED.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    destination.with_suffix('.sha256').write_text(report['sha256']+'  '+destination.name+'\n',encoding='ascii')
    print(json.dumps(report,indent=2))


def main():
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    prepare=commands.add_parser('stage');prepare.add_argument('candidate',type=Path)
    finish=commands.add_parser('close');finish.add_argument('candidate',type=Path);finish.add_argument('--qa',type=Path,required=True);finish.add_argument('--releases',type=Path,default=ROOT/'releases');finish.add_argument('--revision',type=int,default=1)
    args=parser.parse_args()
    if args.command=='stage':stage(args.candidate.resolve())
    else:close(args.candidate.resolve(),args.qa.resolve(),args.releases.resolve(),args.revision)


if __name__=='__main__':main()
