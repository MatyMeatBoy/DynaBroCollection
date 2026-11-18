"""Exercise packaged native binaries with development DLL paths excluded."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from tools.build import find_by_hash, DB1_SHA, DB2_SHA


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
    def run(name,target,rom,frames,script=None,language=None,replay=None,hint=False):
        output=args.output/name
        command=[sys.executable,str(ROOT/'tools/native_probe.py'),str(payload/target),str(rom),
                 '--output',str(output),'--frames',str(frames),'--portable-env']
        if script:command+=['--script',str(script)]
        if language:command+=['--language',language]
        if replay:command+=['--replay',str(replay)]
        env=dict(os.environ);env.pop('DYNA_IRQ_SELFTEST',None);env.pop('DYNA_IRQ_VERIFY',None)
        if hint:env.update(DYNA_IRQ_SELFTEST='1',DYNA_IRQ_VERIFY='1')
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
    run('replay-record','DynaBrothers2AOT.exe',db2,300)
    run('replay-play','DynaBrothers2AOT.exe',db2,301,replay=args.output/'replay-record/profile/logs/DynaBrothers2AOT-feedback.replay')
    run('hint','DynaBrothers1AOT.exe',db1,60,hint=True)
    run('db1-practice','DynaBrothers1AOT.exe',db1,9600,ROOT/'games/db1/scenes/practice-completion-diagnostic.input')
    run('db1-alien','DynaBrothers1AOT.exe',db1,9600,ROOT/'games/db1/scenes/alien-completion-diagnostic.input')
    run('db1-credits','DynaBrothers1AOT.exe',db1,52000,ROOT/'games/db1/scenes/credits-diagnostic.input')
    binaries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in payload.glob('*.exe')}
    report=dict(passed=all(x['passed'] for x in cases.values()),binaries=binaries,cases=cases,
        environment='Windows x64; PATH excludes development DLL directories; isolated settings and saves',
        scope='Finite native scene/replay/IRQ contracts; physical controllers/audio and complete campaigns remain unverified')
    (args.output/'verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases)),indent=2))


if __name__=='__main__':main()
