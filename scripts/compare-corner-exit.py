"""Compare B28, faster-rise-only, and B29 command traces; no physical plant.

Default reference is the local B28 backup ZIP. Before release, pass
--baseline-source D:/stm32-linefollower-spl/app/control.c (while still B28).
"""
import argparse
import csv
import importlib.util
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('corner_compare', ROOT / 'scripts/compare-corner.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-source', type=Path)
    args = parser.parse_args()
    work = ROOT / 'build/corner-exit-comparison'
    reference = work / 'reference-b28'
    reference.mkdir(parents=True, exist_ok=True)
    files = ['control.c','control.h','control_tuning.h']
    if args.baseline_source:
        for name in files:
            (reference / name).write_bytes((args.baseline_source.parent / name).read_bytes())
    else:
        with zipfile.ZipFile(ROOT / 'backups/B28-before-B29-20261029/B28-source.zip') as archive:
            for name in files:
                key = next(n for n in archive.namelist() if n.replace('\\','/') == 'app/' + name)
                (reference / name).write_bytes(archive.read(key))
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name, source, defines in [
        ('baseline-b28', reference / 'control.c', []),
        ('faster-rise-only', ROOT / 'app/control.c', ['-DCONTROL_CORNER_RETURN_CONFIRM_MS=0']),
        ('b29-return', ROOT / 'app/control.c', []),
    ]:
        control, entry, axf = [work / (name + ext) for ext in ('.o','-entry.o','.axf')]
        common = ['--cpu','Cortex-M3','--c99','-O1','-g','-I',str(source.parent)]
        subprocess.run([str(compiler / 'armcc.exe'), *common, *defines, '-c',str(source),'-o',str(control)], check=True)
        subprocess.run([str(compiler / 'armcc.exe'), *common,'-c',str(ROOT / 'tests/test_exit_trace.c'),'-o',str(entry)], check=True)
        subprocess.run([str(compiler / 'armlink.exe'),'--cpu','Cortex-M3','--entry','test_entry',
                        '--ro-base','0x08000000','--rw-base','0x20000000','--libpath',str(compiler.parent / 'lib'),
                        '--no_remove','--output',str(axf),str(control),str(entry)], check=True)
        rows = traces[name] = helper.emulate(axf)
        with (work / (name + '.csv')).open('w', newline='', encoding='utf-8') as stream:
            writer = csv.writer(stream)
            writer.writerow(['ms','error_x1000','correction_x1000','left_x10','right_x10','target_x1000','mode','extra_P_gain_x1000'])
            writer.writerows(rows)
    print('Synthetic P-only input: PWM requests, not measured speed/yaw/lap time.')
    print('candidate          entry correction   steady correction   extra gain at return+6ms')
    for name, rows in traces.items():
        print(f'{name:18} {rows[18][2]/1000:8.3f}%          {rows[31][2]/1000:8.3f}%           {rows[34][7]/1000:8.3f}')
    base, rise, both = (traces[n] for n in ['baseline-b28','faster-rise-only','b29-return'])
    assert all(base[i][2] == both[i][2] for i in range(16))
    assert both[18][2] > base[18][2] and both[31][2] == base[31][2]
    assert both[34][7] < rise[34][7] and both[34][7] < base[34][7]
    assert all(row[5] == 45000 for row in both)
    print('PASS: faster buildup, same steady maximum, earlier return fade, Base45 throughout.')
    print(f'Traces: {work}')


if __name__ == '__main__':
    main()
