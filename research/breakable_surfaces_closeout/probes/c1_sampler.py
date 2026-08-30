"""Differential EIP profile of C1.Dll (the C front end) over real compiles.

64-bit Python sampling a 32-bit (WOW64) CL.Exe: enumerate threads, suspend,
Wow64GetThreadContext, read Eip, resume.  Histogram addresses inside a module
range, then subtract a control compile so that header parsing -- which
dominates a small TU -- cancels and expression-building code stands out.

usage: python scratch/c1_sampler.py <runs> [lo_hex hi_hex]
"""
import ctypes as C
from ctypes import wintypes as W
import collections, os, re, subprocess, sys

k32 = C.WinDLL("kernel32", use_last_error=True)

class FSA(C.Structure):
    _fields_ = [("ControlWord", W.DWORD), ("StatusWord", W.DWORD), ("TagWord", W.DWORD),
                ("ErrorOffset", W.DWORD), ("ErrorSelector", W.DWORD),
                ("DataOffset", W.DWORD), ("DataSelector", W.DWORD),
                ("RegisterArea", C.c_byte * 80), ("Cr0NpxState", W.DWORD)]

class WOW64_CONTEXT(C.Structure):
    _fields_ = [("ContextFlags", W.DWORD), ("Dr0", W.DWORD), ("Dr1", W.DWORD),
                ("Dr2", W.DWORD), ("Dr3", W.DWORD), ("Dr6", W.DWORD), ("Dr7", W.DWORD),
                ("FloatSave", FSA),
                ("SegGs", W.DWORD), ("SegFs", W.DWORD), ("SegEs", W.DWORD),
                ("SegDs", W.DWORD), ("Edi", W.DWORD), ("Esi", W.DWORD),
                ("Ebx", W.DWORD), ("Edx", W.DWORD), ("Ecx", W.DWORD), ("Eax", W.DWORD),
                ("Ebp", W.DWORD), ("Eip", W.DWORD), ("SegCs", W.DWORD),
                ("EFlags", W.DWORD), ("Esp", W.DWORD), ("SegSs", W.DWORD),
                ("ExtendedRegisters", C.c_byte * 512)]

class TE32(C.Structure):
    _fields_ = [("dwSize", W.DWORD), ("cntUsage", W.DWORD), ("th32ThreadID", W.DWORD),
                ("th32OwnerProcessID", W.DWORD), ("tpBasePri", C.c_long),
                ("tpDeltaPri", C.c_long), ("dwFlags", W.DWORD)]

CTX_CONTROL = 0x00010001
k32.OpenThread.restype = W.HANDLE
k32.CreateToolhelp32Snapshot.restype = W.HANDLE


def cflags():
    bn = open("build.ninja").read()
    B = chr(92)
    key = "build" + B + "base" + B + "source" + B + "physics" + B + "breakable_surfaces.obj:"
    i = bn.index(key); j = bn.index("cflags = ", i); k = bn.index("\nbuild ", j)
    cf = re.sub(r"\s+", " ", bn[j + 9:k].replace("$\n", " ")).strip()
    toks = re.findall(r'/I"[^"]+"|\S+', cf)
    return [("/I" + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in toks]


CL = os.path.abspath("xbox/bin/vc7/CL.Exe")
FLAGS = cflags()


def sample_one(src, lo, hi, hist):
    obj = "scratch/_samp.obj"
    p = subprocess.Popen([CL, "/nologo", "/c"] + FLAGS + ["/Fo" + obj, src],
                         stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    pid = p.pid
    n = 0
    while p.poll() is None:
        snap = k32.CreateToolhelp32Snapshot(0x4, 0)
        if snap == W.HANDLE(-1).value:
            break
        te = TE32(); te.dwSize = C.sizeof(TE32)
        ok = k32.Thread32First(snap, C.byref(te))
        while ok:
            if te.th32OwnerProcessID == pid:
                h = k32.OpenThread(0x2 | 0x8, False, te.th32ThreadID)
                if h:
                    if k32.SuspendThread(h) != 0xFFFFFFFF:
                        ctx = WOW64_CONTEXT(); ctx.ContextFlags = CTX_CONTROL
                        if k32.Wow64GetThreadContext(h, C.byref(ctx)):
                            eip = ctx.Eip
                            if lo <= eip < hi:
                                hist[eip & ~0xF] += 1
                                n += 1
                        k32.ResumeThread(h)
                    k32.CloseHandle(h)
            ok = k32.Thread32Next(snap, C.byref(te))
        k32.CloseHandle(snap)
    p.wait()
    return n


def profile(src, runs, lo, hi):
    hist = collections.Counter()
    tot = 0
    for _ in range(runs):
        tot += sample_one(src, lo, hi, hist)
    return hist, tot


if __name__ == "__main__":
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 12
    lo = int(sys.argv[2], 16) if len(sys.argv) > 3 else 0x10600000
    hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x10663000
    a, ta = profile("scratch/m6only.c", runs, lo, hi)
    b, tb = profile("scratch/m6none.c", runs, lo, hi)
    print("samples in range: with-expression %d, control %d" % (ta, tb))
    if ta == 0:
        print("NO SAMPLES IN RANGE -- module not executing there, or sampling too slow")
        sys.exit(0)
    sa = 1.0 / max(ta, 1); sb = 1.0 / max(tb, 1)
    diff = {k: a[k] * sa - b.get(k, 0) * sb for k in a}
    print("\ntop excess addresses (with-expression minus control, normalised):")
    for addr, d in sorted(diff.items(), key=lambda kv: -kv[1])[:20]:
        print("  0x%08x  excess %+.4f   (with %4d / ctrl %4d)" %
              (addr, d, a[addr], b.get(addr, 0)))
