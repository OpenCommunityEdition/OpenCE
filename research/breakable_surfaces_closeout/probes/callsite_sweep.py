"""Call-site variants around the four distance computations, TRUE basin.

Headers untouched (flat dot is proven tree-wide), so nothing here can
regress another object.  All earlier caller sweeps ran in the refuted
grouped-dot basin and are void; this re-opens that space honestly.
"""
import subprocess
import sys

BASE = "scratch/witness_mirror2.c"
T = "\t"

FIRST = (T*5 + "surface_bounds.x1 = plane3d_distance_to_point(&s_plane, vertex_point);\n"
         + T*5 + "surface_bounds.x0 = surface_bounds.x1;\n"
         + T*5 + "surface_bounds.y1 = plane3d_distance_to_point(&t_plane, vertex_point);\n"
         + T*5 + "surface_bounds.y0 = surface_bounds.y1;\n")
ELSE = (T*5 + "real s = plane3d_distance_to_point(&s_plane, vertex_point);\n"
        + T*5 + "real t = plane3d_distance_to_point(&t_plane, vertex_point);\n")

VARIANTS = {
    "s0_control": [],
    # t distance before s in the seed arm
    "s1_first_t_first": [(FIRST,
        T*5 + "surface_bounds.y1 = plane3d_distance_to_point(&t_plane, vertex_point);\n"
        + T*5 + "surface_bounds.y0 = surface_bounds.y1;\n"
        + T*5 + "surface_bounds.x1 = plane3d_distance_to_point(&s_plane, vertex_point);\n"
        + T*5 + "surface_bounds.x0 = surface_bounds.x1;\n")],
    # seed arm through named temps, like the else arm
    "s2_first_named": [(FIRST,
        T*5 + "real s = plane3d_distance_to_point(&s_plane, vertex_point);\n"
        + T*5 + "real t = plane3d_distance_to_point(&t_plane, vertex_point);\n"
        + "\n"
        + T*5 + "surface_bounds.x1 = s;\n"
        + T*5 + "surface_bounds.x0 = s;\n"
        + T*5 + "surface_bounds.y1 = t;\n"
        + T*5 + "surface_bounds.y0 = t;\n")],
    # x0 assigned from the call, x1 copied (reverse the copy direction)
    "s3_first_x0_first": [(FIRST,
        T*5 + "surface_bounds.x0 = plane3d_distance_to_point(&s_plane, vertex_point);\n"
        + T*5 + "surface_bounds.x1 = surface_bounds.x0;\n"
        + T*5 + "surface_bounds.y0 = plane3d_distance_to_point(&t_plane, vertex_point);\n"
        + T*5 + "surface_bounds.y1 = surface_bounds.y0;\n")],
    # else arm: t before s
    "s4_else_t_first": [(ELSE,
        T*5 + "real t = plane3d_distance_to_point(&t_plane, vertex_point);\n"
        + T*5 + "real s = plane3d_distance_to_point(&s_plane, vertex_point);\n")],
    # else arm: decl then assign (definition-position split)
    "s5_else_split": [(ELSE,
        T*5 + "real s;\n" + T*5 + "real t;\n" + "\n"
        + T*5 + "s = plane3d_distance_to_point(&s_plane, vertex_point);\n"
        + T*5 + "t = plane3d_distance_to_point(&t_plane, vertex_point);\n")],
    # both arms reversed together
    "s6_both_t_first": [
        (FIRST,
         T*5 + "surface_bounds.y1 = plane3d_distance_to_point(&t_plane, vertex_point);\n"
         + T*5 + "surface_bounds.y0 = surface_bounds.y1;\n"
         + T*5 + "surface_bounds.x1 = plane3d_distance_to_point(&s_plane, vertex_point);\n"
         + T*5 + "surface_bounds.x0 = surface_bounds.x1;\n"),
        (ELSE,
         T*5 + "real t = plane3d_distance_to_point(&t_plane, vertex_point);\n"
         + T*5 + "real s = plane3d_distance_to_point(&s_plane, vertex_point);\n")],
}


def run(tag, edits):
    w = open(BASE, encoding="latin-1").read()
    for find, repl in edits:
        assert find in w, "%s: pattern not found" % tag
        w = w.replace(find, repl)
    src = "scratch/witness_cs2_%s.c" % tag
    open(src, "w", encoding="latin-1", newline="\n").write(w)
    out = "scratch/disas_cs2_%s.txt" % tag
    with open(out, "w") as f:
        subprocess.run(["python", "tools/campaign/gate.py", "source/physics/breakable_surfaces",
                        "--source", src, "--fn", "_breakable_surface_effect",
                        "--disas", "_breakable_surface_effect"],
                       stdout=f, stderr=subprocess.STDOUT)
    txt = open(out, encoding="utf-8", errors="replace").read()
    if "COMPILE FAILED" in txt or "Traceback" in txt:
        print("%-20s COMPILE FAILED" % tag, flush=True)
        return
    n = sum(1 for l in txt.splitlines() if "|" in l) - 1
    print("%-20s diff-lines=%d" % (tag, n), flush=True)


for tag in (sys.argv[1:] or list(VARIANTS)):
    run(tag, VARIANTS[tag])
