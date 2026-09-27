"""H1: list every access to .bss owner symbols in a COFF object: section alignment,
symbols, and for each DIR32 relocation into a .bss symbol the instruction, the
addend (displacement), and the access width. Read-only."""
import struct, sys, os, collections
sys.path.insert(0, r"C:\halo-worktrees\claude-compiler-application-20260925\tools\campaign")
import coffio, capstone
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True

def align_of(chars):
    a = (chars >> 20) & 0xF
    return 0 if a == 0 else 1 << (a - 1)

def main(path, only=None):
    o = coffio.load(path)
    print("OBJECT", path)
    for s in o.sections:
        if s["name"].startswith(".bss") or s["name"].startswith(".data") or (only and s["name"] == only):
            print("  SEC %2d %-8s size=0x%x chars=0x%08x align=%d" % (s["index"], s["name"], s["size"], s["chars"], align_of(s["chars"])))
    bss = {s["index"] for s in o.sections if s["name"].startswith(".bss") or s["name"].startswith(".data")}
    syms = {sy["index"]: sy for sy in o.symbols}
    print("  DATA SYMBOLS:")
    for sy in o.symbols:
        if sy["sec"] in bss:
            print("    #%d %-50s sec=%d value=0x%x storage=%d type=0x%x" % (sy["index"], sy["name"], sy["sec"], sy["value"], sy["storage"], sy["type"]))
    # function section owners
    owners = {}
    for sy in o.symbols:
        if sy["sec"] > 0 and sy["value"] == 0 and sy["storage"] in (2, 3) and not sy["name"].startswith((".", "$")):
            s = o.sections[sy["sec"] - 1]
            if s["name"].startswith(".text"):
                owners.setdefault(sy["sec"], sy["name"])
    rows = []
    for s in o.sections:
        if not s["name"].startswith(".text"):
            continue
        code = s["data"]
        insns = list(md.disasm(code, 0))
        for va, symidx, typ in s["relocs"]:
            sy = syms[symidx]
            if sy["sec"] not in bss:
                continue
            addend = struct.unpack_from("<l", code, va)[0]
            ins = next((i for i in insns if i.address <= va < i.address + i.size), None)
            width = None
            if ins is not None:
                for op in ins.operands:
                    if op.type == capstone.x86.X86_OP_MEM and op.mem.disp == addend:
                        width = op.size
                txt = "%s %s" % (ins.mnemonic, ins.op_str)
            else:
                txt = "?"
            rows.append((owners.get(s["index"], "?"), ins.address if ins else va, sy["name"], sy["value"] + addend, addend, width, txt))
    for r in rows:
        print("  %-36s +%04x  %-28s off=0x%04x (addend 0x%x) w=%s  %s" % r)
    return rows

if __name__ == "__main__":
    main(sys.argv[1])
