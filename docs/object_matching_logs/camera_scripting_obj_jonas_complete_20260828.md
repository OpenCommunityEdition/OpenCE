# `camera_scripting.obj` complete

This ledger records the strict closeout of
`source/camera/camera_scripting.obj`. The retained legal C advances the object
from 6/13 to 13/13 exact functions, recovers every target-owned non-code
section, and changes the unit status from `NonMatching` to `Matching`.

## Result

- Integration base: `793bc4b6f5da7e1ffd9097cff7d60349d5f1fddd`.
- Target object SHA-256:
  `5be1b15bb8f628bd795b912bbea2f45b89cedd4b0f69b2ec99247a33ba408075`.
- Preserved baseline object SHA-256:
  `2a12623d00f5688e62319b0c257e8e4cd4311712091960dfd6544a06cab3a6d1`.
- Final candidate and preserved final-audit object SHA-256:
  `7f548f13941d18ff3dd23762d61516c58e6060039ddb65817dfddf91f3c604c3`.
- Source blob: `e483b46fb5590de28539e4b79460b67541fdb4e3` to
  `eeade548401b41f230740225d2af10e5e12d320b`.
- Function census: 6/13 to 13/13.
- Meaningful code: 108/2,534 to 2,534/2,534 bytes, a 2,426-byte gain.
- Padded code: 176/2,656 to 2,656/2,656 bytes, a 2,480-byte gain.
- Newly recovered function relocations: 142. Final function-relocation total:
  151.
- Target-owned non-code: all three owners exact, totaling 166 logical bytes
  in 172 allocated bytes.

The exact-set regression census reports the seven functions below as newly
exact. Its only apparent lost name is the synthetic `.text` section marker,
which is not a function; all six inherited function owners remain exact.

