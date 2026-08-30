"""Change WHICH variable the cross reads (use-count / CSE structure).

units.obj lesson: a "doesn't work" verdict is only valid at the use count
tested.  s_plane.n is a byte-identical copy of s_normal, so reading the
cross operands from the plane instead of the normal is semantically inert
but changes the use graph the scheduler sees.
"""
import subprocess
import sys

BASE = "scratch/witness_mirror2.c"
T = "\t"
CROSS = T*5 + "cross_product3d(&s_normal, &surface_plane.n, &t_normal);\n"
SPLANE = T*5 + "plane3d_from_point_and_normal(&s_plane, &origin, &s_normal);\n"
TPLANE = T*5 + "plane3d_from_point_and_normal(&t_plane, &origin, &t_normal);\n"
BLOCK = CROSS + "\n" + SPLANE + TPLANE

VARIANTS = {
    "f0_control": BLOCK,
    # build s_plane first, then cross from the plane's own normal
    "f1_cross_from_splane": (SPLANE + "\n"
                             + T*5 + "cross_product3d(&s_plane.n, &surface_plane.n, &t_normal);\n"
                             + TPLANE),
    # build s_plane first, cross still from s_normal
    "f2_splane_first": SPLANE + "\n" + CROSS + TPLANE,
    # cross writes straight into t_plane.n, then finish the plane
    "f3_cross_into_tplane": (CROSS.replace("&t_normal", "&t_plane.n") + "\n"
                             + SPLANE
                             + T*5 + "plane3d_from_point_and_normal(&t_plane, &origin, &t_plane.n);\n"),
}


def run(tag, block):
    w = open(BASE, encoding="latin-1").read()
    assert BLOCK in w
    w = w.replace(BLOCK, block)
    src = "scratch/witness_df_%s.c" % tag
    open(src, "w", encoding="latin-1", newline="\n").write(w)
    out = "scratch/disas_df_%s.txt" % tag
    with open(out, "w") as f:
        subprocess.run(["python", "tools/campaign/gate.py", "source/physics/breakable_surfaces",
                        "--source", src, "--fn", "_breakable_surface_effect",
                        "--disas", "_breakable_surface_effect"],
                       stdout=f, stderr=subprocess.STDOUT)
    txt = open(out, encoding="utf-8", errors="replace").read()
    if "COMPILE FAILED" in txt or "Traceback" in txt:
        print("%-22s COMPILE FAILED" % tag, flush=True)
        return
    n = sum(1 for l in txt.splitlines() if "|" in l) - 1
    print("%-22s diff-lines=%d" % (tag, n), flush=True)


for tag in (sys.argv[1:] or list(VARIANTS)):
    run(tag, VARIANTS[tag])
