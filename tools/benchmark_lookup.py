"""Compare guarded native lookup policies using one binary and scene."""
import argparse
import json
from pathlib import Path
import statistics
import shutil
import subprocess
import sys


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path)
    parser.add_argument('rom',type=Path)
    parser.add_argument('script',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--frames',type=int,default=10010)
    parser.add_argument('--cpu-core',type=int)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    shutil.copy2(args.binary,args.output/'measured-binary.exe')
    rows=[]
    for number,policy in enumerate(['binary','cached']*3):
        output=args.output/f'{number+1}-{policy}'
        command=[sys.executable,str(Path(__file__).with_name('native_probe.py')),str(args.binary),str(args.rom),
            '--script',str(args.script),'--output',str(output),'--frames',str(args.frames),'--lookup',policy]
        if args.cpu_core is not None:command += ['--cpu-core',str(args.cpu_core)]
        subprocess.run(command,
            check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        row=json.loads((output/'probe.json').read_text(encoding='utf-8'));row['policy']=policy;rows.append(row)
        print(f'{number+1} {policy}: cpu_seconds={row["measurement"]["cpu_seconds"]} fps={row["measurement"]["fps"]}',flush=True)
    for key in ['binary_sha256','rom_sha256']:
        if len({row[key] for row in rows})!=1:raise RuntimeError('Input identity changed')
    for key in ['state_fnv1a64','audio_state_fnv1a64']:
        if len({row['measurement'][key] for row in rows})!=1:raise RuntimeError('Native result differs across lookup policies')
    medians={policy:statistics.median(row['measurement']['cpu_seconds'] for row in rows if row['policy']==policy) for policy in ['binary','cached']}
    report=dict(scope='Uncapped native CPU workload; not physical audio, display cadence or campaign equivalence',
        repetitions=3,frames=args.frames,cpu_seconds_median=medians,
        cpu_time_reduction_percent=100*(1-medians['cached']/medians['binary']),rows=rows)
    (args.output/'comparison.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({key:value for key,value in report.items() if key!='rows'},indent=2))


if __name__=='__main__':main()
