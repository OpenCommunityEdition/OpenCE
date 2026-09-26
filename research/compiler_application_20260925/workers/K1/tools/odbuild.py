#!/usr/bin/env python3
"""Read-only evidence tool for the later first-party UNOPTIMISED debug build
(halo_cache_symbols.exe: /Od + /RTC, no PDB, no symbols). DATA ONLY - the binary
is never executed.

Why it matters: /Od does not inline, does not reorder statements and does not
invent or merge locals. So this build shows, for almost every Halo function:
  * STATEMENT ORDER and which helpers are REAL CALLS (vs expanded expressions),
  * every LOCAL as its own frame slot (store-then-reload), incl. pointer locals,
    result accumulators, loop counters and their WIDTH (movsx/movzx = short/char),
  * the number of `return` sites (jumps to the single epilogue),
  * RTC descriptors naming every address-taken AGGREGATE local with its size,
  * assert LINE NUMBERS (push imm before the assert call) - compare with
    January's to see whether the source text matches up to that line.
It is a 2020-era build with KNOWN source differences: use it for names, types,
topology and intent - NEVER as January byte proof. Confirm every fact against
January's own bytes (frame size, reference multiset, ret count) before using it.

  python scratch/orch/odbuild.py str "hover thrusters"        # string VAs, code refs, containing function
  python scratch/orch/odbuild.py fn 0x8f5530 [--out file]     # whole function, thunks/strings/floats resolved
  python scratch/orch/odbuild.py callers 0x8f5530             # call sites (through the ILT thunk too)
  python scratch/orch/odbuild.py rtc 0x8f5530                 # RTC stack-variable descriptor(s): offset size name
  python scratch/orch/odbuild.py file "ai\\actions.c"         # every function that references that __FILE__ string

Finding your function: search a string literal it uses (assert text, a marker or
console string), or take `callers` of a callee you already located, or use
`file` with the source path to list all functions of the TU that assert.
"""
import struct
import sys

import capstone
import pefile

EXE = 'C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe'

pe = pefile.PE(EXE, fast_load=True)
BASE = pe.OPTIONAL_HEADER.ImageBase
DATA = pe.__data__
SECS = [(s.Name.rstrip(b'\0').decode(), BASE + s.VirtualAddress, s.PointerToRawData,
         s.SizeOfRawData, max(s.Misc_VirtualSize, s.SizeOfRawData)) for s in pe.sections]
TEXT = [s for s in SECS if s[0] == '.text'][0]
MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def va2off(va):
    for n, sva, off, rsz, vsz in SECS:
        if sva <= va < sva + rsz:
            return off + (va - sva)
    return None


def off2va(o):
    for n, sva, off, rsz, vsz in SECS:
        if off <= o < off + rsz:
            return sva + (o - off)
    return None


def in_text(va):
    return TEXT[1] <= va < TEXT[1] + TEXT[3]


def cstring(va, limit=96):
    o = va2off(va)
    if o is None:
        return None
    end = DATA.find(b'\0', o, o + 400)
    raw = bytes(DATA[o:end if end >= 0 else o + limit])
    if len(raw) < 3 or any(c < 9 or c > 126 for c in raw):
        return None
    return raw[:limit].decode('ascii', 'replace')


def thunk_target(va):
    """Debug builds call through an incremental-link thunk: `jmp rel32`."""
    o = va2off(va)
    if o is not None and DATA[o] == 0xE9:
        return (va + 5 + struct.unpack_from('<i', DATA, o + 1)[0]) & 0xffffffff
    return va


def func_start(va):
    """Nearest `push ebp; mov ebp,esp` at or before va."""
    o = va2off(va)
    lo = va2off(TEXT[1])
    while o > lo:
        if DATA[o] == 0x55 and DATA[o + 1] == 0x8B and DATA[o + 2] == 0xEC:
            return off2va(o)
        o -= 1
    return None


def func_end(start):
    """First `ret` that is followed by int3 padding or another prologue."""
    o = va2off(start)
    blob = bytes(DATA[o:o + 0x8000])
    last = None
    for ins in MD.disasm(blob, start):
        if ins.mnemonic in ('ret', 'retn'):
            nxt = ins.address + ins.size - start
            tail = blob[nxt:nxt + 3]
            if tail[:1] in (b'\xcc', b'\x55') or tail[:2] == b'\x8b\xff':
                return ins.address + ins.size
            last = ins.address + ins.size
    return last or start + 0x400


def find_strings(text):
    b = text.encode('latin-1')
    out, i = [], 0
    while True:
        i = DATA.find(b, i)
        if i < 0:
            break
        s = i
        while s > 0 and DATA[s - 1] != 0:
            s -= 1
        va = off2va(s)
        if va is not None and va not in out:
            out.append(va)
        i += 1
    return out


def refs_to(va):
    pat = struct.pack('<I', va)
    n, sva, off, rsz, vsz = TEXT
    blob = DATA[off:off + rsz]
    out, i = [], 0
    while True:
        i = blob.find(pat, i)
        if i < 0:
            break
        out.append(sva + i)
        i += 1
    return out


