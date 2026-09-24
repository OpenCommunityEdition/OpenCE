"""Sample the VC7 backend during a periodic_functions candidate compile.

This is a read-only probe of the compiler process; it never changes source.
It uses the same command line as dbg32's generated dbg_cmd.u16.
"""

import collections
import ctypes as C
from ctypes import wintypes as W
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import c2_live_debug as d


CREATE_SUSPENDED = 0x4
CREATE_NO_WINDOW = 0x08000000
STILL_ACTIVE = 259

d.k32.CreateProcessW.argtypes = [W.LPCWSTR, W.LPWSTR, C.c_void_p, C.c_void_p,
                                 W.BOOL, W.DWORD, C.c_void_p, W.LPCWSTR,
                                 C.POINTER(d.STARTUPINFOW), C.POINTER(d.PROCESS_INFORMATION)]
d.k32.ResumeThread.argtypes = [W.HANDLE]
d.k32.SuspendThread.argtypes = [W.HANDLE]
d.k32.GetExitCodeProcess.argtypes = [W.HANDLE, C.POINTER(W.DWORD)]


def sample_once(cmdline, cwd, hits):
    si = d.STARTUPINFOW()
    si.cb = C.sizeof(si)
    pi = d.PROCESS_INFORMATION()
    exe = cmdline[1:cmdline.index('"', 1)] if cmdline.startswith('"') else cmdline.split(' ', 1)[0]
    buf = C.create_unicode_buffer(cmdline)
    if not d.k32.CreateProcessW(exe, buf, None, None, False,
                               CREATE_SUSPENDED | CREATE_NO_WINDOW, None,
                               cwd, C.byref(si), C.byref(pi)):
        raise OSError(C.get_last_error())
    d.k32.ResumeThread(pi.hThread)
    exit_code = W.DWORD(STILL_ACTIVE)
    while d.k32.GetExitCodeProcess(pi.hProcess, C.byref(exit_code)) and exit_code.value == STILL_ACTIVE:
        if d.k32.SuspendThread(pi.hThread) == 0xffffffff:
            break
        ctx = d.WOW64_CONTEXT()
        ctx.ContextFlags = d.CONTEXT_CONTROL
        if d.k32.Wow64GetThreadContext(pi.hThread, C.byref(ctx)):
            if 0x10700000 <= ctx.Eip < 0x1084a000:
                hits[ctx.Eip & ~0xf] += 1
        d.k32.ResumeThread(pi.hThread)
    d.k32.CloseHandle(pi.hThread)
    d.k32.CloseHandle(pi.hProcess)


def main():
    here = os.path.dirname(__file__)
    cmdline = open(os.path.join(here, 'dbg_cmd.u16'), 'rb').read().decode('utf-16-le').rstrip('\0')
    cwd = open(os.path.join(here, 'dbg_dir.u16'), 'rb').read().decode('utf-16-le').rstrip('\0')
    hits = collections.Counter()
    for _ in range(int(sys.argv[1]) if len(sys.argv) > 1 else 8):
        sample_once(cmdline, cwd, hits)
    print('C2 samples:', sum(hits.values()))
    print('dag candidate region 0x107bd000-0x107bf800:',
          sum(n for a, n in hits.items() if 0x107bd000 <= a < 0x107bf800))
    for addr, count in hits.most_common(30):
        print('%08x %d' % (addr, count))


if __name__ == '__main__':
    main()
