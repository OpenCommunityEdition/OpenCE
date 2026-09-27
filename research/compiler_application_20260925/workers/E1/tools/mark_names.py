"""E1: list the C2 MARK order (function compile order) of a TU with each function's decorated name.

    python -B mark_names.py <label> <src.c> <unit>

bps [MARK] ungated; chain c0 = EAX -> +0 -> +4 (the decorated name string, per the fable5 S3-3 attribution recipe:
"the function name comes from EAX at MARK 0x10720825 ... chain `0 32 0 4` prints the decorated name")."""
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / 'W7' / 'tools'))
import w7trace as W  # noqa: E402

W.RUNS = W.ROOT / 'scratch/campaign/workers/E1/runs'


def cstr(words):
    b = b''.join(struct.pack('<I', x) for x in (words or [])).split(b'\0')[0]
    return b.decode('latin-1', 'replace')


def main():
    label, src, unit = sys.argv[1:4]
    events, meta = W.run(label, src, unit, [W.MARK], chains='0 32 0 4', note='E1 MARK order')
    names = [cstr(e['chains'].get(0)) for e in events if e['kind'] == 'LT' and e['bp'] == 0]
    adir = W.RUNS.parent / 'analysis'
    adir.mkdir(exist_ok=True)
    (adir / (label + '.marks.json')).write_text(json.dumps(names, indent=1) + '\n')
    for i, n in enumerate(names, 1):
        print(i, n)


if __name__ == '__main__':
    try:
        main()
    except W.TraceError as e:
        print('ABORT (fail-closed):', e)
        sys.exit(2)
