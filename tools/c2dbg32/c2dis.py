# -*- coding: utf-8 -*-
"""Disassemble a c2.dll function (fixed base 0x10700000) from a VA until int3 pad.
Usage: c2dis.py <hexVA> [maxbytes]   or   c2dis.py --find <hexVA> to show enclosing func.
"""
import sys, capstone

C2 = r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream\xbox\bin\vc7\C2.Dll"
IB = 0x10700000
TEXT_OFF = 0x1000
TEXT_VA = IB + TEXT_OFF

data = open(C2, "rb").read()
# .text is 0x14a000 per acc_sampler
text = data[TEXT_OFF:TEXT_OFF + 0x14a000]
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True


def va_to_off(va):
    return va - TEXT_VA


def func_start(va):
    # scan back to a 0xCC run boundary
    o = va_to_off(va)
    while o > 0 and not (text[o - 1] == 0xCC and text[o] != 0xCC):
        o -= 1
    return TEXT_VA + o


def func_end(va):
    o = va_to_off(va)
    n = len(text)
    while o < n - 1:
        if text[o] == 0xCC and (o + 1 >= n or text[o + 1] != 0xCC or text[o] == 0xCC):
            # find run of cc
            if text[o] == 0xCC:
                # confirm it's padding: several cc or aligned
                return TEXT_VA + o
        o += 1
    return TEXT_VA + n


def disasm(va, maxb=0):
    start = func_start(va)
    if maxb == 0:
        end = func_end(start)
        maxb = end - start
    o = va_to_off(start)
    chunk = text[o:o + maxb]
    for ins in md.disasm(chunk, start):
        mark = " <==" if ins.address == va else ""
        print("%08x  %-30s %s%s" % (ins.address, ins.bytes.hex(), ins.mnemonic + " " + ins.op_str, mark))


if __name__ == "__main__":
    if sys.argv[1] == "--start":
        print("%08x" % func_start(int(sys.argv[2], 16)))
    else:
        va = int(sys.argv[1], 16)
        maxb = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        print("func_start=%08x" % func_start(va))
        disasm(va, maxb)
