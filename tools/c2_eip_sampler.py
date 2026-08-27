# -*- coding: utf-8 -*-
"""Sample EIP of a free-running ai_debug compile to find where C2.Dll executes.

Launches cl.exe suspended, resumes, then tight-loops: enumerate the process's
threads (Toolhelp), suspend each, read WOW64 Eip, resume. Histograms EIPs that
land in C2.Dll's range [0x10700000, 0x1084a000). Confirms c2 executes and gives
hot anchor addresses to breakpoint.
"""
import ctypes as C
from ctypes import wintypes as W
import json, collections, importlib.util

_spec = importlib.util.spec_from_file_location('c2dbg', 'c2dbg.py')
d = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(d)
k32 = d.k32

TH32CS_SNAPTHREAD = 0x4
THREAD_GET_CONTEXT = 0x8
THREAD_SUSPEND_RESUME = 0x2
C2_LO, C2_HI = 0x10700000, 0x1084a000
CREATE_SUSPENDED = 0x4


class THREADENTRY32(C.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD),
                ("th32ThreadID", W.DWORD), ("th32OwnerProcessID", W.DWORD),
                ("tpBasePri", C.c_long), ("tpDeltaPri", C.c_long), ("dwFlags", W.DWORD)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


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


def main():
    cmd = json.load(open('dbg_cmd.json'))
    argv = cmd['argv']; root = cmd['root']
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    si = d.STARTUPINFOW(); si.cb = C.sizeof(si)
    pi = d.PROCESS_INFORMATION()
    k32.CreateProcessW(argv[0], C.create_unicode_buffer(cmdline), None, None, False,
                       CREATE_SUSPENDED | d.CREATE_NEW_CONSOLE, None, root,
                       C.byref(si), C.byref(pi))
    k32.ResumeThread(pi.hThread)
    hist = collections.Counter()
    samples = 0
    c2_samples = 0
    # loop until process exits
    STILL_ACTIVE = 259
    ec = W.DWORD(STILL_ACTIVE)
    while True:
        k32.GetExitCodeProcess(pi.hProcess, C.byref(ec))
        if ec.value != STILL_ACTIVE:
            break
        for tid in threads_of(pi.dwPid):
            hth = k32.OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, False, tid)
            if not hth:
                continue
            k32.SuspendThread(hth)
            ctx = d.WOW64_CONTEXT(); ctx.ContextFlags = d.CONTEXT_CONTROL
            if k32.Wow64GetThreadContext(hth, C.byref(ctx)):
                eip = ctx.Eip
                samples += 1
                if C2_LO <= eip < C2_HI:
                    c2_samples += 1
                    hist[eip & ~0xf] += 1     # 16-byte bucket
            k32.ResumeThread(hth)
            k32.CloseHandle(hth)
    print('total samples %d, in-C2 %d (%.1f%%)'
          % (samples, c2_samples, 100.0 * c2_samples / max(1, samples)))
    print('top C2 EIP buckets:')
    for addr, cnt in hist.most_common(25):
        print('   %#010x  %d' % (addr, cnt))


if __name__ == '__main__':
    main()
