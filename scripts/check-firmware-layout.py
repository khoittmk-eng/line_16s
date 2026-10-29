"""Reject a HEX that touches the reserved black-box slots or exceeds flash."""
import sys
from pathlib import Path


def check(path):
    upper = 0
    eof = False
    highest = 0
    for line in Path(path).read_text(encoding='ascii').splitlines():
        if not line:
            continue
        if eof or not line.startswith(':'):
            raise ValueError('Malformed Intel HEX')
        record = bytes.fromhex(line[1:])
        if len(record) < 5 or len(record) != record[0] + 5 or sum(record) & 255:
            raise ValueError('Invalid Intel HEX length/checksum')
        size, address, kind = record[0], int.from_bytes(record[1:3], 'big'), record[3]
        if kind == 0:
            begin = upper + address
            if not 0x08000000 <= begin <= begin + size <= 0x0800C000:
                raise ValueError(f'Firmware overlaps log storage or non-application memory at {begin:#x}')
            highest = max(highest, begin + size)
        elif kind == 4 and size == 2:
            upper = int.from_bytes(record[4:6], 'big') << 16
        elif kind == 2 and size == 2:
            upper = int.from_bytes(record[4:6], 'big') << 4
        elif kind == 1 and size == 0:
            eof = True
        elif kind not in (3, 5):
            raise ValueError('Unexpected Intel HEX record')
    if not eof or not highest:
        raise ValueError('Incomplete/empty firmware HEX')
    return highest


if __name__ == '__main__':
    try:
        high = check(sys.argv[1])
        print(f'Layout PASS: image ends at {high:#010x}; log Flash 0x0800C000..0x0800FFFF untouched.')
    except (ValueError, OSError, IndexError) as error:
        raise SystemExit(f'Layout FAIL: {error}')
