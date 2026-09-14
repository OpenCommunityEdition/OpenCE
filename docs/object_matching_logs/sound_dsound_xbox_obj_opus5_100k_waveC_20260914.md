# `sound_dsound_xbox.obj` Opus 5 100K consolidated, wave C (2026-09-14)

## Scope and provenance

- Translation unit `source/sound/sound_dsound_xbox.c`. Target
  `build/split/source/sound/sound_dsound_xbox.obj` (January 2002 Xbox debug
  build, csplit).
- Compiler: XDK 3911 `CL.Exe` 13.00.9254. Unit flags: `/O2 /Oy- /DDEBUG /Dxbox`
  with the repository include graph. No flag, header, config or park changes.
- Lane `opus/100k-consolidated-20260914`, wave C worker. Scope was this one `.c`
  file.
- Evidence read before any experiment:
  - the previous wave's worker notes (`scratch/workers/sound_dsound_xbox.md`);
  - ledgers `_opus5_fresh_graphs_20260914`, `_opus_reconciliation_20260903` and
    `_opus5_100k_20260914`;
  - the fresh-graphs probe specs (`p1.py` comma probe, `cm4`-`cm6`);
  - the XDK `DSound.h` in the tree, which uses plain STDAPI prototypes and
    object-like `IDirectSound3DListener_*` macros;
  - Stian `sound_dsound_xbox.c:816-870`;
  - HCEA `set_listener_properties_dsound.cpp`. It is FMOD, so it gives no
    spelling evidence;
  - the January map atlas.
- `git log --all` for the file: e481d5142, 0c18c70e0, 608c66380, b654ee4f9,
  b8c442f9d, f54ec267f, ecd0fff2d, e830efbd7. None of these has an exact body
  for either landed row.

## Validated baseline

Gate at lane HEAD, point guard passed: **35 exact / 4 residual / 0 unwritten**
of 39 functions. The unit has no parked entries.

| Function | Target size / relocs / nsha16 | Base size / relocs / nsha16 | First divergence |
| --- | --- | --- | --- |
| `_dsound_set_listener_properties` | 816 / 63 / `2c07c059201ef12a` | 784 / 63 / `39384b94a33e124c` | +0x5b: the 2nd and 3rd coordinate arguments go through the x87 |
| `_channel_set_location` | 752 / 43 / `b509812c41541745` | 720 / 43 / `88e79a8ebcc18a51` | +0xee: same family; plus the obstruction compare load order and the tail stores |
| `_dsound_initialize` | 720 / 52 / `d73c7ae0918c2fc4` | 720 / 52 / `11e94e43be496945` | +0x29f: which of four identical failure tails survives cross-jumping |
| `_channel_queue_packet` | 640 / 37 / `eedc03a5428064c8` | 640 / 37 / `b864c6f9b8f3ea40` | ebx/edi register colour tie |

## Final state

Real-file gate with `--forbid-emitted-symbol _point_from_line3d` (passed):
**37 exact / 2 residual / 0 unwritten**. The full `--all` listing diff against
baseline shows only the two landed rows changing, both residual to EXACT.

## The paren law (new, measured; applies across the lane)

**Law.** VC7 13.00.9254 `/O2` treats a plain `real` lvalue argument or operand
differently when it is written inside parentheses that enclose the whole
expression. Examples: `f(p, (s->v.y), (s->v.z))` or `a-(s->b)`.

- **Arguments.** The parenthesised value is pushed through the x87:
  `fld [m]; sub esp,8; fstp [esp+4]; fld [m]; fstp [esp]`, or
  `fld; push ecx; fstp [esp]`. Without the parentheses VC7 copies it as an
  integer (`mov reg,[m]; push reg`).
- **Arithmetic operands.** A parenthesised leaf is evaluated first. For
  example, `a-(s->b)` compiles to `fld [s->b]; fsubr [a]` rather than
  `fld [a]; fsub [s->b]`.

The effect comes from the parentheses themselves:

- Parentheses around only part of the argument do nothing: `(s->v).y`,
  `(s)->v.y`, `(&s->v)->y`.
- Casts without parentheses fold away: `(real)P`, `(FLOAT)P`,
  `(real)(double)P`.
- Every earlier probe that "worked" was parenthesised: `(0, P)`,
  `(real)(double)(P)` and `(P)*1.f`. The comma or cast inside was irrelevant.
- The leading float of a run of float arguments usually stays an integer copy.
  Which argument keeps the integer copy depends on scheduling; see micro-lab
  `m11.c` `s1` versus `s4`.

Where this was measured:

- Micro TUs in `scratch/workersC/sound_dsound_xbox/`: `m8.c` (`k0`-`k3`),
  `m9.c` (`m0`-`m5`), `m10.c` (`n1`-`n9`) and `m11.c` (`s1`-`s4`). Compile and
  dump them with `lab.py`.
- The full TU:
  - `(P)` alone → 800;
  - `(real)(P)` → 800;
  - `(real)P` → 784;
  - parentheses on all 12 vector arguments → listener EXACT.
- **Transfer check in another object** (scratch copy only; `players.c` itself
  was not touched): writing `(device->object.bounding_sphere_radius)` as the
  float argument of `fast_vector_intersects_sphere` makes
  `players::_player_examine_nearby_device` strict EXACT. That row is 176 bytes;
  the lane head builds 160. Probe: `xfer/players_paren.c`.
- **Admissibility caveat.** Bare parentheses at a call site do not read as
  natural code. The authentic explanation is a function-like macro whose
  expansion parenthesises its parameters, as Bungie's macros do. Each owner has
  to find that macro, as below.

## Accepted controls (landed)

