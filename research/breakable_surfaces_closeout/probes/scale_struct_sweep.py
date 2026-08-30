"""scale_vector3d STRUCTURE (not store order) -- the one common producer.

All five divergent sites read s_normal's three members, which scale_vector3d
writes.  A change that is byte-neutral inside normalize3d but shifts those
values' definition structure is exactly the context lever class.
"""
import hashlib
import subprocess
import sys

P = r"source\math\real_math.h"
SRC = "scratch/witness_mirror2.c"
BASE = ["\tresult->i = c*a->i;", "\tresult->j = c*a->j;", "\tresult->k = c*a->k;"]

VARIANTS = {
    "v0_control": BASE,
    # operand order within each product
    "v1_a_first": ["\tresult->i = a->i*c;", "\tresult->j = a->j*c;", "\tresult->k = a->k*c;"],
    # named temps, then stores (definition-position split)
    "v2_temps": ["\treal i = c*a->i;", "\treal j = c*a->j;", "\treal k = c*a->k;",
                 "\tresult->i = i;", "\tresult->j = j;", "\tresult->k = k;"],
    # read the source vector into locals first
    "v3_read_first": ["\treal ai = a->i;", "\treal aj = a->j;", "\treal ak = a->k;",
                      "\tresult->i = c*ai;", "\tresult->j = c*aj;", "\tresult->k = c*ak;"],
    # a local copy of the whole source vector
    "v4_local_copy": ["\treal_vector3d v = *a;",
                      "\tresult->i = c*v.i;", "\tresult->j = c*v.j;", "\tresult->k = c*v.k;"],
}


def run(tag, lines):
    subprocess.run(["git", "checkout", "--", "source/math/real_math.h"], check=True)
    s = open(P, encoding="latin-1", newline="").read()
    nl = "\r\n" if "\r\n" in s else "\n"
    old = nl.join(BASE)
    assert old in s, "scale block not found"
    open(P, "w", encoding="latin-1", newline="").write(s.replace(old, nl.join(lines)))
    h = hashlib.md5(open(P, "rb").read()).hexdigest()[:8]
    out = "scratch/disas_sc_%s.txt" % tag
    with open(out, "w") as f:
        subprocess.run(["python", "tools/campaign/gate.py", "source/physics/breakable_surfaces",
                        "--source", SRC, "--fn", "_breakable_surface_effect",
                        "--disas", "_breakable_surface_effect"],
                       stdout=f, stderr=subprocess.STDOUT)
    txt = open(out, encoding="utf-8", errors="replace").read()
    if "COMPILE FAILED" in txt or "Traceback" in txt:
        print("%-16s header=%s COMPILE FAILED" % (tag, h), flush=True)
        return
    n = sum(1 for l in txt.splitlines() if "|" in l) - 1
    print("%-16s header=%s diff-lines=%d" % (tag, h, n), flush=True)


for tag in (sys.argv[1:] or list(VARIANTS)):
    run(tag, VARIANTS[tag])
subprocess.run(["git", "checkout", "--", "source/math/real_math.h"], check=True)
print("header restored")
