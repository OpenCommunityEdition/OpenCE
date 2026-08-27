# -*- coding: utf-8 -*-
"""c2.dll live-debug harness (64-bit host / 32-bit WOW64 debuggee).

C2.Dll loads in-process in CL.Exe at its fixed preferred base 0x10700000
(verified: no relocation), so static VAs are literal live addresses.

Usage: import and call run(bp_vas, on_hit) where bp_vas is a list of live
c2 VAs and on_hit(ctx, mem, va, hitno) is a callback receiving the WOW64
register context and a memory reader. The harness plants INT3, and on each
hit reads the 32-bit context, invokes the callback, then transparently
restores the byte, single-steps over, and re-arms.
"""
import ctypes as C
from ctypes import wintypes as W
import json

k32 = C.WinDLL('kernel32', use_last_error=True)
DEBUG_ONLY_THIS_PROCESS = 0x00000002
CREATE_NEW_CONSOLE = 0x00000010
INFINITE = 0xFFFFFFFF
DBG_CONTINUE = 0x00010002
DBG_EXCEPTION_NOT_HANDLED = 0x80010001
EXCEPTION_DEBUG_EVENT = 1
CREATE_PROCESS_DEBUG_EVENT = 3
EXIT_PROCESS_DEBUG_EVENT = 5
LOAD_DLL_DEBUG_EVENT = 6
EXC_BREAKPOINT = 0x80000003
EXC_SINGLE_STEP = 0x80000004
LPVOID = C.c_void_p
ULPTR = C.c_uint64
C2_BASE = 0x10700000

# WOW64_CONTEXT (x86 CONTEXT) — flags
WOW64_CONTEXT_i386 = 0x00010000
CONTEXT_CONTROL = WOW64_CONTEXT_i386 | 0x1
CONTEXT_INTEGER = WOW64_CONTEXT_i386 | 0x2
CONTEXT_SEG = WOW64_CONTEXT_i386 | 0x4
CONTEXT_FLOAT = WOW64_CONTEXT_i386 | 0x8
CONTEXT_FULL = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEG
CONTEXT_ALL = CONTEXT_FULL | CONTEXT_FLOAT


class FLOATING_SAVE_AREA(C.Structure):
    _fields_ = [("ControlWord", W.DWORD), ("StatusWord", W.DWORD),
                ("TagWord", W.DWORD), ("ErrorOffset", W.DWORD),
                ("ErrorSelector", W.DWORD), ("DataOffset", W.DWORD),
                ("DataSelector", W.DWORD), ("RegisterArea", C.c_ubyte * 80),
                ("Cr0NpxState", W.DWORD)]


class WOW64_CONTEXT(C.Structure):
    _fields_ = [("ContextFlags", W.DWORD),
                ("Dr0", W.DWORD), ("Dr1", W.DWORD), ("Dr2", W.DWORD),
                ("Dr3", W.DWORD), ("Dr6", W.DWORD), ("Dr7", W.DWORD),
                ("FloatSave", FLOATING_SAVE_AREA),
                ("SegGs", W.DWORD), ("SegFs", W.DWORD), ("SegEs", W.DWORD),
                ("SegDs", W.DWORD), ("Edi", W.DWORD), ("Esi", W.DWORD),
                ("Ebx", W.DWORD), ("Edx", W.DWORD), ("Ecx", W.DWORD),
                ("Eax", W.DWORD), ("Ebp", W.DWORD), ("Eip", W.DWORD),
                ("SegCs", W.DWORD), ("EFlags", W.DWORD), ("Esp", W.DWORD),
                ("SegSs", W.DWORD), ("ExtendedRegisters", C.c_ubyte * 512)]


class STARTUPINFOW(C.Structure):
    _fields_ = [("cb", W.DWORD), ("r0", W.LPWSTR), ("r1", W.LPWSTR),
                ("r2", W.LPWSTR), ("dwX", W.DWORD), ("dwY", W.DWORD),
                ("dwXSize", W.DWORD), ("dwYSize", W.DWORD), ("dwXCC", W.DWORD),
                ("dwYCC", W.DWORD), ("dwFill", W.DWORD), ("dwFlags", W.DWORD),
                ("wShow", W.WORD), ("cbR2", W.WORD), ("lpR2", LPVOID),
                ("hIn", W.HANDLE), ("hOut", W.HANDLE), ("hErr", W.HANDLE)]


