"""Rebase ILP32 guest code onto a base register (the macOS port).

The guest (port/android/README.md, "How the port operates") is ILP32
AArch64 code: 32-bit pointers, kept zero-extended in 64-bit registers, and
used as they are as addresses. Android lets the guest live in the low 4 GB
of its process. macOS does not: every native arm64 process has a hard 4 GB
page zero, and nothing can be mapped below it. The macOS port therefore
places the guest's 4 GB at a 4 GB-aligned base address B instead
(port/macos/host/host_memory.c), and this script rewrites the guest's
assembly (after tools/android_asm_convert.py) so that every access goes
there:

- x28 holds B in guest code and x27 is a scratch register; the compiler
  never allocates either (-ffixed-x27 -ffixed-x28), and the host sets x28
  whenever it enters guest code (port/macos/host/host_thread.c);
- a load or store through a register r becomes one through B + (r's low
  32 bits): ``[x0]`` turns into ``[x28, w0, uxtw]`` where the instruction
  has that addressing mode, and ``add x27, x28, w0, uxtw`` followed by the
  access through x27 where it does not (immediate offsets, pairs,
  exclusives, vector structures);
- an indirect branch through r goes to B + (r's low 32 bits) likewise.

The guest's registers then hold guest addresses, except for sp and x29,
which are real (host) addresses inside the guest's 4 GB: the stack pointer
and the frame records that the hardware, the host and the guest's own
frame walker use. Because B is 4 GB-aligned, the low 32 bits of each such
address are the guest address of the same byte, so 32-bit arithmetic and
comparisons on them (w29) give the guest's answer. So that no host address
escapes into an ordinary register (the compiler assumes every pointer's
upper half is zero, and may compare or widen pointers as 64-bit values), a
value computed from sp or x29 (``add x8, sp, #16``), and an adrp or adr
result, is zero-extended after the instruction that makes it. Accesses
through sp and x29 themselves stay as they are, and so does a write of sp
or x29 from either; a write of sp from any other register adds B. (x30 is
no exception: return addresses are host addresses, but once a function has
saved its own the compiler uses x30 as an ordinary register, and the
return address it reloads only goes back into the program counter.)

Usage: guest_asm_rebase.py input.s output.s (ELF assembly)
"""

import re
import sys
from typing import List, Optional, Tuple

BASE = "x28"
SCRATCH = "x27"
SCRATCH_W = "w27"

GPR = re.compile(r"^([xw])([0-9]|[12][0-9]|30)$")

# loads and stores with the [Xn, Wm, uxtw] addressing mode
REGISTER_OFFSET = {
    "ldr", "str", "ldrb", "strb", "ldrh", "strh", "ldrsb", "ldrsh", "ldrsw", "prfm",
}

# instructions that never take a memory operand or change control flow
# through a register, so need no attention unless they read sp or x29
BRANCHES = {"b", "bl", "cbz", "cbnz", "tbz", "tbnz", "ret"}
COMPARES = {"cmp", "cmn", "tst", "ccmp", "ccmn"}


class RebaseError(Exception):
    pass


def split_operands(text: str) -> List[str]:
    parts: List[str] = []
    depth = 0
    current = ""
    for ch in text:
        if ch in "[{":
            depth += 1
        elif ch in "]}":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        parts.append(current.strip())
    return parts


def register_number(operand: str) -> Optional[int]:
    """the number of a general register operand (x0-x30, w0-w30), or None"""
    m = GPR.match(operand.strip())
    return int(m.group(2)) if m else None


def is_host_register(operand: str) -> bool:
    """sp and x29 hold host addresses (the 64-bit names: their low halves,
    wsp and w29, are the guest's)"""
    return operand.strip() in ("sp", "x29")


def immediate(text: str) -> int:
    text = text.strip()
    if text.startswith("#"):
        text = text[1:]
    return int(text, 0)


def add_immediate(register: str, value: int) -> str:
    if value >= 0:
        return f"\tadd {register}, {register}, #{value}"
    return f"\tsub {register}, {register}, #{-value}"