1. **`realcmp_epsilon(a, b, epsilon)`**, defined as
   `(fabs((a)-(b))<(epsilon))` and TU-local. It replaces all 31
   `!(epsilon>fabs(a-b))` deferred-parameter change tests in
   `dsound_set_listener_properties`, `channel_set_location` and
   `channel_set_properties`.
   - Binary evidence: the January obstruction test is
     `fld [channel+0x48]; fsubr [obstruction]`. That is the
     parenthesised-operand fingerprint. The same macro on all 31 sites keeps
     every sibling exact (probes `u1`, `v2a`, `v3`).
   - Provenance: Bungie's `realcmp` macro family is attested by January
     `.rdata` assert strings: `!realcmp(determinant, 0.0f)` (matrix_math),
     `realcmp(actor->input.facing_vect...` (actors) and
     `realcmp(plane3d_distance_to_poin...`. The tree reconstructs `realcmp` as
     `(fabs((a)-(b))<_real_epsilon)` in four TUs. The `!(... )` negation with
     a per-site epsilon is the shape that macro produces.
   - The name `realcmp_epsilon` is **not attested**.
2. **`DSOUND_POINT3D(p)` / `DSOUND_VECTOR3D(v)`**, expanding to
   `((p).x), ((p).z), ((p).y)` and `((v).i), ((v).k), ((v).j)`. Every
   DirectSound vector call uses them: listener position, orientation (forward
   and up) and velocity; channel position, cone orientation and velocity.
   - Semantics: DirectSound is left-handed with +y up, and the Halo world is
     right-handed with +z up, so each vector is passed as (x, z, y). One
     conversion macro for six call sites is natural.
   - Binary evidence: all 12 swapped components carry the parenthesised
     staging, and so do none of the other float arguments in the TU.
   - The names are **not attested**. The byte result does not depend on the
     name. A pointer-taking form (`(&v)->i`) was also exact (`v2c`).
3. **`channel_set_location` tail store order**: occlusion, obstruction,
   `attenuate_direct_path`. This is the order of the change-test conditions.
   January loads attenuate, occlusion and obstruction in that order and only
   then stores; the old source order compiled to a different schedule. This is
   the fresh-graphs `p5` order. It is needed together with control 1 (`o1` alone
   fails; `o1p5` is exact).

## Experiment matrix

All experiments ran on scratch copies. The sibling check was clean for every
variant: no baseline-exact row regressed.

| Shape | Listener | Channel | Result |
| --- | --- | --- | --- |
| `/Op` flag (mechanism check only) | 784 | 720 | inert |
| `(real)(double)(P)` on position | 800 | - | reverted (probe) |
| `?:` constant condition on position | 800 | - | reverted (probe) |
| `(real)P` on all 12 vector args, no parens | 784 | 720 | reverted |
| `(real)(P)` on position | 800 | - | reverted |
| `(P)` on position / on swapped only | 800 / 800 | - | reverted (probe) |
| `(real)P` on swapped only | 784 | - | reverted |
| parens on all 12 vector args (`parenall`) | **816 EXACT** | 752, compare+stores differ | probe |
| + `obstruction-(channel->obstruction)` (`o1`) | EXACT | 752, stores differ | probe |
| + `(obstruction)-channel->obstruction` (`o2`) | EXACT | compare still differs | reverted |
| + `channel->obstruction-obstruction` (`o3`) | EXACT | `fsub` sign form differs | reverted |
| `o1` + store order `p5` | EXACT | **752 EXACT** | probe |
| parens on both operands of both tail tests (`s2`) / right operand only (`s3`) | EXACT | EXACT | probe |
| parens on both operands of all 31 tests (`u1`) | EXACT | EXACT | probe |
| inline forwarding wrapper, result used (micro `e1`, `g1`, `g3`, `g4`, `g6`, `h1`-`h4`) | frame homes | - | rejected |
| inline forwarding wrapper, result unused (micro `g2`, `g5`) | staging reproduced | - | inapplicable: January uses the result |
| macros `realcmp_epsilon` + `DSOUND_*3D`, object form (`v2a`/`v3`) | **EXACT** | **EXACT** | **landed** |

Mechanism note on the D3D analogy: the XDK `D3DINLINE` forwarding wrappers
(`IDirect3DDevice8_SetVertexData4f`) produce the same staging because their
parameters become register temporaries. That only happens when the call's
result is unused. With the result used, VC7 gives the temporaries dead frame
homes. So an inline layer cannot explain the DirectSound sites, where January
tests the HRESULT.

## Do-not-repeat list

- Cast-only spellings, `real`/`double`/point locals, inline forwarding wrappers
  whose HRESULT is tested, and the `/Op` flag.
- Grinding the `_dsound_initialize` cross-jump tie and the
  `_channel_queue_packet` colour tie. Both are unchanged, and the paren law
  does not apply to them: no float values are involved.

## Residual classification

- `_dsound_initialize`: scheduling (cross-jump survivor among four identical
  failure tails). Measured 233/233 instructions.
- `_channel_queue_packet`: ABI/register colour tie (ebx/edi), after the
  fresh-graphs `q1` shape. Integer webs only.

## Reopen criteria

- `_dsound_initialize`: evidence that the four failure arms differed
  structurally in source.
- `_channel_queue_packet`: a use-weight lever with source evidence, starting
  from `q1`.
- Macro names: if an original name for the change-test or axis-swap macro
  surfaces (a January assert string, or a PC or Halo 2 era header), rename the
  macros. Bytes are independent of the names.

## Disposition

- Still active, `NonMatching`.
- 37/39 functions exact, up from 35.
- +2 functions / +1,568 padded bytes, strict exact with no alias.
- No fuzzy landing and no park.
- Data sections are untouched.
- Owner census: candidate equals `build/base`.
- Fake-match scan: 0 leads.