class PROCESS_INFORMATION(C.Structure):
    _fields_ = [("hProcess", W.HANDLE), ("hThread", W.HANDLE),
                ("dwPid", W.DWORD), ("dwTid", W.DWORD)]


class EXCEPTION_RECORD(C.Structure):
    _fields_ = [("Code", W.DWORD), ("Flags", W.DWORD), ("Rec", ULPTR),
                ("Addr", ULPTR), ("nParams", W.DWORD), ("Info", ULPTR * 15)]


class EXCEPTION_DEBUG_INFO(C.Structure):
    _fields_ = [("ExceptionRecord", EXCEPTION_RECORD), ("dwFirstChance", W.DWORD)]


class CREATE_PROCESS_DEBUG_INFO(C.Structure):
    _fields_ = [("hFile", W.HANDLE), ("hProcess", W.HANDLE), ("hThread", W.HANDLE),
                ("lpBaseOfImage", ULPTR), ("off", W.DWORD), ("sz", W.DWORD),
                ("tlb", ULPTR), ("start", ULPTR), ("name", ULPTR), ("uni", W.WORD)]


class LOAD_DLL_DEBUG_INFO(C.Structure):
    _fields_ = [("hFile", W.HANDLE), ("lpBaseOfDll", ULPTR), ("off", W.DWORD),
                ("sz", W.DWORD), ("name", ULPTR), ("uni", W.WORD)]


class EXIT_PROCESS_DEBUG_INFO(C.Structure):
    _fields_ = [("dwExitCode", W.DWORD)]


class DBG_U(C.Union):
    _fields_ = [("Exception", EXCEPTION_DEBUG_INFO),
                ("CreateProcessInfo", CREATE_PROCESS_DEBUG_INFO),
                ("LoadDll", LOAD_DLL_DEBUG_INFO),
                ("ExitProcess", EXIT_PROCESS_DEBUG_INFO),
                ("_raw", C.c_ubyte * 160)]


class DEBUG_EVENT(C.Structure):
    _fields_ = [("code", W.DWORD), ("pid", W.DWORD), ("tid", W.DWORD), ("u", DBG_U)]


k32.OpenThread.restype = W.HANDLE
k32.OpenThread.argtypes = [W.DWORD, W.BOOL, W.DWORD]
k32.Wow64GetThreadContext.argtypes = [W.HANDLE, C.POINTER(WOW64_CONTEXT)]
k32.Wow64SetThreadContext.argtypes = [W.HANDLE, C.POINTER(WOW64_CONTEXT)]


def _rpm(hproc, addr, n):
    buf = (C.c_ubyte * n)()
    got = C.c_size_t(0)
    k32.ReadProcessMemory(hproc, LPVOID(addr), buf, n, C.byref(got))
    return bytes(buf[:got.value])


PAGE_EXECUTE_READWRITE = 0x40
k32.VirtualProtectEx.argtypes = [W.HANDLE, LPVOID, C.c_size_t, W.DWORD, C.POINTER(W.DWORD)]


def _wpm(hproc, addr, data):
    n = len(data)
    buf = (C.c_ubyte * n).from_buffer_copy(data)
    wrote = C.c_size_t(0)
    old = W.DWORD(0)
    k32.VirtualProtectEx(hproc, LPVOID(addr), n, PAGE_EXECUTE_READWRITE, C.byref(old))
    ok = k32.WriteProcessMemory(hproc, LPVOID(addr), buf, n, C.byref(wrote))
    k32.VirtualProtectEx(hproc, LPVOID(addr), n, old.value, C.byref(old))
    k32.FlushInstructionCache(hproc, LPVOID(addr), n)
    return ok and wrote.value == n


