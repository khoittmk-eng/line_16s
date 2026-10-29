"""Compare actual ARMCC controller commands, not physical performance.

Three candidates: B27 baseline, abrupt scheduled P, gradual scheduled P.
Use --baseline-source before release, otherwise the local B27 backup ZIP.
Outputs CSV traces under build/corner-comparison; never touches the MCU.
"""
import argparse
import csv
from pathlib import Path
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'build/test-deps'))
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP


def emulate(path):
    with path.open('rb') as stream:
        elf = ELFFile(stream)
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        cpu.mem_map(0x08000000, 0x10000)
        cpu.mem_map(0x20000000, 0x5000)
        for section in elf.iter_sections():
            if section['sh_flags'] & 2 and section['sh_type'] != 'SHT_NOBITS':
                cpu.mem_write(section['sh_addr'], section.data())
        symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
        done = symbols['test_done'] & ~1
        reached = []
        def complete(machine, address, size, data):
            reached.append(True)
            machine.emu_stop()
        cpu.hook_add(UC_HOOK_CODE, complete, begin=done, end=done)
        cpu.reg_write(UC_ARM_REG_SP, 0x20004FF0)
        cpu.emu_start(symbols['test_entry'] | 1, 0, timeout=60000000, count=300000000)
        if not reached:
            raise RuntimeError('Trace did not complete')
        count = int.from_bytes(cpu.mem_read(symbols['trace_count'], 4), 'little')
        assert count == 64
        raw = bytes(cpu.mem_read(symbols['trace'], count * 32))
        rows = list(struct.iter_unpack('<8i', raw))
        assert all(0 <= r[3] <= 850 and 0 <= r[4] <= 850 and r[6] == 1 for r in rows)
        return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-source', type=Path)
    args = parser.parse_args()
    work = ROOT / 'build/corner-comparison'
    work.mkdir(parents=True, exist_ok=True)
    baseline = work / 'reference-b27.c'
    if args.baseline_source:
        baseline.write_bytes(args.baseline_source.read_bytes())
    else:
        backup = ROOT / 'backups/B27-before-B28-20261029/B27-source.zip'
        with zipfile.ZipFile(backup) as archive:
            name = next(n for n in archive.namelist() if n.replace('\\', '/') == 'app/control.c')
            baseline.write_bytes(archive.read(name))
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name, source, defines in [
        ('baseline-b27', baseline, []),
        ('abrupt-gain', ROOT / 'app/control.c', ['-DCONTROL_CORNER_GAIN_RISE_PER_MS=1000.0f']),
        ('gradual-gain', ROOT / 'app/control.c', []),
    ]:
        control, entry, axf = [work / (name + ext) for ext in ('.o', '-entry.o', '.axf')]
        common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(ROOT / 'app')]
        subprocess.run([str(compiler / 'armcc.exe'), *common, *defines, '-c', str(source), '-o', str(control)], check=True)
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(ROOT / 'tests/test_gain_trace.c'), '-o', str(entry)], check=True)
        subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                        '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                        str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                        str(control), str(entry)], check=True)
        rows = traces[name] = emulate(axf)
        with (work / (name + '.csv')).open('w', newline='', encoding='utf-8') as output:
            writer = csv.writer(output)
            writer.writerow(['ms','error_x1000','correction_x1000','left_x10','right_x10','target_x1000','mode','extra_P_gain_x1000'])
            writer.writerows(rows)
    base = traces['baseline-b27']
    print('Synthetic P-only cue; numbers are requested PWM, not speed, yaw or lap time.')
    print('candidate        extra at cue+6ms    largest assist step    final correction')
    for name, rows in traces.items():
        extra = [(rows[i][2] - base[i][2])/1000 for i in range(16,32)]
        largest = max(abs(extra[i] - (extra[i-1] if i else 0)) for i in range(len(extra)))
        print(f'{name:16} {extra[2]:8.3f}%              {largest:8.3f}%           {rows[31][2]/1000:8.3f}%')
    assert all(traces['gradual-gain'][i][2] == base[i][2] for i in range(16))
    assert 0 < traces['gradual-gain'][18][2]-base[18][2] < traces['abrupt-gain'][18][2]-base[18][2]
    assert traces['gradual-gain'][31][2] > base[31][2]
    print('PASS: ordinary soft curve unchanged; gradual assist has a smaller entry step than abrupt gain.')
    print(f'Traces: {work}')


if __name__ == '__main__':
    main()
