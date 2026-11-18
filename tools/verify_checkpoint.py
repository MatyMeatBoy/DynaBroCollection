"""Exercise packaged native binaries with development DLL paths excluded."""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from tools.build import find_by_hash, DB1_SHA, DB2_SHA
from tools.windows_environment import portable_windows_env


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate',type=Path);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--inputs',type=Path,default=Path('D:/01Claude/Dynabrothers'))
    args=parser.parse_args();payload=args.candidate.resolve()/'DynaBrothers'
    args.output.mkdir(parents=True,exist_ok=False)
    db1=find_by_hash(args.inputs,DB1_SHA,{'.md','.bin'});db2=find_by_hash(args.inputs,DB2_SHA,{'.md','.bin'})
    english={}
    for game in ['db1','db2']:
        metadata=json.loads((ROOT/f'generated/language/{game}_language.json').read_text(encoding='utf-8'))
        english[game]=find_by_hash(ROOT/'local/roms',metadata['images'][1]['sha256'],{'.md','.bin'})
    cases={}
    def run(name,target,rom,frames,script=None,language=None,replay=None,hint=False,irq=False,widescreen=None,ui=False):
        output=args.output/name
        command=[sys.executable,str(ROOT/'tools/native_probe.py'),str(payload/target),str(rom),
                 '--output',str(output),'--frames',str(frames),'--portable-env']
        if script:command+=['--script',str(script)]
        if language:command+=['--language',language]
        if replay:command+=['--replay',str(replay)]
        if widescreen:command+=['--widescreen',widescreen]
        if ui:command+=['--achievement-ui-test']
        env=dict(os.environ);env.pop('DYNA_IRQ_SELFTEST',None);env.pop('DYNA_IRQ_VERIFY',None)
        if hint:env.update(DYNA_IRQ_SELFTEST='1',DYNA_IRQ_VERIFY='1')
        elif irq:env['DYNA_IRQ_VERIFY']='1'
        result=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        if result.returncode:
            print(result.stdout.decode('utf-8',errors='replace'));raise RuntimeError('Packaged regression failed: '+name)
        report=json.loads((output/'probe.json').read_text(encoding='utf-8'))
        expected='Japanese original' if language=='ja' else 'English'
        if '[language] '+expected not in (output/'run.log').read_text(encoding='utf-8'):
            raise RuntimeError('Language selection was not observed: '+name)
        cases[name]=dict(passed=report['passed'],frames=report['measurement']['frames'],
            binary_sha256=report['binary_sha256'],rom_sha256=report['rom_sha256'],
            state_fnv1a64=report['measurement']['state_fnv1a64'],audio_state_fnv1a64=report['measurement']['audio_state_fnv1a64'],
            screenshots=report['screenshots'],replay_executions=report.get('replay_executions'))
        print(name+': pass',flush=True)
    run('db1-en','DynaBrothers1AOT.exe',db1,8900,ROOT/'games/db1/scenes/map-labels.input')
    run('db1-ja','DynaBrothers1AOT.exe',english['db1'],8900,ROOT/'games/db1/scenes/map-labels.input','ja')
    script=ROOT/'games/db2special/scenes/tutorial.input'
    run('db2-en','DynaBrothers2AOT.exe',db2,10010,script)
    run('db2-ja','DynaBrothers2AOT.exe',english['db2'],10010,script,'ja')
    run('db2-unpause','DynaBrothers2AOT.exe',db2,18000,ROOT/'games/db2special/scenes/unpause-gameplay.input',irq=True)
    run('replay-record','DynaBrothers2AOT.exe',db2,300)
    run('replay-play','DynaBrothers2AOT.exe',db2,301,replay=args.output/'replay-record/profile/logs/DynaBrothers2AOT-feedback.replay')
    run('hint','DynaBrothers1AOT.exe',db1,60,hint=True)
    run('db1-practice','DynaBrothers1AOT.exe',db1,9600,ROOT/'games/db1/scenes/practice-completion-diagnostic.input')
    run('db1-alien','DynaBrothers1AOT.exe',db1,9600,ROOT/'games/db1/scenes/alien-completion-diagnostic.input')
    run('db1-credits','DynaBrothers1AOT.exe',db1,52000,ROOT/'games/db1/scenes/credits-diagnostic.input')
    version=json.loads((payload/'PACKAGE.json').read_text(encoding='utf-8'))['version']
    if tuple(map(int,version.split('.'))) >= (0,1,3):
        run('db1-wide-original','DynaBrothers1AOT.exe',db1,6100,payload/'debug/db1-16x9.input',widescreen='off')
        run('db1-wide-expanded','DynaBrothers1AOT.exe',db1,6100,payload/'debug/db1-16x9.input',widescreen='16:9')
        original,expanded=cases['db1-wide-original'],cases['db1-wide-expanded']
        if any(original[key]!=expanded[key] for key in ['state_fnv1a64','audio_state_fnv1a64']):
            raise RuntimeError('DB1 expanded presentation changed canonical game/audio state')
        cases['db1-wide-state-equality']=dict(passed=True,frames=6100,state_fnv1a64=original['state_fnv1a64'],audio_state_fnv1a64=original['audio_state_fnv1a64'])
        run('db1-achievements-ui','DynaBrothers1AOT.exe',db1,120,ui=True)
        run('db2-achievements-ui','DynaBrothers2AOT.exe',db2,120,ui=True)
        for name in ['DynaDb1VideoTest','DynaDb1visibleTest']:
            subprocess.run([str(payload/'debug'/(name+'.exe'))],env=portable_windows_env(dict(os.environ)),check=True)
            cases[name]=dict(passed=True)
        for game,count,points in [('db1',76,470),('db2',99,1025)]:
            catalog=json.loads((payload/f'assets/achievements/{game}.json').read_text(encoding='utf-8'))['achievements']
            if len(catalog)!=count or sum(x['points'] for x in catalog)!=points:
                raise RuntimeError('Incorrect packaged achievement catalog: '+game)
            for entry in catalog:
                for key in ['badge_file_locked','badge_file_unlocked']:
                    path=payload/'assets/achievements'/entry[key]
                    data=path.read_bytes()
                    if not data.startswith(b'\x89PNG\r\n\x1a\n') or int.from_bytes(data[16:20],'big')!=64 or int.from_bytes(data[20:24],'big')!=64:
                        raise RuntimeError('Invalid packaged badge: '+str(path))
        cases['achievement-assets']=dict(passed=True,db1_goals=76,db2_goals=99,badges=350)
        # Exercise the user's exact no-Python entry point from a path with
        # spaces, without altering the staged package's immutable inventory.
        portable_copy=args.output.resolve()/'portable debug test'
        shutil.copytree(payload,portable_copy)
        powershell=Path(os.environ['SystemRoot'])/'System32/WindowsPowerShell/v1.0/powershell.exe'
        process=subprocess.run([str(powershell),'-NoProfile','-ExecutionPolicy','Bypass','-File',
            str(portable_copy/'debug/Test-DB1-16x9.ps1'),'-RomPath',str(db1),'-CompareOnly'],
            env=portable_windows_env(dict(os.environ)),stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (args.output/'portable-debug-script.log').write_bytes(process.stdout)
        results=list((portable_copy/'debug').glob('results-*/RESULT.txt'))
        if process.returncode or len(results)!=1 or not results[0].read_text(encoding='utf-8-sig').startswith('PASS:'):
            raise RuntimeError('Portable PowerShell debug entry point failed; inspect portable-debug-script.log')
        cases['portable-debug-script']=dict(passed=True,windows_powershell=True,development_path_excluded=True)
    binaries={p.relative_to(payload).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in payload.rglob('*.exe')}
    report=dict(passed=all(x['passed'] for x in cases.values()),binaries=binaries,cases=cases,
        environment='Windows x64; PATH excludes development DLL directories; isolated settings and saves',
        scope='Finite native scene/replay/IRQ contracts; physical controllers/audio and complete campaigns remain unverified')
    (args.output/'verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases)),indent=2))


if __name__=='__main__':main()
