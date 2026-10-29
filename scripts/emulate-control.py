"""Execute actual ARMCC-compiled pure C algorithms, not peripherals, under Unicorn.
Install once: python -m pip install --target build/test-deps unicorn==2.1.4 pyelftools==0.33
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'build/test-deps'))
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_PC
from elftools.elf.elffile import ELFFile
with open(sys.argv[1], 'rb') as source:
    elf = ELFFile(source)
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    cpu.mem_map(0x08000000, 0x10000)
    cpu.mem_map(0x20000000, 0x5000)
    # RAM-backed peripheral address range for motor driver register request tests.
    cpu.mem_map(0x40000000, 0x30000)
    # STM32 DBGMCU register request used by actual Board_Init (RAM model only).
    cpu.mem_map(0xE0042000, 0x1000)
    # Test-only Flash/serial sinks; these are not additional MCU RAM.
    cpu.mem_map(0x60000000, 0x20000)
    # ARM scatter loading may keep RW initialization bytes in an RO load segment.
    # Load sections at their execution addresses because we enter C without __main.
    for section in elf.iter_sections():
        if section['sh_flags'] & 2 and section['sh_type'] != 'SHT_NOBITS':
            cpu.mem_write(section['sh_addr'], section.data())
    symbols = {symbol.name: symbol['st_value'] for symbol in elf.get_section_by_name('.symtab').iter_symbols()}
    cpu.reg_write(UC_ARM_REG_SP, 0x20004FF0)
    done = symbols['test_done'] & ~1
    reached = [False]
    def hook(machine, address, size, data):
        if address == done:
            reached[0] = True
            machine.emu_stop()
    # Only the completion address needs a Python callback. A global instruction
    # hook spent the wall-clock budget in Python as regression scenarios grew.
    cpu.hook_add(UC_HOOK_CODE, hook, begin=done, end=done)
    def invalid_memory(machine, access, address, size, value, data):
        pc = machine.reg_read(UC_ARM_REG_PC)
        functions = [(symbol['st_value'] & ~1, symbol.name) for symbol in elf.get_section_by_name('.symtab').iter_symbols() if symbol['st_info']['type'] == 'STT_FUNC']
        nearest = max((item for item in functions if item[0] <= pc), default=(0, '?'))
        print(f'Invalid emulation access: address={address:#x}, PC={pc:#x}, nearest={nearest[1]}+{pc-nearest[0]:#x}')
        return False
    cpu.hook_add(UC_HOOK_MEM_INVALID, invalid_memory)
    # B20 adds entry braking/slew diagnostics and adversarial transition tests.
    # Keep the wall-clock limit; instruction budget is for the whole suite,
    # not a claim about the 2 ms hardware control deadline.
    cpu.emu_start(symbols['test_entry'] | 1, 0, timeout=60000000, count=300000000)
    read = lambda name: int.from_bytes(cpu.mem_read(symbols[name], 4), 'little')
    count, failure = read('test_count'), read('failure_line')
    if failure or not reached[0]:
        raise SystemExit(f'FAIL: assertions={count}, source failure line={failure}, completed={reached[0]}')
    if 'blackbox_export_bytes' in symbols:
        size = read('blackbox_export_bytes')
        assert 0 < size < 65535
        Path(sys.argv[1]).with_name('blackbox-export.csv').write_bytes(bytes(cpu.mem_read(0x60010000, size)))
    if 'b23_replay_min_x10' in symbols:
        print('B22 seq2 raw replay on current controller (commanded PWM, not dynamics): '
              f'min={read("b23_replay_min_x10")/10:.1f}%, '
              f'rejoin={read("b23_replay_rejoin_x10")/10:.1f}%, '
              f'end={read("b23_replay_end_x10")/10:.1f}%.')
    print(f'PASS: {count} assertions executed from ARMCC-compiled C on Cortex-M3 emulation.')
    print('Coverage: controller, gaps/recovery, menu/interlocks/watchdogs, black-box ring/Flash/CSV, motor and log drivers with fake peripherals/clocks. No physical dynamics, Flash endurance or baud timing validation.')
