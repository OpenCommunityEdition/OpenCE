# -*- coding: utf-8 -*-
"""Find the enclosing c2 function of a VA and disassemble it fully.
Function boundaries = runs of 0xCC padding. Handles data-in-code by resyncing.
Usage: c2func.py <hexVA> [ctxbytes-before]
"""
import sys, capstone

C2 = r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream\xbox\bin\vc7\C2.Dll"
IB = 0x10700000
TOFF = 0x1000
TVA = IB + TOFF
data = open(C2, "rb").read()
text = data[TOFF:TOFF + 0x14a000]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def func_start(va):
    """Scan back for a >=2-byte 0xCC run (padding), return the byte after it."""
    o = va - TVA
    while o > 0:
        if text[o - 1] == 0xCC and text[o - 2] == 0xCC:
            return TVA + o
        o -= 1
    return TVA


def func_end(va):
    o = va - TVA
    n = len(text)
    while o < n - 2:
        if text[o] == 0xCC and text[o + 1] == 0xCC:
            return TVA + o
        o += 1
    return TVA + n


if __name__ == "__main__":
    va = int(sys.argv[1], 16)
    start = func_start(va)
    end = func_end(va)
    print("func %08x .. %08x  (%d bytes), target %08x" % (start, end, end - start, va))
    o = start - TVA
    chunk = text[o:end - TVA]
    for ins in md.disasm(chunk, start):
        mark = " <==TARGET" if ins.address == va else ""
        print("%08x  %-22s %s%s" % (ins.address, ins.bytes.hex(), ins.mnemonic + " " + ins.op_str, mark))
