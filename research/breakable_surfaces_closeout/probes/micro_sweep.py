"""Enumerate cross_product3d implementations against the m6 micro-probe.

m6 layout: a.i=-0x18 a.j=-0x14 a.k=-0x10 | b.i=-0xc b.j=-8 b.k=-4

Our build left-loads the two products containing a.k; January right-loads
them.  Report each variant's fld/fmul slot pairs so the rule is visible.
"""
import re
import subprocess
import sys

P = r"source\math\real_math.h"
NAMES = {-0x18: "a.i", -0x14: "a.j", -0x10: "a.k", -0xc: "b.i", -8: "b.j", -4: "b.k"}

BASE = ["\treal k = a->i*b->j - a->j*b->i;",
        "\treal j = a->k*b->i - a->i*b->k;",
        "\treal i = a->j*b->k - a->k*b->j;",
        "\tresult->i = i;",
        "\tresult->j = j;",
        "\tresult->k = k;"]

VARIANTS = {
    "c0_control": BASE,
    # name every product (changes the DAG: six named leaves, three subtractions)
    "c1_named_products": [
        "\treal ij = a->i*b->j;", "\treal ji = a->j*b->i;",
        "\treal ki = a->k*b->i;", "\treal ik = a->i*b->k;",
        "\treal jk = a->j*b->k;", "\treal kj = a->k*b->j;",
        "\tresult->i = jk - kj;", "\tresult->j = ki - ik;", "\tresult->k = ij - ji;"],
    # local copies of both vectors first
    "c2_local_copies": [
        "\treal_vector3d va = *a;", "\treal_vector3d vb = *b;",
        "\treal k = va.i*vb.j - va.j*vb.i;",
        "\treal j = va.k*vb.i - va.i*vb.k;",
        "\treal i = va.j*vb.k - va.k*vb.j;",
        "\tresult->i = i;", "\tresult->j = j;", "\tresult->k = k;"],
    # hoist every member into a named scalar
    "c3_named_members": [
        "\treal ai = a->i;", "\treal aj = a->j;", "\treal ak = a->k;",
        "\treal bi = b->i;", "\treal bj = b->j;", "\treal bk = b->k;",
        "\treal k = ai*bj - aj*bi;", "\treal j = ak*bi - ai*bk;", "\treal i = aj*bk - ak*bj;",
        "\tresult->i = i;", "\tresult->j = j;", "\tresult->k = k;"],
    # store results as computed, i first
    "c4_store_as_computed": [
        "\tresult->i = a->j*b->k - a->k*b->j;",
        "\tresult->j = a->k*b->i - a->i*b->k;",
        "\tresult->k = a->i*b->j - a->j*b->i;"],
    # k component last but stored first (mirror shape, 2 temps)
    "c5_2temp": ["\treal k = a->i*b->j - a->j*b->i;",
                 "\treal j = a->k*b->i - a->i*b->k;",
                 "\tresult->i = a->j*b->k - a->k*b->j;",
                 "\tresult->j = j;",
                 "\tresult->k = k;"],
    # negate form: write each component as -(reversed subtraction)
    "c6_negated": ["\treal k = -(a->j*b->i - a->i*b->j);",
                   "\treal j = -(a->i*b->k - a->k*b->i);",
                   "\treal i = -(a->k*b->j - a->j*b->k);",
                   "\tresult->i = i;", "\tresult->j = j;", "\tresult->k = k;"],
    # only the a->k operands hoisted
    "c7_ak_local": ["\treal ak = a->k;",
                    "\treal k = a->i*b->j - a->j*b->i;",
                    "\treal j = ak*b->i - a->i*b->k;",
                    "\treal i = a->j*b->k - ak*b->j;",
                    "\tresult->i = i;", "\tresult->j = j;", "\tresult->k = k;"],
}


def nl_of(s):
    return "\r\n" if "\r\n" in s else "\n"


def run(tag, lines):
    subprocess.run(["git", "checkout", "--", "source/math/real_math.h"], check=True)
    s = open(P, encoding="latin-1", newline="").read()
    nl = nl_of(s)
    old = nl.join(BASE)
    assert old in s, "cross block not found"
    open(P, "w", encoding="latin-1", newline="").write(s.replace(old, nl.join(lines)))
    r = subprocess.run(["python", "scratch/probelab.py", "scratch/micro.c", "_micro_m6"],
                       capture_output=True, text=True)
    if "COMPILE FAILED" in r.stdout or r.returncode != 0:
        print(f"{tag:20s} COMPILE FAILED", flush=True)
        return
    pairs, cur = [], None
    for line in r.stdout.splitlines():
        m = re.match(r'\s*[0-9a-f]+: (fld|fmul)\s+dword ptr \[ebp - (0x[0-9a-f]+)\]', line)
        if not m:
            continue
        slot = -int(m.group(2), 16)
        nm = NAMES.get(slot, hex(slot))
        if m.group(1) == 'fld':
            cur = nm
        elif cur:
            pairs.append("fld %s*%s" % (cur, nm))
            cur = None
    print(f"{tag:20s} " + " | ".join(pairs), flush=True)


if __name__ == "__main__":
    for tag in (sys.argv[1:] or list(VARIANTS)):
        run(tag, VARIANTS[tag])
    subprocess.run(["git", "checkout", "--", "source/math/real_math.h"], check=True)
    print("header restored")