| Newly exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_scripted_camera_object_is_first_person_camera` | 43 | 48 | 3 | `b2cac7d07e9f31c29a2fe6ca5fca3d15048439823fd8fa4f5211eac2c2bfd5e2` |
| `_scripted_camera_set_first_person` | 51 | 64 | 5 | `68e72f1f8da999bc527c77e6cf04d86627e8943451d82b017a4b751c8e773e02` |
| `_scripted_camera_set_dead` | 51 | 64 | 5 | `475a971a7d81d730e7ac972cc2cec87f972806ac58eb3faf0169c5403681cc47` |
| `_scripted_camera_set_animation` | 206 | 208 | 11 | `e7b293ed2840eeea6757690ea0864115b1bdc5566a34c29ea1bd07cd9daae45d` |
| `_scripted_camera_set` | 211 | 224 | 18 | `80b8ee322caa9fecd2b3eb2f4ad77df884c7ad61d8fde75ecf736dec07caaf0a` |
| `_scripted_camera_set_camera_point_relative` | 207 | 208 | 18 | `c20595ec0eb25ab612e96bdca1cb8fa667729e7fda099fb7b7fd4cf1352baceb` |
| `_scripted_camera_update` | 1,657 | 1,664 | 82 | `cb504e5b44213920854f97d593ab2361bda073fcb0ffef531347b0c4b8565e06` |
| **Closeout delta** | **2,426** | **2,480** | **142** | |

The six inherited exact functions are `_scripted_camera_enable`,
`_scripted_camera_set_absolute`,
`_scripted_camera_set_camera_point_absolute`,
`_scripted_camera_next_camera_point`,
`_scripted_camera_object_relative_to`, and `_scripted_camera_time`. Their
individual proof is retained in
`docs/object_matching_logs/camera_scripting_obj_jonas_leaf_wave_20260820.md`.

## Target-owned data

The source now owns a typed 0x40-byte `struct scripted_camera_globals`. Its
field layout is proved by the complete call and relocation graph rather than
raw-address access. The two source strings reproduce the target's COMDAT
owners. All three hardened comparisons report `all_equal: true` and zero
relocations.

| Owner | Logical bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `_data_002dcb60` | 64 | `44bbb7b1f0ad01b6e22697ef8ed246b50458278e10d99813d5084ebeeb3556fb` |
| `??_C@_0DN@JNLEKBID@cannot?5set?5first?5person?5camera?5o@` | 61 | `9be3896bb597352515045201d5e16f09ac0ebc22a1ab4f8143c4e622d4eef530` |
| `??_C@_0CJ@GLCFCHKA@c?3?2halo?2SOURCE?2camera?2camera_scr@` | 41 | `d7802a234cdeff892231ef0e97951611a370cad0eb192687969ffbfc66ac0b2b` |

## Reconstruction evidence

The primary semantic donor was the authenticated HCEA camera reconstruction
at `work/halocea-reference/src/scripted_camera_update.c`, with its related
`camera_script_globals.h`, `camera_script_mode.h`,
`data/camera_script_globals.c`, and small-function files. The donor supplied
behavior and type intent; the January object remained the sole authority for
source shape, storage, call order, bytes, and relocations. The HCEA reference
commit used by the earlier wave was
`c168af2e747d3095d9a29418ae401f3a39544863`.

Additional semantic checks came from
`research-cache/stian-halo-current-20260827/src/halo/objects/objects.c`, the
already reconstructed `source/camera/dead_camera.c`, and the prior camera leaf
ledger. Repository `CLAUDE.md`, campaign/tooling documentation, object ledgers,
and the available Claude/Fable work records were reviewed before editing. The
latest active Claude/Fable lane was `units.obj`; this camera unit was therefore
unclaimed and was kept isolated from agent-owned work.

The decisive update-body source levers were the typed point/animation/
first-person/dead switch, a `long` animation frame count, the inline clamped
frame argument to `animation_get_root_matrix`, the staged rotated-offset
accumulation, field-of-view assignment before root position, and the target
validation expression and source line. The retained source uses typed object,
scenario, animation, camera, and tag access throughout.

## Comparator classification

Hardened `tools/coff_compare.py` proves all 1,664 padded bytes and all 82
semantic relocations of `_scripted_camera_update` exact. Ordinary objdiff
reports 98.14433% because its local jump-table relocation-placeholder encoding
does not canonicalize to the target representation. This is the same reviewed
Class E reporting case as the completed recorded-animation dispatcher.

`config/semantic_matches.json` therefore contains a narrow, named bridge for
this one function. It grants no byte credit by assertion: semantic progress
re-runs the hardened COFF proof on every build. No compiler flag, symbol map,
target object, comparator rule, parked entry, or assembly implementation was
changed.

## Preserved experiments

- `build/audit/camera_scripting_six_small_first_shot_20260828.obj`:
  `7f9f981f45e96bbab3e93b0b0f2a7373bc1f3621039e6b5bc3bffaec07d3842a`.
- `build/audit/camera_scripting_six_small_exact_20260828.obj`:
  `bcb40ec3f4e49dc14338877b5e4ecd8ed3fb3de3a368b5faf785e06a52d5ea63`.
- `build/audit/camera_scripting_update_first_shot_20260828.obj` preserves the
  first complete update candidate before bounded source-shape convergence.
- `build/audit/camera_scripting_complete_20260828.obj` is byte-identical to
  the final production candidate object.

## Validation

- Production XDK 3911 `/O2 /Oy- /DDEBUG /Dxbox` syntax gate: pass.
- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened whole-unit gate: 13 exact, zero residual, zero unwritten.
- Camera board: 2,656/2,656 padded bytes and 13/13 functions.
- Exact-set regression census: seven newly exact functions and zero lost real
  functions.
- Semantic progress: 470 units, 4,610 functions evaluated, 4,479 semantic
  exact, 4,506 accepted exact, and zero unit errors.
- Object-admission audit: zero candidates, zero contradicted, zero revoked.
- Strict campaign board: 271/619 objects, 4,460/8,246 functions, and
  598,903/1,922,669 padded code bytes. Parked and assembly implementations are
  excluded.
- Parked audit: 13 active, zero stale, zero invalid.
- Full Python tooling suite: 205 tests pass.
- `git diff --check` and the banned-construct scan: pass.

All no-argument functions use explicit `void`; parameters are one per line;
every function has an explicit terminal return. The retained C contains no
assembly, volatile scheduling device, register keyword, pragma, intrinsic,
optimizer barrier, undefined behavior, raw-address field access, or byte
forcing.
