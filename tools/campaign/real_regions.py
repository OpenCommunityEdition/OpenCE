#!/usr/bin/env python3
"""How many of a residual's differing regions are REAL?

READ-ONLY DIAGNOSTIC. Reads build/split and a candidate object, writes nothing,
recompiles nothing, runs in no build step.

`alndiff.py` prints a region whenever its two normalized instruction texts are
not identical - and that includes a pure PRESENTATION difference in how the two
objects spell a relocation target:

    T  12d  fcomp dword ptr [0] ; target=symbol:__real@00000000:0
    O  12d  fcomp dword ptr [0] ; target=defined-noncode:.rdata:__real@00000000:0

Same address, same type, same target. `relocdiff --allow-structural` scores that
row `=`. It is not a divergence, and counting it as one makes a residual look
further away than it is.

That matters because the noise is concentrated on exactly the functions that are
CLOSEST, where the ratio is worst. Measured over the Lane A AI portfolio:

    _actor_perception_friend_prop_is_attacking   3 raw ->  1 real
    _code_00039990                              11 raw ->  2 real
    _ai_communication_get_player_rating         12 raw ->  2 real
    _actor_emotion_update                       12 raw ->  3 real
    _ai_communication_actor_talk_weight          9 raw ->  3 real
    _actor_perception_update                    30 raw -> 14 real

A function reported at "12 regions" that is actually at 2 is a function a triage
pass will wrongly deprioritise. Use this number, not the raw one, when ranking.

CAVEAT, and it matters. Normalising a branch's immediate away also hides a
SHORT versus NEAR encoding difference, which is a real byte difference even
though the instruction text is the same. So `0 REAL regions` does NOT mean
`exact` - it means every instruction's TEXT matches and the divergence is in
encoding lengths or positions. `_actor_path_refresh` reads 0 REAL with one
padding-only region: its content is identical, our encodings are about 12 bytes
longer, and January's 12 trailing NOPs absorb the difference into the same
section size. When you see `0 REAL` plus a padding-only region, the gap is
encoding length - go and compare where real code ENDS on both sides.

Usage:
    python tools/campaign/real_regions.py <unit> <function> [--ours-object OBJ]
    python tools/campaign/real_regions.py source/ai/ai_debug _code_00039990
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

TARGET_SPELLING = re.compile(r'target=[^ ]+')
ROW_PREFIX = re.compile(r'^\s*[TO]\s+[0-9a-f]+\s+')
RELOC_SITE = re.compile(r'rel\+0x[0-9a-f]+')
BRANCH_IMM = re.compile(r'\b(j[a-z]{1,3}|call|loop[a-z]*)\s+0x[0-9a-f]+')
PAD = re.compile(r'^\s*[TO]\s+[0-9a-f]+\s+(nop|lea\s+e\wx, \[e\wx\]|int3)\s*$')


def canonical(line):
    """An instruction row stripped of everything that is an ADDRESS, not content.

    Removed: the row's own address, the relocation-target spelling, the
    relocation SITE address (`rel+0x...`, which shifts whenever anything
    upstream changes length), and a branch or call's immediate target. What is
    left is the instruction's actual text.
    """
    line = ROW_PREFIX.sub('', line)
    line = TARGET_SPELLING.sub('', line)
    line = RELOC_SITE.sub('', line)
    line = BRANCH_IMM.sub(lambda m: m.group(1) + ' <target>', line)
    return line.strip()


def is_padding_only(rows):
    """True if every row is a padding instruction (trailing alignment filler)."""
    return bool(rows) and all(PAD.match(r) for r in rows)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit')
    ap.add_argument('function')
    ap.add_argument('--ours-object', default=None)
    ap.add_argument('--show', action='store_true', help='print the real regions')
    a = ap.parse_args()

    cmd = [sys.executable, 'tools/campaign/alndiff.py', a.unit, a.function,
           '--max-lines', '100000']
    if a.ours_object:
        cmd += ['--ours-object', a.ours_object]
    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT).stdout
    if not out.strip():
        sys.exit('alndiff produced no output; check the unit and function names')

    header = out.splitlines()[0]
    blocks = out.split('--- ')[1:]
    real, addressing, padding = [], 0, 0
    for b in blocks:
        rows = [l for l in b.splitlines()[1:] if l.strip().startswith(('T ', 'O '))]
        t = [l for l in rows if l.strip().startswith('T ')]
        o = [l for l in rows if l.strip().startswith('O ')]
        if is_padding_only(t) and not o or is_padding_only(o) and not t:
            padding += 1                  # trailing alignment filler on one side
            continue
        if len(t) == len(o) and all(canonical(x) == canonical(y) for x, y in zip(t, o)):
            addressing += 1               # relocation spelling / site / branch target
            continue
        real.append(b)

    print(header)
    print('regions: %d reported, %d REAL  (%d addressing-only, %d padding-only)'
          % (len(blocks), len(real), addressing, padding))
    if a.show:
        for b in real:
            print('--- ' + b.rstrip())


if __name__ == '__main__':
    main()