def callers(target):
    """E8 rel32 call sites reaching `target` directly or through its thunk."""
    n, sva, off, rsz, vsz = TEXT
    blob = DATA[off:off + rsz]
    thunks = {target}
    i = 0
    while True:                       # thunks: E9 rel32 landing on target
        i = blob.find(b'\xE9', i)
        if i < 0:
            break
        if i + 5 <= len(blob):
            dst = (sva + i + 5 + struct.unpack_from('<i', blob, i + 1)[0]) & 0xffffffff
            if dst == target:
                thunks.add(sva + i)
        i += 1
    out, i = [], 0
    while True:
        i = blob.find(b'\xE8', i)
        if i < 0:
            break
        if i + 5 <= len(blob):
            dst = (sva + i + 5 + struct.unpack_from('<i', blob, i + 1)[0]) & 0xffffffff
            if dst in thunks:
                out.append(sva + i)
        i += 1
    return out


def float_at(va):
    o = va2off(va)
    if o is None:
        return None
    f = struct.unpack_from('<f', DATA, o)[0]
    d = struct.unpack_from('<d', DATA, o)[0]
    return f, d


def annotate(ins):
    notes = []
    if ins.mnemonic == 'call' and ins.op_str.startswith('0x'):
        t = int(ins.op_str, 16)
        r = thunk_target(t)
        notes.append('-> fn 0x%x' % r if r != t else '-> 0x%x' % t)
    for tok in ins.op_str.replace('[', ' ').replace(']', ' ').replace(',', ' ').split():
        if tok.startswith('0x'):
            try:
                v = int(tok, 16)
            except ValueError:
                continue
            if v < BASE + 0x1000 or in_text(v):
                continue
            s = cstring(v)
            if s:
                notes.append('"%s"' % s)
            elif ins.mnemonic.startswith('f') and ins.mnemonic not in ('fs',):
                fv = float_at(v)
                if fv:
                    width = 'qword' in ins.op_str
                    notes.append('= %r' % (fv[1] if width else fv[0]))
    return ('   ; ' + ' '.join(notes)) if notes else ''


def rtc(start, end):
    """/RTCs: `lea edx,[desc]` ... `call _RTC_CheckStackVars`. desc = {count, vars*};
    var = {int offset; int size; char *name}."""
    o = va2off(start)
    found = []
    for ins in MD.disasm(bytes(DATA[o:o + (end - start)]), start):
        if ins.mnemonic == 'lea' and ins.op_str.startswith('edx, [0x'):
            desc = int(ins.op_str[ins.op_str.index('[') + 1:-1], 16)
            d = va2off(desc)
            if d is None:
                continue
            count, vars_va = struct.unpack_from('<iI', DATA, d)
            v = va2off(vars_va)
            if v is None or not 0 < count < 64:
                continue
            rows = []
            for k in range(count):
                off_, size, name_va = struct.unpack_from('<iiI', DATA, v + k * 12)
                rows.append((off_, size, cstring(name_va) or '?'))
            found.append((desc, rows))
    return found


def cmd_fn(va, out=None):
    start = func_start(va) if not (DATA[va2off(va)] == 0x55) else va
    end = func_end(start)
    o = va2off(start)
    lines = ['; function 0x%x .. 0x%x  (%d bytes, /Od build - topology evidence only)' % (start, end, end - start)]
    rets = 0
    for ins in MD.disasm(bytes(DATA[o:o + (end - start)]), start):
        if ins.mnemonic.startswith('ret'):
            rets += 1
        lines.append('%08x  %-7s %s%s' % (ins.address, ins.mnemonic, ins.op_str, annotate(ins)))
    for desc, rows in rtc(start, end):
        lines.append('; RTC descriptor 0x%x - address-taken aggregate locals:' % desc)
        for off_, size, name in rows:
            lines.append(';     [ebp%+#x]  size %-5d %s' % (off_, size, name))
    text = '\n'.join(lines)
    if out:
        open(out, 'w', encoding='utf-8', newline='\n').write(text + '\n')
        print('wrote %d lines to %s (function 0x%x..0x%x)' % (len(lines), out, start, end))
    else:
        print(text)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    cmd, arg = sys.argv[1], sys.argv[2]
    if cmd == 'str':
        for sva in find_strings(arg):
            print('string 0x%x  "%s"' % (sva, cstring(sva, 120)))
            for r in refs_to(sva):
                fs = func_start(r)
                print('    ref at 0x%x   in function 0x%x' % (r, fs or 0))
    elif cmd == 'file':
        seen = {}
        for sva in find_strings(arg):
            for r in refs_to(sva):
                fs = func_start(r)
                seen.setdefault(fs, []).append(r)
        for fs in sorted(k for k in seen if k):
            print('function 0x%x   (%d __FILE__ refs, size %d)' % (fs, len(seen[fs]), func_end(fs) - fs))
    elif cmd == 'fn':
        out = sys.argv[sys.argv.index('--out') + 1] if '--out' in sys.argv else None
        cmd_fn(int(arg, 16), out)
    elif cmd == 'callers':
        t = int(arg, 16)
        for c in callers(t):
            print('call at 0x%x   in function 0x%x' % (c, func_start(c) or 0))
    elif cmd == 'rtc':
        start = func_start(int(arg, 16))
        for desc, rows in rtc(start, func_end(start)):
            print('RTC descriptor 0x%x' % desc)
            for off_, size, name in rows:
                print('    [ebp%+#x]  size %-5d %s' % (off_, size, name))
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
