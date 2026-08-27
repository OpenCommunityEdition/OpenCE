"""Find conditional branches whose polarity is inverted against the reference.

This is the detector for a specific, otherwise invisible class of residual: an
`if`/`else` or a ternary whose two arms are swapped relative to the original.
MSVC puts a ternary's true arm in the fallthrough, so

    a!=b ? x : y        and        a==b ? y : x

are the same program and different code. When both arms are the same length --
which they usually are -- the mistake changes no size, no relocation and no
frame, so every other diagnostic in docs/house_rules.md is blind to it. Only
branch polarity finds it.

Usage:
    python tools/inverted_branch_scan.py <reference.obj> <candidate.obj> [symbol ...]

With no symbols it scans every .text section owner the two objects share.

Validate on a known positive before trusting a zero -- a broken scan and a clean
object look identical. Flipping one comparison in a matched function and
re-running is enough.
"""
import sys, os, re, difflib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
import capstone

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_hex = re.compile(r'0x[0-9a-f]+')

OPPOSITE = {}
for _a, _b in [('je', 'jne'), ('jl', 'jge'), ('jle', 'jg'), ('jb', 'jae'),
               ('jbe', 'ja'), ('jp', 'jnp'), ('js', 'jns'), ('jo', 'jno')]:
    OPPOSITE[_a] = _b
    OPPOSITE[_b] = _a


def _owners(obj):
    """Map owning symbol name -> section index for .text COMDATs."""
    out = {}
    for sym in obj['symbols']:
        name = sym['name']
        if not name.startswith('_') or name.startswith('__') or name.startswith('??'):
            continue
        if sym['section'] <= 0 or sym['value'] != 0:
            continue
        section = obj['sections'][sym['section'] - 1]
        if section['name'].startswith('.text'):
            out.setdefault(name, sym['section'])
    return out


def _section_bytes(obj, index):
    section = obj['sections'][index - 1]
    return bytes(cc._section_bytes(obj, section))[:section['size']]


def _token(insn):
    """Normalized token. Branch and call targets are erased so that accumulated
    drift between the two streams does not desynchronize the alignment."""
    if insn.mnemonic.startswith('j') or insn.mnemonic == 'call':
        return insn.mnemonic + ' @'
    return insn.mnemonic + ' ' + _hex.sub('#', insn.op_str)


def scan_function(reference, candidate, name, ref_index, cand_index):
    a = list(_md.disasm(_section_bytes(reference, ref_index), 0))
    b = list(_md.disasm(_section_bytes(candidate, cand_index), 0))
    matcher = difflib.SequenceMatcher(None, [_token(i) for i in a],
                                      [_token(i) for i in b], autojunk=False)
    hits = []
    for op, i1, i2, j1, j2 in matcher.get_opcodes():
        if op != 'replace':
            continue
        for k in range(min(i2 - i1, j2 - j1)):
            x, y = a[i1 + k], b[j1 + k]
            if OPPOSITE.get(x.mnemonic) == y.mnemonic:
                hits.append((name, x.address, x.mnemonic, y.address, y.mnemonic))
    return hits


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    reference = cc.load(argv[1])
    candidate = cc.load(argv[2])
    wanted = set(argv[3:])

    ref_owners = _owners(reference)
    cand_owners = _owners(candidate)

    scanned = 0
    hits = []
    for name in sorted(set(ref_owners) & set(cand_owners)):
        if wanted and name not in wanted:
            continue
        scanned += 1
        hits.extend(scan_function(reference, candidate, name,
                                  ref_owners[name], cand_owners[name]))

    for name, ref_addr, ref_mnem, cand_addr, cand_mnem in hits:
        print('%-34s reference %05x %-4s   vs candidate %05x %-4s'
              % (name, ref_addr, ref_mnem, cand_addr, cand_mnem))
    print('scanned %d shared functions, %d inverted-polarity branches'
          % (scanned, len(hits)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
