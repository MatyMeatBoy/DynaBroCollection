"""Exercise packaged native binaries with development DLL paths excluded."""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT))
from tools.build import find_by_hash, DB1_SHA, DB2_SHA, default_input_root
from tools.windows_environment import portable_windows_env


def verify_media_runtime(payload):
    # Load both libraries in a subprocess so the package stays movable.
    probe = """
import ctypes
from pathlib import Path
base = Path.cwd()
sdl3 = ctypes.WinDLL(str(base / 'SDL3.dll'))
sdl2 = ctypes.WinDLL(str(base / 'SDL2.dll'))
assert sdl3.SDL_GetVersion() // 1000000 == 3
assert sdl2.SDL_Init(0) == 0
sdl2.SDL_Quit()
"""
    subprocess.run([sys.executable, '-c', probe], cwd=payload,
                   env=portable_windows_env(dict(os.environ)), check=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate',type=Path);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--inputs',type=Path,default=default_input_root())
    parser.add_argument('--jobs',type=int,default=min(8,os.cpu_count() or 4),
        help='Parallel packaged-regression workers. Every case runs its own native.exe '
             'subprocess with a hidden/offscreen window (the same self-test env-var hooks '
             'used throughout this project), so concurrent cases do not fight over the real '
             'OS foreground window, mouse or keyboard -- there is no UI automation here to race.')
    args=parser.parse_args();payload=args.candidate.resolve()/'DynaBrothers'
    if (payload/'data').is_dir():payload=payload/'data'
    args.output.mkdir(parents=True,exist_ok=False)
    db1=find_by_hash(args.inputs,DB1_SHA,{'.md','.bin'});db2=find_by_hash(args.inputs,DB2_SHA,{'.md','.bin'})
    english={}
    for game in ['db1','db2']:
        metadata=json.loads((ROOT/f'generated/language/{game}_language.json').read_text(encoding='utf-8'))
        english[game]=find_by_hash(ROOT/'local/roms',metadata['images'][1]['sha256'],{'.md','.bin'})
    cases={}
    version=json.loads((payload/'PACKAGE.json').read_text(encoding='utf-8'))['version']
    if tuple(map(int,version.split('.'))) >= (0,5,0):
        verify_media_runtime(payload)
        cases['sdl3-runtime']=dict(passed=True)
        for game, count in [('db1',76),('db2',99)]:
            name=f'dynabrocollection--{game}.json'
            path=payload/'assets/recomp-achievements'/name
            if path.read_bytes()!=(ROOT/'assets/recomp-achievements'/name).read_bytes():
                raise RuntimeError(f'Incorrect packaged Recomp Achievements list: {game}')
            data=json.loads(path.read_bytes())
            if len(data['achievements'])!=count or data['source']!='events':
                raise RuntimeError(f'Incomplete packaged Recomp Achievements list: {game}')
        cases['recomp-achievements-lists']=dict(passed=True,db1_goals=76,db2_goals=99)
    pool=ThreadPoolExecutor(max_workers=args.jobs)

    def build(name,target,rom,frames,script=None,language=None,replay=None,hint=False,irq=False,widescreen=None,ui=False,audio=False,host_settings=False,window_size=None,visor=None,save_test=False,overlays=False,profile_name=None,diagnostic_env=None,markers=(),db1_progress_seed=None):
        output=args.output/name
        command=[sys.executable,str(ROOT/'tools/native_probe.py'),str(payload/target),str(rom),
                 '--output',str(output),'--frames',str(frames),'--portable-env']
        if script:command+=['--script',str(script)]
        if language:command+=['--language',language]
        if replay:command+=['--replay',str(replay)]
        if widescreen:command+=['--widescreen',widescreen]
        if ui:command+=['--achievement-ui-test']
        if audio:command+=['--require-audio-signal']
        if host_settings:command+=['--audio','--host-settings-test']
        if window_size:command+=['--window-size',window_size]
        if visor is not None:command+=['--visor-offset-tiles',str(visor)]
        if overlays:command+=['--host-overlays']
        if profile_name:command+=['--profile-name',profile_name]
        if db1_progress_seed:command+=['--db1-progress-seed',str(db1_progress_seed)]
        env=dict(os.environ);env.pop('DYNA_IRQ_SELFTEST',None);env.pop('DYNA_IRQ_VERIFY',None)
        env.pop('DYNA_DB1_SAVE_TEST',None)
        if diagnostic_env:env.update(diagnostic_env)
        if save_test:env['DYNA_DB1_SAVE_TEST']='save'
        if hint:env.update(DYNA_IRQ_SELFTEST='1',DYNA_IRQ_VERIFY='1')
        elif irq:env['DYNA_IRQ_VERIFY']='1'
        return command,env,output

    def execute(name,command,env,output,script,language,replay,save_test,markers):
        result=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        log_text=result.stdout.decode('utf-8',errors='replace')
        if result.returncode:
            return dict(ok=False,error='Packaged regression failed: '+name,log=log_text)
        report=json.loads((output/'probe.json').read_text(encoding='utf-8'))
        log=(output/'run.log').read_text(encoding='utf-8')
        for marker in markers:
            if marker not in log:return dict(ok=False,error='Missing native evidence: '+name+' / '+marker)
        if save_test and '[progress-save-test] PASS' not in log:
            return dict(ok=False,error='Native result save was not persisted: '+name)
        expected='Japanese original' if language=='ja' else 'English'
        if '[language] '+expected not in log:
            return dict(ok=False,error='Language selection was not observed: '+name)
        case=dict(passed=report['passed'],frames=report['measurement']['frames'],
            binary_sha256=report['binary_sha256'],rom_sha256=report['rom_sha256'],
            state_fnv1a64=report['measurement']['state_fnv1a64'],audio_state_fnv1a64=report['measurement']['audio_state_fnv1a64'],
            screenshots=report['screenshots'],replay_executions=report.get('replay_executions'),
            audio_analysis=report.get('audio_analysis'),scope=report.get('scope'))
        return dict(ok=True,case=case)

    def submit(name,target,rom,frames,**kw):
        script=kw.get('script');language=kw.get('language');replay=kw.get('replay')
        save_test=kw.get('save_test',False);markers=kw.get('markers',())
        command,env,output=build(name,target,rom,frames,**kw)
        return pool.submit(execute,name,command,env,output,script,language,replay,save_test,markers)

    def collect(name,future):
        """Block on one case's future and either store it in `cases` or raise.
        Call this wherever the old sequential code called run(...) directly;
        call it right after submit(...) for cases nothing else depends on, or
        later (after submitting independent siblings) to let them overlap."""
        result=future.result()
        if not result['ok']:
            if 'log' in result:print(result['log'])
            raise RuntimeError(result['error'])
        cases[name]=result['case']
        print(name+': pass',flush=True)

    def run(name,target,rom,frames,script=None,**kw):
        """Drop-in equivalent of the old synchronous run(): submit and block
        immediately. Used where a case has no independent sibling worth
        overlapping it with."""
        collect(name,submit(name,target,rom,frames,script=script,**kw))

    # Independent cases run as one big parallel batch; each is submitted as
    # soon as its inputs are known and collected only where its result is
    # actually needed (a comparison, a dependent replay, or simply "did this
    # pass"). This is the same coverage as the old sequential version, just
    # not waiting on each packaged native run before starting the next.
    futures={}
    futures['db1-en']=submit('db1-en','DynaBrothers1AOT.exe',db1,8900,script=ROOT/'games/db1/scenes/map-labels.input')
    futures['db1-ja']=submit('db1-ja','DynaBrothers1AOT.exe',english['db1'],8900,script=ROOT/'games/db1/scenes/map-labels.input',language='ja')
    db2_script=ROOT/'games/db2special/scenes/tutorial.input'
    futures['db2-en']=submit('db2-en','DynaBrothers2AOT.exe',db2,10010,script=db2_script)
    futures['db2-ja']=submit('db2-ja','DynaBrothers2AOT.exe',english['db2'],10010,script=db2_script,language='ja')
    futures['db2-unpause']=submit('db2-unpause','DynaBrothers2AOT.exe',db2,18000,script=ROOT/'games/db2special/scenes/unpause-gameplay.input',irq=True)
    futures['replay-record']=submit('replay-record','DynaBrothers2AOT.exe',db2,300)
    futures['hint']=submit('hint','DynaBrothers1AOT.exe',db1,60,hint=True)
    futures['db1-practice']=submit('db1-practice','DynaBrothers1AOT.exe',db1,9600,script=ROOT/'games/db1/scenes/practice-completion-diagnostic.input')
    futures['db1-alien']=submit('db1-alien','DynaBrothers1AOT.exe',db1,9600,script=ROOT/'games/db1/scenes/alien-completion-diagnostic.input')
    futures['db1-credits']=submit('db1-credits','DynaBrothers1AOT.exe',db1,52000,script=ROOT/'games/db1/scenes/credits-diagnostic.input')
    for name in ['db1-en','db1-ja','db2-en','db2-ja','db2-unpause','hint','db1-practice','db1-alien','db1-credits']:
        collect(name,futures[name])
    # replay-play needs replay-record's output file on disk first.
    collect('replay-record',futures['replay-record'])
    run('replay-play','DynaBrothers2AOT.exe',db2,301,replay=args.output/'replay-record/profile/logs/DynaBrothers2AOT-feedback.replay')

    if tuple(map(int,version.split('.'))) >= (0,1,3):
        wide_jobs={
            'db1-wide-original':submit('db1-wide-original','DynaBrothers1AOT.exe',db1,6100,script=payload/'debug/db1-16x9.input',widescreen='off'),
            'db1-wide-expanded':submit('db1-wide-expanded','DynaBrothers1AOT.exe',db1,6100,script=payload/'debug/db1-16x9.input',widescreen='16:9'),
        }
        ui_jobs={
            'db1-achievements-ui':submit('db1-achievements-ui','DynaBrothers1AOT.exe',db1,120,ui=True),
            'db2-achievements-ui':submit('db2-achievements-ui','DynaBrothers2AOT.exe',db2,120,ui=True),
        }
        for n,f in wide_jobs.items():collect(n,f)
        for n,f in ui_jobs.items():collect(n,f)
        original,expanded=cases['db1-wide-original'],cases['db1-wide-expanded']
        if any(original[key]!=expanded[key] for key in ['state_fnv1a64','audio_state_fnv1a64']):
            raise RuntimeError('DB1 expanded presentation changed canonical game/audio state')
        cases['db1-wide-state-equality']=dict(passed=True,frames=6100,state_fnv1a64=original['state_fnv1a64'],audio_state_fnv1a64=original['audio_state_fnv1a64'])
        native_exe_futures={name:pool.submit(subprocess.run,[str(payload/'debug'/(name+'.exe'))],
            env=portable_windows_env(dict(os.environ)),check=True)
            for name in ['DynaDb1VideoTest','DynaDb1visibleTest']}
        for name,future in native_exe_futures.items():
            future.result();cases[name]=dict(passed=True)
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
    if tuple(map(int,version.split('.'))) >= (0,1,4):
        v14_jobs={
            'db1-audio-signal':submit('db1-audio-signal','DynaBrothers1AOT.exe',db1,6100,script=payload/'debug/db1-16x9.input',audio=True),
            'db1-lightning':submit('db1-lightning','DynaBrothers1AOT.exe',db1,8000,script=ROOT/'games/db1/scenes/lightning-diagnostic.input',widescreen='16:9'),
            'host-settings-same-window':submit('host-settings-same-window','DynaBrothers1AOT.exe',db1,300,host_settings=True),
        }
        integration=subprocess.run([str(payload/'DynaBrothersLauncher.exe'),'--integration-info'],env=portable_windows_env(dict(os.environ)),stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        info=json.loads(integration.stdout)
        if integration.returncode or info.get('schema_version')!=1 or len(info.get('games',[]))!=2:raise RuntimeError('Launcher integration metadata failed')
        cases['launcher-integration-info']=dict(passed=True,schema_version=1)
        for n,f in v14_jobs.items():collect(n,f)
    if tuple(map(int,version.split('.'))) >= (0,1,5):
        v15_jobs={}
        for name,scene,frames in [
            ('db1-stage-intro-1440','camera-stage-intro',5600),
            ('db1-camera-1440','camera-vertical',6800),
            ('db1-options-1440','options-native',6400),
            ('db1-password-1440','password-native',6500),
            ('db1-submenus-1440','submenu-native',12000),
            ('db1-tsunami-1440','hazard-tsunami',8500)]:
            v15_jobs[name]=submit(name,'DynaBrothers1AOT.exe',db1,frames,script=ROOT/('games/db1/scenes/'+scene+'.input'),
                widescreen='16:9',audio=True,window_size='2560x1440')
        from tools.hub_probe import verify_hub
        result=verify_hub(payload/'DynaBrothersLauncher.exe',db1,args.output/'green-hub-same-window')
        if not result['passed']:raise RuntimeError('Green hub original-window regression failed')
        cases['green-hub-same-window']=result
        for n,f in v15_jobs.items():collect(n,f)
    if tuple(map(int,version.split('.'))) >= (0,1,6):
        from tools.hub_probe import verify_home
        result=verify_home(payload/'DynaBrothersLauncher.exe',args.output/'green-hub-home-buttons')
        if not result['passed']:raise RuntimeError('Actual home Settings button regression failed')
        cases['green-hub-home-buttons']=result
        v16_jobs={
            'db1-intro-fade':submit('db1-intro-fade','DynaBrothers1AOT.exe',db1,2500,
                script=ROOT/'games/db1/scenes/intro-side-fade.input',widescreen='16:9'),
            'db1-save-result':submit('db1-save-result','DynaBrothers1AOT.exe',db1,11000,
                script=ROOT/'games/db1/scenes/progress-save-diagnostic.input',widescreen='16:9',save_test=True,overlays=True),
            'db1-hard-ending-save':submit('db1-hard-ending-save','DynaBrothers1AOT.exe',db1,43000,
                script=ROOT/'games/db1/scenes/progress-hard-ending-diagnostic.input',widescreen='16:9',overlays=True),
        }
        visor_jobs={}
        for offset in [-2,0,2]:
            visor_jobs[offset]=submit('db1-visor-'+str(offset),'DynaBrothers1AOT.exe',db1,6800,
                script=ROOT/'games/db1/scenes/camera-vertical.input',widescreen='16:9',
                window_size='2560x1440',visor=offset)
        # Use production input, not the Save-test callback, under a real Unicode profile.
        unicode_script=args.output/'unicode-save.input'
        source=(ROOT/'games/db1/scenes/progress-save-diagnostic.input').read_text(encoding='utf-8')
        source='\n'.join(line for line in source.splitlines() if not line.startswith('SCREENSHOT'))
        unicode_script.write_text(source+'\nPRESS DOWN 12\nWAIT 30\nPRESS A 12\nWAIT 120\n',encoding='utf-8')
        unicode_name='db1-save-unicode'
        unicode_profile='profile-保存'
        unicode_future=submit(unicode_name,'DynaBrothers1AOT.exe',db1,12500,script=unicode_script,widescreen='16:9',profile_name=unicode_profile)
        for n,f in v16_jobs.items():collect(n,f)
        for offset,f in visor_jobs.items():collect('db1-visor-'+str(offset),f)
        baseline=cases['db1-visor-0']
        for offset in [-2,2]:
            if any(cases['db1-visor-'+str(offset)][key]!=baseline[key]
                   for key in ['state_fnv1a64','audio_state_fnv1a64']):
                raise RuntimeError('Visor placement changed canonical game/audio state')
        collect(unicode_name,unicode_future)
        saves=list((args.output/unicode_name/unicode_profile).glob('dyna1-progress-*.bin'))
        if len(saves)!=1:raise RuntimeError('Unicode profile native Save did not create its progress file')
        data=saves[0].read_bytes()
        if len(data)!=105 or data[:8]!=b'DYNAPRG1' or int.from_bytes(data[101:105],'big')!=0x1becc:
            raise RuntimeError('Unicode profile saved an unexpected native progress record')
        checked=bytearray(data);checked[84:88]=bytes(4);checksum=2166136261
        for value in checked:checksum=((checksum^value)*16777619)&0xffffffff
        if checksum!=int.from_bytes(data[84:88],'big'):
            raise RuntimeError('Unicode profile progress checksum failed')
        cases['db1-save-unicode']=cases.pop(unicode_name)
        cases['db1-save-unicode']['native_save_sha256']=hashlib.sha256(data).hexdigest()
    if tuple(map(int,version.split('.'))) >= (0,1,7):
        progress_seeds=list((args.output/'db1-save-result/profile').glob('dyna1-progress-*.bin'))
        if len(progress_seeds)!=1:raise RuntimeError('Missing verified Normal 2 save for title level selection')
        title_jobs={}
        title_meta={}
        for level in [False,True]:
            name='db1-title-level' if level else 'db1-title-extras'
            dest=args.output.resolve()/name
            script=args.output/(name+'.input')
            source='WAIT 1999\nPRESS START 12\nWAIT 2000\nPRESS START 12\nWAIT 500\nPRESS UP 12\nWAIT 60\nPRESS A 12\nWAIT 2500\n'
            if level:source+='ASSERT_RAM16 FF0034 0015\nASSERT_RAM16 FF002E 00FF\nASSERT_RAM16 FF0030 B3F0\n'
            source+='SCREENSHOT title-result.png\n'
            script.write_text(source,encoding='utf-8')
            env={'DYNA_DB1_TITLE_UI_TEST':str(dest/'extras.bmp'),'DYNA_DB1_TITLE_UI_MODE':'level' if level else 'back','DYNA_PROGRESS_TRACE':'1'}
            markers=['PASS 4 level modal presentations; CPU/RAM/VDP unchanged','PASS real SDL level confirm closed modal','status=5'] if level else ['PASS 6 modal presentations; CPU/RAM/VDP unchanged','PASS real SDL Back event closed modal']
            if level:env['DYNA_DB1_TITLE_UI_INDEX']='0'
            title_jobs[name]=submit(name,'DynaBrothers1AOT.exe',db1,8000,script=script,widescreen='16:9',overlays=True,
                diagnostic_env=env,markers=markers,db1_progress_seed=progress_seeds[0] if level else None)
            title_meta[name]=dest
        emi_future=submit('db1-emi-save','DynaBrothers1AOT.exe',db1,9500,
            script=ROOT/'games/db1/scenes/emi-natural-loss.input',widescreen='16:9',overlays=True,
            diagnostic_env={'DYNA_DB1_REPLAY_TEST_INDEX':'95','DYNA_PROGRESS_TRACE':'1'},markers=['status=5'])
        for name,f in title_jobs.items():
            collect(name,f)
            capture=title_meta[name]/'extras.bmp'
            if not capture.is_file() or capture.read_bytes()[:2]!=b'BM':raise RuntimeError('Missing actual Extras compositor capture')
        collect('db1-emi-save',emi_future)
        saves=list((args.output/'db1-emi-save/profile').glob('dyna1-progress-*.bin'))
        if len(saves)!=1:raise RuntimeError('EMI natural-loss Save is missing')
        data=saves[0].read_bytes()
        if len(data)!=105 or data[88:92]!=bytes.fromhex('00040010') or data[93:97]!=bytes.fromhex('1232226A') or int.from_bytes(data[101:105],'big')!=0x1becc:
            raise RuntimeError('EMI Save did not preserve original native padded code')
        cases['db1-emi-save']['native_save_sha256']=hashlib.sha256(data).hexdigest()
    if tuple(map(int,version.split('.'))) >= (0,1,8):
        player_jobs={
            'db2-player-6button-original':submit('db2-player-6button-original','DynaBrothers2AOT.exe',db2,9400,script=ROOT/'games/db2special/scenes/player-normal2.input',
                widescreen='off',window_size='2560x1440',diagnostic_env={'DYNA_PAD_TRACE':'1','DYNA_DB2_VIDEO_TRACE':'1'}),
            'db2-player-6button-wide':submit('db2-player-6button-wide','DynaBrothers2AOT.exe',db2,9400,script=ROOT/'games/db2special/scenes/player-normal2.input',
                widescreen='16:9',window_size='2560x1440',diagnostic_env={'DYNA_PAD_TRACE':'1','DYNA_DB2_VIDEO_TRACE':'1'}),
        }
        for name,f in player_jobs.items():
            collect(name,f)
            log=(args.output/name/'run.log').read_text(encoding='utf-8')
            if '[pad-burst] invalid' in log:raise RuntimeError('Invalid native controller identification: '+name)
            cases[name]['invalid_native_controller_ids']=0
        a,b=cases['db2-player-6button-original'],cases['db2-player-6button-wide']
        if any(a[k]!=b[k] for k in ['state_fnv1a64','audio_state_fnv1a64']):raise RuntimeError('DB2 player wide mode changed canonical game/audio state')
        cases['db2-player-6button-state-equality']=dict(passed=True,frames=9400,state_fnv1a64=a['state_fnv1a64'],audio_state_fnv1a64=a['audio_state_fnv1a64'],scope='Actual typed Normal2 password, six-button detector and B+direction camera postcondition; held-scroll may use native fallback')
    pool.shutdown(wait=True)
    binaries={p.relative_to(payload).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in payload.rglob('*.exe')}
    if 'sdl3-runtime' in cases:
        for name in ('SDL2.dll','SDL3.dll'):
            binaries[name]=hashlib.sha256((payload/name).read_bytes()).hexdigest()
    report=dict(passed=all(x['passed'] for x in cases.values()),binaries=binaries,cases=cases,
        environment='Windows x64; PATH excludes development DLL directories; isolated settings and saves',
        scope='Finite native scene/replay/IRQ contracts; physical controllers/audio and complete campaigns remain unverified')
    (args.output/'verification.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases)),indent=2))


if __name__=='__main__':main()
