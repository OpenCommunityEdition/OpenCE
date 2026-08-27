# -*- coding: utf-8 -*-
"""Accumulating EIP sampler over N ai_debug compiles, attributed to c2 functions.

Builds c2 function-start table from int3-padding boundaries, samples EIP of all
threads across many compiles, and reports the hottest c2 functions.
"""
import ctypes as C
from ctypes import wintypes as W
import json, collections, importlib.util, struct, sys

_spec = importlib.util.spec_from_file_location('c2dbg', 'c2dbg.py')
d = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(d)
k32 = d.k32

TH32CS_SNAPTHREAD = 0x4
THREAD_GET_CONTEXT = 0x8
THREAD_SUSPEND_RESUME = 0x2
C2_LO, C2_HI = 0x10700000, 0x1084a000
CREATE_NO_WINDOW = 0x08000000


class THREADENTRY32(C.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD),
                ("th32ThreadID", W.DWORD), ("th32OwnerProcessID", W.DWORD),
                ("tpBasePri", C.c_long), ("tpDeltaPri", C.c_long), ("dwFlags", W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


def build_func_starts():
    data = open(r'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream\xbox\bin\vc7\C2.Dll', 'rb').read()
    text = data[0x1000:0x1000 + 0x14a000]
    ib = 0x10700000
    starts = []
    i = 0
    n = len(text)
    # a function start = first non-0xCC byte after a run of >=1 0xCC padding
    while i < n - 1:
        if text[i] == 0xCC and text[i + 1] != 0xCC:
            starts.append(ib + 0x1000 + i + 1)
        i += 1
    starts.append(ib + 0x1000)  # first func
    starts.sort()
    return starts


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


def one_compile(hist, samples):
    cmd = json.load(open('dbg_cmd.json'))
    argv = cmd['argv']; root = cmd['root']
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    si = d.STARTUPINFOW(); si.cb = C.sizeof(si)
    pi = d.PROCESS_INFORMATION()
    k32.CreateProcessW(argv[0], C.create_unicode_buffer(cmdline), None, None, False,
                       CREATE_NO_WINDOW, None, root, C.byref(si), C.byref(pi))
    STILL = 259
    ec = W.DWORD(STILL)
    while True:
        k32.GetExitCodeProcess(pi.hProcess, C.byref(ec))
        if ec.value != STILL:
            break
        for tid in threads_of(pi.dwPid):
            hth = k32.OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, False, tid)
            if not hth:
                continue
            k32.SuspendThread(hth)
            ctx = d.WOW64_CONTEXT(); ctx.ContextFlags = d.CONTEXT_CONTROL
            if k32.Wow64GetThreadContext(hth, C.byref(ctx)):
                eip = ctx.Eip
                samples[0] += 1
                if C2_LO <= eip < C2_HI:
                    hist.append(eip)
            k32.ResumeThread(hth)
            k32.CloseHandle(hth)
    k32.CloseHandle(pi.hProcess); k32.CloseHandle(pi.hThread)


def main(niter):
    starts = build_func_starts()
    import bisect
    raw = []
    samples = [0]
    for _ in range(niter):
        one_compile(raw, samples)
    func_hits = collections.Counter()
    for eip in raw:
        idx = bisect.bisect_right(starts, eip) - 1
        func_hits[starts[idx]] += 1
    print('compiles=%d total samples=%d in-c2=%d  functions touched=%d'
          % (niter, samples[0], len(raw), len(func_hits)))
    print('top c2 functions by sample count:')
    for fs, cnt in func_hits.most_common(25):
        print('   %#010x  %d' % (fs, cnt))
    json.dump({'starts': starts, 'func_hits': dict(func_hits)}, open('c2_profile.json', 'w'))


if __name__ == '__main__':
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 15)