def run(bp_vas, on_hit, max_hits_per_bp=3, timeout_events=200000):
    cmd = json.load(open('dbg_cmd.json'))
    argv = cmd['argv']; root = cmd['root']
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    si = STARTUPINFOW(); si.cb = C.sizeof(si)
    pi = PROCESS_INFORMATION()
    if not k32.CreateProcessW(argv[0], C.create_unicode_buffer(cmdline), None, None,
                              False, DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
                              None, root, C.byref(si), C.byref(pi)):
        raise OSError('CreateProcessW %d' % C.get_last_error())
    hproc = None
    orig = {}          # va -> original byte
    planted = set()
    pending = {}       # tid -> va to re-arm after single-step
    hits = {va: 0 for va in bp_vas}
    de = DEBUG_EVENT()
    n = 0
    while n < timeout_events:
        n += 1
        if not k32.WaitForDebugEvent(C.byref(de), INFINITE):
            break
        status = DBG_CONTINUE
        code = de.code
        if code == CREATE_PROCESS_DEBUG_EVENT:
            hproc = de.u.CreateProcessInfo.hProcess
        elif code == LOAD_DLL_DEBUG_EVENT:
            if de.u.LoadDll.lpBaseOfDll == C2_BASE:
                for va in bp_vas:
                    orig[va] = _rpm(hproc, va, 1)
                    _wpm(hproc, va, b'\xcc')
                    planted.add(va)
        elif code == EXCEPTION_DEBUG_EVENT:
            exc = de.u.Exception.ExceptionRecord
            addr = exc.Addr & 0xffffffff
            if exc.Code == EXC_BREAKPOINT and addr in planted:
                hth = k32.OpenThread(0x1FFFFF, False, de.tid)
                ctx = WOW64_CONTEXT(); ctx.ContextFlags = CONTEXT_ALL
                k32.Wow64GetThreadContext(hth, C.byref(ctx))
                hits[addr] += 1
                try:
                    on_hit(ctx, lambda a, l: _rpm(hproc, a, l), addr, hits[addr])
                except Exception as e:
                    print('on_hit error:', e)
                # restore original, back up EIP, single-step to re-arm
                _wpm(hproc, addr, orig[addr])
                ctx.Eip = addr
                if hits[addr] < max_hits_per_bp:
                    ctx.EFlags |= 0x100      # TF
                    pending[de.tid] = addr
                ctx.ContextFlags = CONTEXT_ALL
                k32.Wow64SetThreadContext(hth, C.byref(ctx))
                k32.CloseHandle(hth)
            elif exc.Code == EXC_SINGLE_STEP and de.tid in pending:
                va = pending.pop(de.tid)
                _wpm(hproc, va, b'\xcc')      # re-arm
            elif exc.Code in (EXC_BREAKPOINT, EXC_SINGLE_STEP):
                pass  # loader breakpoint etc.
            else:
                status = DBG_EXCEPTION_NOT_HANDLED
        elif code == EXIT_PROCESS_DEBUG_EVENT:
            k32.ContinueDebugEvent(de.pid, de.tid, status)
            if de.pid == pi.dwPid:
                break
            continue
        k32.ContinueDebugEvent(de.pid, de.tid, status)
    return hits


if __name__ == '__main__':
    INVOKE = 0x107b6938  # _InvokeCompilerPass@12 (backend entry cl.exe calls)

    def dump(ctx, mem, va, hitno):
        # args are at [esp+4],[esp+8],[esp+0xc] (stdcall @12 = 3 dwords)
        stk = mem(ctx.Esp, 16)
        a = struct_unpack(stk)
        print('HIT InvokeCompilerPass #%d  eip=%#x esp=%#x  ret=%#x args=(%#x,%#x,%#x)'
              % (hitno, ctx.Eip, ctx.Esp, a[0], a[1], a[2], a[3]))
    h = run([INVOKE], dump, max_hits_per_bp=5)
    print('hit counts:', {hex(k): v for k, v in h.items()})


def struct_unpack(b):
    import struct
    return struct.unpack_from('<4I', b + b'\0' * 16)