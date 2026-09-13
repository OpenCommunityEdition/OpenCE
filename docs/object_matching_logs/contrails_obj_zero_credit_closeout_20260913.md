# `contrails.obj` zero-credit closeout (2026-09-13)

## Result

This pass adds no strict-exact function and no matched byte. The isolated final
gate remains 17/19 exact functions, with no unwritten functions. The two
remaining functions are credible semantic reconstructions, but are parked
fail-closed as `unclassified` fuzzy bodies at zero credit:

| Function | Target meaningful / padded | Candidate padded | Relocations | Target / candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_contrail_scale_random_value` | 102 / 112 | 96 | 2 / 2 | `45251a9f1f22cc5ea22d197f1e307f2d1b22d9d1de67c2c5566242dc17f150dd` / `e6fd923e93659df5b7bba6bf84c59125de7e7ce4386fc0852f56314214dacb17` |
| `_contrail_add_points` | 945 / 960 | 944 | 28 / 28 | `e91892ea5670bbef2d3068f01f7856618d7f5a092927f0058522ac6121b6b3cd` / `4e3e3d2050699c4809f2cd048d811b88adef969910dcb44e660a67dd9d4fdc12` |

The residual boundary is therefore 1,047 meaningful target bytes (1,072
padded). The canonical report remains 3,173/4,220 exact code bytes and 448/448
exact data bytes. No completion label, configuration edit, status exception,
or semantic credit is warranted.

## Retained source-authentic correction

The only retained source changes correct the bit-index ABI from `char` to
`short` for `contrail_scale_value` and `contrail_scale_random_value`, and place
the latter external prototype beside the former in its genuine owner,
`source/effects/contrails.h`. The database-backed independent source lift uses
`int16_t` for both bit-index parameters, and its `contrail_add_points` caller
declares the random-scale helper with that same type. The focused consumer
gates prove the header correction is neutral to every inherited exact
function.

`contrail_add_points` remains `static` and private to `contrails.c`. Exporting
it through the header was rejected: it changed calling convention/code shape
and regressed three inherited exact callers (17 exact to 14). No unrelated
definition header is touched.

## Residual verdict

`_contrail_scale_random_value` has the authenticated two-bit scaling behavior
and the same two relocation identities as the target. Its target calls occur
eight bytes later and the compiler chooses a different bit-index/register
schedule. Natural byte/short forms, helper composition, casts, and ordinary
temporary layouts did not close the 16 padded-byte gap; forcing the target
shape would require source-incredible scheduling.

`_contrail_add_points` preserves the expected typed tag/object access, marker
lookup, random cone velocity, allocation/linking, scenario-location work, and
all 28 relocation identities. The remaining 16 padded-byte difference is a
broad stack/register schedule mismatch rather than an isolated missing source
operation. The existing direct interpolation and
`scenario_location_from_line` schedule is retained; `_point_from_line3d` is
not emitted.

Both bodies stay readable C with semantic names and explicit returns. No
assembly, `register`, `volatile`, barrier, intrinsic, attribute, raw offset,
type pun, synthetic caller, forced inline, object patch, or byte-forcing
expression is introduced. Reopen only on authoritative same-toolchain source
or an exact compatible donor.

## Focused validation

- PID-isolated `gate.py source/effects/contrails --all
  --forbid-emitted-symbol _point_from_line3d` passes at 17 exact, 2 residual,
  0 unwritten.
- Direct header consumers remain unchanged: `render_contrails` 6/6 exact,
  `objects` 121/121 exact, and `game` retains its pre-existing 25 exact / 2
  residual boundary.
- The forbidden-symbol guard passes for all four focused gates.
- No global configure, Ninja run, config edit, commit, or push was performed.
