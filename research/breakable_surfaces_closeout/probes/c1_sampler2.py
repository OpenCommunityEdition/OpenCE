"""Differential EIP profile of C1.Dll, sampling via a SUSPENDED launch.

The Toolhelp version failed two ways (OpenThread denied; one loop pass per
compile).  This launches CL.Exe with CREATE_SUSPENDED so the main thread
handle is owned outright -- no Toolhelp, no OpenThread -- resumes it, then
tight-loops Suspend / Wow64GetThreadContext / Resume on that one handle.

Differential: m6only.c (has the cross product) minus m6none.c (identical
headers and locals, expression removed), so header parsing cancels.

usage: python scratch/c1_sampler2.py <runs> [lo_hex hi_hex]
"""
import bisect
import collections
import ctypes as C
from ctypes import wintypes as W
import os
import re
import struct
import sys

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

class STARTUPINFOW(C.Structure):
    _fields_ = [("cb", W.DWORD), ("lpReserved", W.LPWSTR), ("lpDesktop", W.LPWSTR),
                ("lpTitle", W.LPWSTR), ("dwX", W.DWORD), ("dwY", W.DWORD),
                ("dwXSize", W.DWORD), ("dwYSize", W.DWORD), ("dwXCountChars", W.DWORD),
                ("dwYCountChars", W.DWORD), ("dwFillAttribute", W.DWORD),
                ("dwFlags", W.DWORD), ("wShowWindow", W.WORD), ("cbReserved2", W.WORD),
                ("lpReserved2", C.POINTER(C.c_byte)), ("hStdInput", W.HANDLE),
                ("hStdOutput", W.HANDLE), ("hStdError", W.HANDLE)]

class PROCESS_INFORMATION(C.Structure):
    _fields_ = [("hProcess", W.HANDLE), ("hThread", W.HANDLE),
                ("dwProcessId", W.DWORD), ("dwThreadId", W.DWORD)]

CTX_CONTROL = 0x00010001
CREATE_SUSPENDED = 0x4
CREATE_NO_WINDOW = 0x08000000
STARTF_USESTDHANDLES = 0x100
STILL_ACTIVE = 259
k32.CreateFileW.restype = W.HANDLE


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
    argv = [CL, "/nologo", "/c"] + FLAGS + ["/Foscratch/_samp.obj", os.path.abspath(src)]
    cmd = " ".join('"%s"' % a if " " in a else a for a in argv)
    nul = k32.CreateFileW("NUL", 0x40000000, 3, None, 3, 0x80, None)
    si = STARTUPINFOW(); si.cb = C.sizeof(si)
    si.dwFlags = STARTF_USESTDHANDLES
    si.hStdInput = nul; si.hStdOutput = nul; si.hStdError = nul
    pi = PROCESS_INFORMATION()
    ok = k32.CreateProcessW(None, C.create_unicode_buffer(cmd), None, None, True,
                            CREATE_SUSPENDED | CREATE_NO_WINDOW, None,
                            os.getcwd(), C.byref(si), C.byref(pi))
    if not ok:
        print("CreateProcessW failed err=%d" % C.get_last_error())
        return 0
    k32.ResumeThread(pi.hThread)
    n = 0
    code = W.DWORD(STILL_ACTIVE)
    while True:
        k32.GetExitCodeProcess(pi.hProcess, C.byref(code))
        if code.value != STILL_ACTIVE:
            break
        if k32.SuspendThread(pi.hThread) != 0xFFFFFFFF:
            ctx = WOW64_CONTEXT(); ctx.ContextFlags = CTX_CONTROL
            if k32.Wow64GetThreadContext(pi.hThread, C.byref(ctx)):
                eip = ctx.Eip
                if lo <= eip < hi:
                    hist[eip] += 1
                    n += 1
            k32.ResumeThread(pi.hThread)
    k32.CloseHandle(pi.hThread); k32.CloseHandle(pi.hProcess); k32.CloseHandle(nul)
    return n


def module_text(path):
    d = open(path, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 0x34)[0]
    so = pe + 24 + opt
    for i in range(nsec):
        off = so + i * 40
        name = d[off:off + 8].rstrip(b"\0").decode()
        vsz, va, rsz, ptr = struct.unpack_from("<IIII", d, off + 8)
        if name == ".text":
            return d, base + va, ptr, min(vsz, rsz)
    raise SystemExit("no .text")


def func_starts(d, text_va, ptr, ln):
    starts, prev_cc = [], True
    for i in range(ptr, ptr + ln):
        if d[i] == 0xCC:
            prev_cc = True
        else:
            if prev_cc:
                starts.append(text_va + (i - ptr))
            prev_cc = False
    return starts


if __name__ == "__main__":
    runs = int(sys.argv[1]) if len(sys.argv) > 1 else 8
    lo = int(sys.argv[2], 16) if len(sys.argv) > 3 else 0x10600000
    hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x10663000
    ha, hb = collections.Counter(), collections.Counter()
    ta = tb = 0
    for r in range(runs):
        ta += sample_one("scratch/m6only.c", lo, hi, ha)
        tb += sample_one("scratch/m6none.c", lo, hi, hb)
    print("samples: with-expression %d, control %d  (%d runs each)" % (ta, tb, runs))
    if ta == 0:
        print("STILL NO SAMPLES IN RANGE")
        sys.exit(0)
    d, tva, ptr, ln = module_text("xbox/bin/vc7/C1.Dll")
    starts = func_starts(d, tva, ptr, ln)
    print("C1 function starts found: %d" % len(starts))

    def attrib(h):
        out = collections.Counter()
        for eip, c in h.items():
            k = bisect.bisect_right(starts, eip) - 1
            if k >= 0:
                out[starts[k]] += c
        return out

    fa, fb = attrib(ha), attrib(hb)
    sa, sb = 1.0 / ta, 1.0 / max(tb, 1)
    rows = [(fa[k] * sa - fb.get(k, 0) * sb, k, fa[k], fb.get(k, 0)) for k in fa]
    rows.sort(reverse=True)
    print("\ntop C1 functions by EXCESS (with-expression minus control, normalised):")
    for d_, k, x, y in rows[:15]:
        print("  0x%08x  excess %+.4f   with=%-5d ctrl=%-5d" % (k, d_, x, y))
    # Raw EIPs are real instruction addresses; the int3-derived "function
    # starts" above are only labels and are NOT valid breakpoint targets.
    rs = [(ha[k] * sa - hb.get(k, 0) * sb, k, ha[k], hb.get(k, 0)) for k in ha]
    rs.sort(reverse=True)
    print("")
    print("top raw EIPs by excess (valid breakpoint targets):")
    for d_, k, x, y in rs[:12]:
        print("  0x%08x  excess %+.5f   with=%-5d ctrl=%-5d" % (k, d_, x, y))
    print("\ntop C1 functions by raw share (with-expression):")
    for k, c in fa.most_common(8):
        print("  0x%08x  %5.2f%%  (%d samples)" % (k, 100.0 * c / ta, c))