class Rebaser:
    def __init__(self, lines: List[str]):
        self.lines = lines
        self.out: List[str] = []

    def emit(self, text: str) -> None:
        self.out.append(text)

    # ---------- memory operands

    def memory(self, mnemonic: str, operands: List[str], line: str) -> None:
        index = next(i for i, op in enumerate(operands) if op.startswith("["))
        address = operands[index]
        writeback = address.endswith("!")
        if writeback:
            address = address[:-1]
        if not address.endswith("]"):
            raise RebaseError("unrecognised memory operand: " + line)
        inner = split_operands(address[1:-1])
        post = operands[index + 1:]
        base = inner[0]
        if base in ("sp", "x29"):
            if base == "x29" and (writeback or post):
                raise RebaseError("frame pointer written back: " + line)
            self.emit(line)
            return
        n = register_number(base)
        if n is None or not base.startswith("x"):
            raise RebaseError("unrecognised base register: " + line)
        if n in (27, 28):
            raise RebaseError("reserved register used as a base: " + line)
        # (a prefetch is only a hint)
        if mnemonic in ("prfm", "prfum"):
            return
        data = operands[:index]
        if any(register_number(op) in (27, 28) for op in data):
            raise RebaseError("reserved register used: " + line)
        w = f"w{n}"
        regoffset = mnemonic in REGISTER_OFFSET

        def access(address_text: str) -> str:
            return f"\t{mnemonic} {', '.join(data + [address_text])}"

        if len(inner) >= 2 and inner[1] in ("wzr", "xzr"):
            # (an index of zero: the base alone)
            inner = inner[:1]
        if len(inner) == 1 and not writeback and not post:
            # [xN]
            if regoffset:
                self.emit(access(f"[{BASE}, {w}, uxtw]"))
            else:
                self.emit(f"\tadd {SCRATCH}, {BASE}, {w}, uxtw")
                self.emit(access(f"[{SCRATCH}]"))
            return
        if len(inner) == 1 and post:
            # [xN], #imm or [xN], xM: access, then advance
            if len(post) != 1:
                raise RebaseError("unrecognised post-index: " + line)
            if regoffset:
                self.emit(access(f"[{BASE}, {w}, uxtw]"))
            else:
                self.emit(f"\tadd {SCRATCH}, {BASE}, {w}, uxtw")
                self.emit(access(f"[{SCRATCH}]"))
            if post[0].startswith("#"):
                self.emit(add_immediate(base, immediate(post[0])))
            elif register_number(post[0]) is not None:
                self.emit(f"\tadd {base}, {base}, {post[0]}")
            else:
                raise RebaseError("unrecognised post-index: " + line)
            return
        if len(inner) == 2 and (inner[1].startswith("#") or inner[1].startswith(":")) and not post:
            offset = inner[1]
            if writeback:
                # [xN, #imm]!: advance, then access
                self.emit(add_immediate(base, immediate(offset)))
                if regoffset:
                    self.emit(access(f"[{BASE}, {w}, uxtw]"))
                else:
                    self.emit(f"\tadd {SCRATCH}, {BASE}, {w}, uxtw")
                    self.emit(access(f"[{SCRATCH}]"))
                return
            self.emit(f"\tadd {SCRATCH}, {BASE}, {w}, uxtw")
            self.emit(access(f"[{SCRATCH}, {offset}]"))
            return
        if len(inner) in (2, 3) and not writeback and not post:
            # register offset: [xN, xM{, lsl #s}] or [xN, wM, (u|s)xtw {#s}]
            index_register = inner[1]
            extend = inner[2] if len(inner) == 3 else ""
            if register_number(index_register) is None:
                raise RebaseError("unrecognised register offset: " + line)
            if index_register.startswith("w") and not extend:
                raise RebaseError("unrecognised register offset: " + line)
            if index_register.startswith("w") and extend.split()[0] not in ("uxtw", "sxtw"):
                raise RebaseError("unrecognised register offset: " + line)
            if index_register.startswith("x") and extend and extend.split()[0] not in ("lsl", "sxtx", "uxtx"):
                raise RebaseError("unrecognised register offset: " + line)
            self.emit(f"\tadd {SCRATCH}, {base}, {index_register}{', ' + extend if extend else ''}")
            if regoffset:
                self.emit(access(f"[{BASE}, {SCRATCH_W}, uxtw]"))
            else:
                self.emit(f"\tadd {SCRATCH}, {BASE}, {SCRATCH_W}, uxtw")
                self.emit(access(f"[{SCRATCH}]"))
            return
        raise RebaseError("unrecognised memory operand: " + line)

    # ---------- instructions

    def instruction(self, line: str) -> None:
        text = line.strip()
        parts = text.split(None, 1)
        mnemonic = parts[0].lower()
        operands = split_operands(parts[1]) if len(parts) > 1 else []

        if any(op.startswith("[") for op in operands):
            self.memory(mnemonic, operands, line)
            return
        if any(register_number(op) in (27, 28) for op in operands):
            raise RebaseError("reserved register used: " + line)

        # control flow through a register
        if mnemonic in ("br", "blr") or (mnemonic == "ret" and operands and operands[0] != "x30"):
            target = operands[0]
            n = register_number(target)
            if n is None or not target.startswith("x"):
                raise RebaseError("unrecognised branch target: " + line)
            self.emit(f"\tadd {SCRATCH}, {BASE}, w{n}, uxtw")
            self.emit(f"\t{mnemonic} {SCRATCH}")
            return
        if mnemonic in BRANCHES or mnemonic.startswith("b."):
            self.emit(line)
            return

        # cache maintenance by address
        if mnemonic in ("dc", "ic") and len(operands) == 2:
            n = register_number(operands[1])
            if n is None:
                raise RebaseError("unrecognised cache operation: " + line)
            self.emit(f"\tadd {SCRATCH}, {BASE}, w{n}, uxtw")
            self.emit(f"\t{mnemonic} {operands[0]}, {SCRATCH}")
            return

        if not operands:
            self.emit(line)
            return
        destination = operands[0]
        sources = operands[1:]
        reads_host = any(is_host_register(op.split()[0]) for op in sources if op)

        if mnemonic in COMPARES:
            if any(is_host_register(op.split()[0]) for op in operands if op):
                raise RebaseError("64-bit comparison with a host address: " + line)
            self.emit(line)
            return

        # writes of the stack and frame pointers
        if destination in ("sp", "x29"):
            if reads_host:
                self.emit(line)
                return
            if destination == "x29":
                raise RebaseError("frame pointer set from a guest value: " + line)
            # sp from another register: compute, then add the base
            self.emit(f"\t{mnemonic} {', '.join([SCRATCH] + sources)}")
            self.emit(f"\tadd sp, {BASE}, {SCRATCH_W}, uxtw")
            return
        if destination in ("wsp", "w29"):
            raise RebaseError("32-bit write of a host register: " + line)

        self.emit(line)
        n = register_number(destination)
        if n is None or n == 30:
            return
        if destination.startswith("x") and (mnemonic in ("adrp", "adr") or reads_host):
            # a host address in an ordinary register: keep its guest half
            self.emit(f"\tmov w{n}, w{n}")

    def peephole_adrp(self, index: int) -> bool:
        """adrp xN followed by a load into xN through [xN, :lo12:...]: the
        host address is used once, as is, and dies; True if handled"""
        line = self.lines[index]
        m = re.match(r"^\s*adrp\s+(x\d+)\s*,", line)
        if not m or index + 1 >= len(self.lines):
            return False
        register = m.group(1)
        following = self.lines[index + 1].strip()
        parts = following.split(None, 1)
        if len(parts) != 2 or not parts[0].lower().startswith("ldr"):
            return False
        operands = split_operands(parts[1])
        if len(operands) != 2 or not operands[1].startswith("[") or operands[1].endswith("!"):
            return False
        inner = split_operands(operands[1][1:-1])
        if len(inner) != 2 or inner[0] != register or not inner[1].startswith(":lo12:"):
            return False
        destination = register_number(operands[0])
        if destination is None or f"x{destination}" != register:
            return False
        self.emit(line)
        self.emit(self.lines[index + 1])
        return True

    def run(self) -> str:
        index = 0
        while index < len(self.lines):
            line = self.lines[index]
            stripped = line.strip()
            try:
                if not stripped or stripped.startswith(".") or stripped.endswith(":") or stripped.startswith("#") \
                        or stripped.startswith("/*") or stripped.startswith("//"):
                    self.emit(line)
                elif self.peephole_adrp(index):
                    index += 1
                else:
                    self.instruction(line)
            except RebaseError as error:
                raise RebaseError(f"line {index + 1}: {error}") from None
            index += 1
        return "\n".join(self.out) + "\n"


def rebase(text: str) -> str:
    return Rebaser(text.split("\n")).run()


def main() -> None:
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(2)
    with open(sys.argv[1], "r", encoding="utf-8", errors="surrogateescape") as f:
        text = f.read()
    try:
        result = rebase(text)
    except RebaseError as error:
        print(f"{sys.argv[1]}: {error}", file=sys.stderr)
        sys.exit(1)
    with open(sys.argv[2], "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(result)


if __name__ == "__main__":
    main()
