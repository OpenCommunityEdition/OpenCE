"""HCEX-attested function-static buffer x unsigned clip-flags coupling.

The previous S4 ledger establishes both facts, but its preserved candidate
still declares the buffer at file scope. This changes real storage scope,
not a decoration or a dead-name count.
"""
import json
import probe

STATIC = [("\tstruct collision_bsp *collision_bsp;", "\tstatic real_point2d decal_points2d_temp[2][12];\n\tstruct collision_bsp *collision_bsp;")]
REMOVE_FILE_STATIC = [("\nstatic real_point2d decal_points2d_temp[2][12];\n", "\n")]
UNSIGNED = [("long *clip_flags,", "unsigned long *clip_flags,"), ("long clipped_flags = 0;", "unsigned long clipped_flags = 0;")]
rows = [
    probe.run("S_storage_function_static", STATIC, "canonical", global_edits=REMOVE_FILE_STATIC),
    probe.run("U_clip_flags_unsigned", UNSIGNED, "canonical"),
    probe.run("SU_storage_unsigned", STATIC + UNSIGNED, "canonical", global_edits=REMOVE_FILE_STATIC),
]
(probe.OUT / "storage_measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
