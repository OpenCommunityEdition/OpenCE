# -*- coding: utf-8 -*-
"""Differential sampler: FP-heavy vs int-heavy compile -> FP-codegen functions.

Samples EIP of both compiles (same flags), attributes to c2 functions via
call-target boundaries, and reports functions whose sample share is much higher
in the FP compile.
"""
import ctypes as C
from ctypes import wintypes as W
import collections, importlib.util, struct, sys, bisect
import capstone

_spec = importlib.util.spec_from_file_location('c2dbg', 'c2dbg.py')
d = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(d)
k32 = d.k32
CREATE_NO_WINDOW = 0x08000000
TH32CS_SNAPTHREAD = 0x4
C2_LO, C2_HI = 0x10700000, 0x1084a000
ROOT = r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream'
CL = ROOT + r'\xbox\bin\vc7\CL.Exe'
FLAGS = ['/nologo', '/c', '/O2', '/Oy-', '/QIfist']


class THREADENTRY32(C.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD),
                ("th32ThreadID", W.DWORD), ("th32OwnerProcessID", W.DWORD),
                ("tpBasePri", C.c_long), ("tpDeltaPri", C.c_long), ("dwFlags", W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


def func_starts():
    data = open(CL.replace('CL.Exe', 'C2.Dll'), 'rb').read()
    text = data[0x1000:0x1000 + 0x14a000]
    ib = 0x10700000
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    starts = set()
    # call targets (E8 rel32)
    for i in range(len(text) - 5):
        if text[i] == 0xE8:
            rel = struct.unpack_from('<i', text, i + 1)[0]
            tgt = (ib + 0x1000 + i + 5 + rel) & 0xffffffff
            if C2_LO <= tgt < C2_HI:
                starts.add(tgt)
    # int3-padding boundaries
    for i in range(1, len(text)):
        if text[i - 1] == 0xCC and text[i] != 0xCC:
            starts.add(ib + 0x1000 + i)
    return sorted(starts)


def threads_of(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    te = THREADENTRY32(); te.dwSize = C.sizeof(te)
    out = []
    if k32.Thread32First(snap, C.byref(te)):
        while True:
            if te.th32OwnerProcessID == pid:
                out.append(te.th32ThreadID)
            if not k32.Thread32Next(snap, C.byref(te)):
                break
    k32.CloseHandle(snap)
    return out


def sample_compile(srcfile, starts, niter):
    import os
    hits = collections.Counter()
    total = 0
    src = os.path.abspath(srcfile)
    argv = [CL] + FLAGS + ['/Fo' + src.replace('.c', '.obj'), src]
    cmdline = ' '.join(('"%s"' % a if ' ' in a else a) for a in argv)
    for _ in range(niter):
        si = d.STARTUPINFOW(); si.cb = C.sizeof(si)
        pi = d.PROCESS_INFORMATION()
        k32.CreateProcessW(CL, C.create_unicode_buffer(cmdline), None, None, False,
                           CREATE_NO_WINDOW, None, ROOT, C.byref(si), C.byref(pi))
        ec = W.DWORD(259)
        while True:
            k32.GetExitCodeProcess(pi.hProcess, C.byref(ec))
            if ec.value != 259:
                break
            for tid in threads_of(pi.dwPid):
                hth = k32.OpenThread(0x8 | 0x2, False, tid)
                if not hth:
                    continue
                k32.SuspendThread(hth)
                ctx = d.WOW64_CONTEXT(); ctx.ContextFlags = d.CONTEXT_CONTROL
                if k32.Wow64GetThreadContext(hth, C.byref(ctx)):
                    total += 1
                    eip = ctx.Eip
                    if C2_LO <= eip < C2_HI:
                        idx = bisect.bisect_right(starts, eip) - 1
                        hits[starts[idx]] += 1
                k32.ResumeThread(hth); k32.CloseHandle(hth)
        k32.CloseHandle(pi.hProcess); k32.CloseHandle(pi.hThread)
    return hits, total


def main(niter):
    starts = func_starts()
    print('func starts:', len(starts))
    fh, ft = sample_compile('fpheavy.c', starts, niter)
    ih, it = sample_compile('intheavy.c', starts, niter)
    fin = sum(fh.values()); iin = sum(ih.values())
    print('FP: %d in-c2 / %d total   INT: %d in-c2 / %d total' % (fin, ft, iin, it))
    # functions ranked by FP-share minus INT-share
    allf = set(fh) | set(ih)
    rows = []
    for f in allf:
        fp = fh[f] / max(1, fin)
        ip = ih[f] / max(1, iin)
        rows.append((fp - ip, fh[f], ih[f], f))
    rows.sort(reverse=True)
    print('functions most FP-biased (fp_share-int_share, fp_hits, int_hits, addr):')
    for delta, fhc, ihc, f in rows[:20]:
        print('   %+.3f  fp=%-4d int=%-4d  %#010x' % (delta, fhc, ihc, f))


if __name__ == '__main__':
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 15)
