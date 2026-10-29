"""Receive one line-follower CSV over CH340, validate it, and save a fresh file.

Close Hercules, run this script, then select Export Flash CSV on the OLED.
The serial port is receive-only; this program never sends a command to the car.
"""
import argparse
from datetime import datetime
import importlib.util
import json
import math
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'build' / 'serial-deps'))
START = b'# linefollower_blackbox_v1,'
MAX_BYTES = 128 * 1024


def load_reader():
    spec = importlib.util.spec_from_file_location('blackbox_reader', Path(__file__).with_name('read-blackbox.py'))
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    return reader


class CaptureError(Exception):
    pass


def receive(port, raw_file, *, wait_seconds=120, transfer_seconds=90, idle_seconds=5,
            clock=time.monotonic, progress=None):
    """Keep exact received bytes on disk, handling arbitrary USB chunk boundaries.

    Return only one complete header..END frame. The caller validates CRC/schema
    before publishing a .csv. Any failure leaves the .partial.log intact.
    """
    pending = bytearray()
    frame = bytearray()
    started_at = None
    opened_at = last_byte_at = last_progress = clock()
    total = 0
    while True:
        now = clock()
        if started_at is None and now - opened_at >= wait_seconds:
            raise CaptureError('Het thoi gian cho header. Bam Export SAU khi script bao READY; kiem tra COM/PB12/GND.')
        if started_at is not None:
            if now - started_at >= transfer_seconds:
                raise CaptureError('Qua thoi gian truyen; chua nhan du # END. Export lai vao file moi.')
            if now - last_byte_at >= idle_seconds:
                raise CaptureError('Du lieu bi ngat truoc # END. Kiem tra nguon/day CH340 va Export lai.')
        chunk = port.read(min(max(port.in_waiting, 1), 4096))
        if not chunk:
            continue
        raw_file.write(chunk)
        raw_file.flush()
        total += len(chunk)
        if total > MAX_BYTES:
            raise CaptureError('Vuot gioi han 128 KiB; khong phai mot ban ghi B22 hop le.')
        now = last_byte_at = clock()
        pending.extend(chunk)
        while b'\n' in pending:
            end = pending.index(b'\n') + 1
            line = bytes(pending[:end])
            del pending[:end]
            if line.startswith(START):
                if started_at is not None:
                    raise CaptureError('Nhan header thu hai truoc # END. Chi bam Export mot lan.')
                started_at = now
            if started_at is not None:
                frame.extend(line)
                if line.startswith(b'# END,'):
                    return bytes(frame)
        if progress and now - last_progress >= 1:
            progress(total, started_at is not None)
            last_progress = now


def save_validated(frame, base):
    """Use exclusive creation: never replace any previous run file."""
    reader = load_reader()
    log = reader.parse_log(frame.decode('ascii'))
    report = reader.summarize(log)
    csv_path = base.with_suffix('.csv')
    summary_path = base.with_suffix('.summary.json')
    # Reserve both names before publishing data. On collision, retain raw capture.
    csv_created = summary_created = False
    try:
        with csv_path.open('xb') as csv_file:
            csv_created = True
            with summary_path.open('x', encoding='utf-8') as summary_file:
                summary_created = True
                csv_file.write(frame)
                json.dump(report, summary_file, ensure_ascii=False, indent=2)
                summary_file.write('\n')
    except BaseException:
        # Close files before unlinking (Windows); never touch a pre-existing run.
        if csv_created:
            csv_path.unlink(missing_ok=True)
        if summary_created:
            summary_path.unlink(missing_ok=True)
        raise
    return csv_path, summary_path, report


