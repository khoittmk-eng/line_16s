"""Validate an exported line-follower log and summarize commands (not measured motion).

Usage: python scripts/read-blackbox.py path/to/run-01.csv
Accepts Hercules surrounding text and LF/CRLF, but requires one complete log.
"""
import argparse
import csv
import io
import json
from pathlib import Path
import zlib

MODES = ['STOPPED', 'TRACK', 'GAP', 'SEARCH', 'FAULT', 'TURN', 'PROBE',
         'UNUSED', 'RECENTER', 'BRAKE', 'ENTRY_RESERVED']
REASONS = ['OFF', 'BASE', 'BOOST', 'CURVE', 'WAIT', 'GAP', 'RECOVERY', 'BRAKE', 'PROBE', 'CORNER']
COLUMNS = ('ms,raw0,raw1,raw2,raw3,raw4,raw5,raw6,raw7,error,left,right,'
           'common_x10,target_x10,correction_x10,derivative_x10,planner_x10,'
           'lost_ms,recovery_ms,mask,valid,mode,reason,fault,stop_reason,side,flags').split(',')


def parse_log(text):
    lines = text.lstrip('\ufeff').splitlines()
    starts = [i for i, line in enumerate(lines) if line.startswith('# linefollower_blackbox_v1,')]
    if len(starts) != 1:
        raise ValueError('Need exactly one log beginning with # linefollower_blackbox_v1.')
    start = starts[0]
    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith('# END,')), None)
    if end is None:
        raise ValueError('Incomplete transfer: missing # END. Export again before sending this file.')
    trailer = lines[end].split(',')
    if len(trailer) != 5 or trailer[:2] != ['# END', 'count'] or trailer[3] != 'crc32':
        raise ValueError('Invalid END marker.')
    expected_count, expected_crc = int(trailer[2]), int(trailer[4])
    body = lines[start:end]
    actual_crc = zlib.crc32(('\r\n'.join(body) + '\r\n').encode('ascii'))
    if actual_crc != expected_crc:
        raise ValueError('CSV CRC32 mismatch: bytes changed or were lost during transfer/copy.')
    meta = body[0].split(',')
    if len(meta) != 7 or meta[1::2] != ['firmware', 'sequence', 'count']:
        raise ValueError('Invalid metadata.')
    if not 1 <= expected_count <= 160 or int(meta[6]) != expected_count:
        raise ValueError('Sample count outside buffer capacity or mismatched metadata.')
    csv_lines = [line for line in body if not line.startswith('#')]
    reader = csv.DictReader(io.StringIO('\n'.join(csv_lines)))
    if reader.fieldnames != COLUMNS:
        raise ValueError('Unexpected schema; use the parser for this firmware version.')
    rows = []
    for record in reader:
        if None in record or any(value is None for value in record.values()):
            raise ValueError('Malformed sample row.')
        row = {name: int(value) for name, value in record.items()}
        if not (0 <= row['ms'] <= 0xFFFFFFFF and 0 <= row['mode'] < len(MODES)
                and 0 <= row['reason'] < len(REASONS) and row['valid'] in (0, 1)
                and 0 <= row['mask'] <= 255 and -1000 <= row['left'] <= 1000
                and -1000 <= row['right'] <= 1000
                and all(0 <= row[f'raw{i}'] <= 4095 for i in range(8))):
            raise ValueError('Sample values outside expected ranges.')
        if rows and ((row['ms'] - rows[-1]['ms']) & 0xFFFFFFFF) > 10000:
            raise ValueError('Non-monotonic timestamps or a gap exceeding 10 seconds.')
        rows.append(row)
    if len(rows) != expected_count:
        raise ValueError('CSV row count does not match END.')
    profile_keys = body[1].split(',')[1:]
    profile_values = body[2].split(',')[1:]
    window_keys = body[3].split(',')[1:]
    window_values = body[4].split(',')[1:]
    if len(profile_keys) != len(profile_values) or len(window_keys) != len(window_values):
        raise ValueError('Invalid profile/window metadata.')
    return {
        'firmware': meta[2], 'sequence': int(meta[4]), 'crc32': f'{actual_crc:08X}',
        'profile': dict(zip(profile_keys, map(int, profile_values))),
        'window': dict(zip(window_keys, map(int, window_values))), 'rows': rows,
    }


def summarize(log):
    rows = log['rows']
    durations = {}
    zero_ms = 0
    for a, b in zip(rows, rows[1:]):
        dt = (b['ms'] - a['ms']) & 0xFFFFFFFF
        mode = MODES[a['mode']]
        durations[mode] = durations.get(mode, 0) + dt
        if (a['left'] == 0) != (a['right'] == 0):
            zero_ms += dt
    transitions = []
    previous = None
    for row in rows:
        pair = (row['mode'], row['reason'])
        if pair != previous:
            transitions.append({
                'ms': row['ms'], 'mode': MODES[row['mode']], 'reason': REASONS[row['reason']],
                'error': row['error'], 'mask': f"{row['mask']:02X}",
                'left': row['left'], 'right': row['right'],
            })
        previous = pair
    return {
        **{k: v for k, v in log.items() if k != 'rows'},
        'sample_count': len(rows), 'window_ms': (rows[-1]['ms'] - rows[0]['ms']) & 0xFFFFFFFF,
        'mode_ms_approx': durations, 'one_wheel_zero_ms_approx': zero_ms,
        'max_abs_error': max(abs(r['error']) for r in rows), 'transitions': transitions,
        'note': 'Requested PWM only. Durations approximate this short window, not a full lap or measured braking.',
    }


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('csv_file', type=Path)
    args = cli.parse_args()
    try:
        report = summarize(parse_log(args.csv_file.read_text(encoding='utf-8-sig')))
    except (ValueError, UnicodeError, IndexError, OSError) as error:
        cli.exit(1, f'Invalid log: {error}\n')
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
