"""H1 lab harness (never lands): compile a generated TU with VC7 + render_debug's flags and report the
.bss/.data layout of its file-scope statics.

    import bsslab
    r = bsslab.compile_tu([(name, decl_fmt, init_or_None), ...], tag)
    r = {'sections': [(idx, name, size, align)], 'syms': {name: (secname, secidx, value)}, 'order': [...]}

decl_fmt uses {n} for the name, e.g. "short {n}" or "struct lab_entry {n}[512]". init is the initialiser text
('0', '{ 0 }', 'FALSE') or None for an uninitialised static.
"""
import os, re, subprocess, struct, sys, random, string

ROOT = r"C:\halo-worktrees\claude-compiler-application-20260925"
sys.path.insert(0, os.path.join(ROOT, "tools", "campaign"))
import coffio

CL = os.path.join(ROOT, "xbox", "bin", "vc7", "CL.Exe")
OUT = os.path.join(ROOT, "scratch", "campaign", "workers", "H1", "lab")
os.makedirs(OUT, exist_ok=True)
FLAGS = ["/nologo", "/c", "/O2", "/Oy-", "/DDEBUG", "/Dxbox"]

PRELUDE = """typedef unsigned char boolean;
struct lab_entry
{
	long words[14];
};
"""


def align_of(chars):
    a = (chars >> 20) & 0xF
    return 0 if a == 0 else 1 << (a - 1)


def make_source(entries, extern_fn=True, extra=""):
    lines = [PRELUDE, extra]
    for name, decl, init, *rest in entries:
        storage = rest[0] if rest else "static "
        d = decl.format(n=name)
        lines.append("%s%s%s;" % (storage, d, "" if init is None else " = " + init))
    lines.append("")
    lines.append("long lab_touch(\n\tlong x)\n{")
    for name, decl, init, *rest in entries:
        if "[" in decl:
            if "lab_entry" in decl:
                lines.append("\t%s[x].words[0] = x;" % name)
            else:
                lines.append("\t%s[x] = (%s)x;" % (name, "char" if decl.startswith("char") else decl.split()[0]))
        else:
            lines.append("\t%s = (%s)(%s + x);" % (name, decl.split()[0], name))
    lines.append("\treturn x;\n}\n")
    return "\n".join(lines)


def compile_tu(entries, tag, extra=""):
    src = os.path.join(OUT, tag + ".c")
    obj = os.path.join(OUT, tag + ".obj")
    open(src, "w", newline="\n").write(make_source(entries, extra=extra))
    if os.path.exists(obj):
        os.remove(obj)
    r = subprocess.run([CL] + FLAGS + ["/Fo" + obj, src], capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0:
        raise RuntimeError("compile failed %s\n%s" % (tag, r.stdout[-2000:]))
    return read_obj(obj, [e[0] for e in entries])


def read_obj(obj, names):
    o = coffio.load(obj)
    secs = [(s["index"], s["name"], s["size"], align_of(s["chars"])) for s in o.sections
            if s["name"].startswith((".bss", ".data"))]
    syms = {}
    for sy in o.symbols:
        nm = sy["name"][1:] if sy["name"].startswith("_") else sy["name"]
        if nm in names and sy["sec"] > 0:
            s = o.sections[sy["sec"] - 1]
            syms[nm] = (s["name"], sy["sec"], sy["value"], sy["storage"])
        elif nm in names and sy["sec"] == 0:
            syms[nm] = ("COMMON", 0, sy["value"], sy["storage"])
    order = sorted(syms, key=lambda n: (syms[n][1], syms[n][2]))
    return {"sections": secs, "syms": syms, "order": order}


def rand_name(rng, n=None):
    n = n or rng.randint(4, 14)
    first = rng.choice(string.ascii_lowercase)
    return first + "".join(rng.choice(string.ascii_lowercase + string.digits + "_") for _ in range(n - 1))