def choose_port(devices, requested=None, ask=input, tell=print):
    if requested:
        return requested
    if devices:
        tell('Cac cong COM dang co:')
        for i, device in enumerate(devices, 1):
            tell(f'  {i}. {device.device} - {device.description}')
    else:
        tell('Chua tim thay COM. Kiem tra CH340/driver; co the nhap ten COM thu cong.')
    answer = ask('Nhap so thu tu hoac ten cong (vi du COM5): ').strip()
    if answer.isdecimal():
        index = int(answer) - 1
        if not 0 <= index < len(devices):
            raise CaptureError('So thu tu COM khong hop le.')
        return devices[index].device
    if not answer:
        raise CaptureError('Chua chon cong COM.')
    return answer


def open_port(serial, name):
    # Configure modem lines before opening; only PB12->RXD and GND are connected.
    port = serial.Serial(port=None, baudrate=9600, bytesize=serial.EIGHTBITS,
                         parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
                         timeout=0.2, xonxoff=False, rtscts=False, dsrdtr=False)
    try:
        port.dtr = False
        port.rts = False
        port.port = name
        port.open()
        port.reset_input_buffer()
        return port
    except Exception:
        port.close()
        raise


def main(argv=None):
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--port', help='COM5; omit to choose interactively')
    cli.add_argument('--out-dir', type=Path, default=ROOT / 'logs')
    cli.add_argument('--list', action='store_true', help='List serial ports and exit')
    cli.add_argument('--wait', type=float, default=120, help='Seconds to wait for Export header')
    args = cli.parse_args(argv)
    if not math.isfinite(args.wait) or args.wait <= 0:
        cli.error('--wait must be positive')
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print('Thieu pySerial. Chay scripts/receive-blackbox.ps1 de cai vao build/serial-deps,', file=sys.stderr)
        print('hoac: python -m pip install -r scripts/requirements-serial.txt', file=sys.stderr)
        return 1
    port = None
    raw_path = None
    try:
        devices = list(list_ports.comports())
        if args.list:
            for device in devices:
                print(f'{device.device}: {device.description}')
            if not devices:
                print('Khong tim thay cong serial.')
            return 0
        name = choose_port(devices, args.port)
        port = open_port(serial, name)
        args.out_dir.mkdir(parents=True, exist_ok=True)
        base = args.out_dir.resolve() / datetime.now().strftime('run-%Y%m%d-%H%M%S-%f')
        raw_path = base.with_suffix('.partial.log')
        with raw_path.open('xb') as raw_file:
            print(f'READY: {name}, 9600 / 8N1. Hercules phai dong.', flush=True)
            print('Tren OLED: System > BLACK BOX > Export Flash CSV > N2.', flush=True)
            print('Chi bam Export mot lan; Ctrl+C de huy. Khong can Arm/chay lai neu Flash da Saved.', flush=True)
            frame = receive(port, raw_file, wait_seconds=args.wait,
                            progress=lambda n, active: print(f'\rDang nhan: {n} byte' +
                                                            ('...' if active else ' (cho header)...'), end='', flush=True))
        port.close()
        port = None
        csv_path, summary_path, report = save_validated(frame, base)
        raw_path.unlink()  # Verified CSV now contains the complete frame.
        print(f'\nPASS: CRC32 {report["crc32"]}, {report["sample_count"]} mau, firmware {report["firmware"]}.')
        print(f'GUI FILE NAY: {csv_path}')
        print(f'Tom tat:      {summary_path}')
        return 0
    except KeyboardInterrupt:
        print('\nDa huy. Chua co file CSV duoc xac nhan.', file=sys.stderr)
        return 130
    except (CaptureError, serial.SerialException, OSError, ValueError, UnicodeError, IndexError) as error:
        print(f'\nFAIL: {error}', file=sys.stderr)
        print('Dong Hercules/Serial Monitor neu cong dang bi chiem. Co the Export lai ban Flash cu.', file=sys.stderr)
        return 1
    finally:
        if port is not None:
            port.close()
        if raw_path is not None and raw_path.exists():
            print(f'Du lieu CHUA xac nhan: {raw_path}', file=sys.stderr)


if __name__ == '__main__':
    sys.exit(main())
