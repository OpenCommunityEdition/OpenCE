# `object_lights.obj` Opus5 100K wave D park re-investigation (2026-09-14)

## Result

Scratch-only park re-investigation. **All five locked Object Lights parks reach
strict January identity in scratch candidates**. Four are strict EXACT by
themselves. `_lights_preprocess_scene` is EXACT once a private data label is
renamed. The production file `source/objects/object_lights.c` is **unchanged**:
every non-exact row is parked, so under the wave D protocol each closure is
handed to the orchestrator as a `park-reopen` proposal.

| Function | Padded | Relocs | Scratch candidate | Result | Mechanism |
| --- | ---: | ---: | --- | --- | --- |
| `_light_unmarked` | 96 | 8 | `scratch/workersD/object_lights/v3.c` (alt `v2.c`) | strict EXACT | conditional TRUE/FALSE result materialisation |
| `_find_point_lights_for_object_in_cluster` | 560 | 31 | same as above | strict EXACT | inlined copy of `light_unmarked`, no caller edit |
| `_build_distant_lights` | 1,024 | 78 | `scratch/workersD/object_lights/pr_bdl.c` | strict EXACT | inlined `magnitude2d` on a 2d view + named scale variable |
| `_lights_prepare_for_object_static` | 832 | 17 | `scratch/workersD/object_lights/pr_lpfos.c` | strict EXACT | `point` scoped to the corner loop |
| `_lights_preprocess_scene` | 2,400 | 165 | `scratch/workersD/object_lights/pr_lps.c` | normalized hash equal; strict `[reloc-identity]`; EXACT with `--alias lights_section=data_0030b388` | two artificial locals removed, one field bug fixed, radius store order |

Combined candidate `scratch/workersD/object_lights/all5.c`: 42 exact / 1
residual / 0 unwritten strict, and 43 / 0 / 0 with the data alias.

## Scope and provenance

- Unit `source/objects/object_lights.c` -> `build/split/source/objects/object_lights.obj`
  (SHA-256 `3e7fbead5c3f4c68bc9fbeadac0b23525830c9dc401012b44cdc0d0e90c3fb00`).
- Lane `opus/100k-consolidated-20260914`, HEAD `548451f6de5b81005586f583c89ef69b70f34a10`.
- Compiler VC7 13.00.9254 (`xbox/bin/vc7/CL.Exe`), flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus repository includes.
- Evidence used: the January target COFF (disassembly, relocations, `.rdata` strings), exact January functions in
  other objects used as same-compiler controls, the later HCEA reconstructions (semantic map only), and January
  debug-map atlas records (code names only; dumped read-only to `scratch/workersD/object_lights/atlas_object_lights.txt`).
  This lane had no prepared Ghidra or atlas export for this object.

## Validated baseline

Gate at lane HEAD: **38 exact / 5 residual / 0 unwritten** (guard passed); final real-file gate identical.

| Residual | Target size/relocs/hash16 | Base size/relocs/hash16 | First divergence |
| --- | --- | --- | --- |
| `_light_unmarked` | 96/8/`6aa93207648f0793` | 96/8/`721187363fa3f19c` | +0x42 our `xor eax,eax; cmp edx,ecx` vs January `cmp edx,[marker]; setne al` |
| `_find_point_lights_for_object_in_cluster` | 560/31/`39d22fa1133941bd` | 560/31/`3c814b365210b775` | the inlined `light_unmarked` predicate |
| `_build_distant_lights` | 1024/78/`9040b294b2196ad5` | 1008/78/`1b4925a4b1ee51c1` | +0x278 shadow-vector store and reload schedule |
| `_lights_prepare_for_object_static` | 832/17/`99209b5ebb1bce3e` | 832/17/`b4b295c47c720213` | +0xe3 corner `point.z` copy placement |
| `_lights_preprocess_scene` | 2400/165/`7265486275713570` | 2400/165/`32951ca676c56696` | +0x3 frame `0x3e0` vs `0x3dc`; +0x38 NONE pinned in ebx |

## Evidence and experiment matrix

### `light_unmarked` / `find_point_lights_for_object_in_cluster`

January produces the byte result directly (`setne al` with no `xor eax,eax`). Our `return a != b;` produces an int
and then truncates it. There is a same-TU control, measured this wave. The exact sibling `should_render_lights`,
respelled as `return render_lights && game_engine_allow_dynamic_lighting();`, becomes residual with the same
signature: `mov eax,1` / `xor eax,eax` where January has `mov al,1` / `xor al,al`. Its landed
`if (...) return TRUE; return FALSE;` form is exact. The exact sibling `light_mark` uses the same conditional form.
January inlines `light_unmarked` into `find_point_lights_for_object_in_cluster` (+0x54..+0x9e). That inlined
copy requires the same spelling: the 560-byte park closes with no edit to the caller.

| Shape | unmarked | find_point_lights | Siblings |
| --- | --- | --- | --- |
| v1 `boolean unmarked = FALSE;` at declaration + if | residual (`xor bl,bl` before the assert) | EXACT | kept |
| v2 `boolean unmarked;` + if/else assignment, single return | EXACT | EXACT | kept |
| v3 `if (light->marker != lights_globals.marker) { return TRUE; } return FALSE;` | EXACT | EXACT | kept |
| v4 assign FALSE after the assert, then if | residual | EXACT | kept |
| control: `should_render_lights` as an expression return | (n/a) | (n/a) | `should_render_lights` residual |

### `build_distant_lights`

At +0x287..+0x29b January emits `fld [esi+0x60]; fld [esi+0x5c]; fld st0; fmul st1; fld st2; fmul st3; faddp;
fsqrt; fstp st2; fstp st0`. These bytes are identical to January's own `_magnitude2d` COMDAT body (action_charge.obj
+0xb). The shadow-vector fields are re-read from memory, j first and then i. January `.rdata` proves the
`(real_vector2d *)&vector3d` idiom in January source. actors.obj has
`"(real_vector2d *) &actor->input.facing_vector"` and `"(real_vector2d *) &control_data.facing_vector"`;
actor_looking.obj has `"(real_vector2d *) &actor->output.facing_vector"`. Landed exact source also uses the idiom
(`action_charge.c` `normalize2d((real_vector2d *)&aim_vector)`).

In the else arm, January keeps the `0.707/h` scale on the x87 stack through both products and pops it afterwards,
which is the pattern of a named real variable. The exact function `following_camera_deterministic` has the same
idiom: `horizontal_magnitude = 1.f / horizontal_magnitude; forward_x *= horizontal_magnitude;`.

| Shape | Size | Result |
| --- | ---: | --- |
| shadow_i/j locals + `magnitude2d` cast | 1024 | 2 instruction diffs |
| shadow_i/j locals, no cast | 1008 | residual |
| A: fields + `magnitude2d((real_vector2d *)&lighting->shadow_vector)`, `*= 0.707f / h` | 1008 | 1 instruction diff |
| C: locals + cast + reused h | 1024 | 2 diffs |
| D/E: `scale_vector2d` inline in the else arm | 1008 | reads fields, residual |
| F: A + block-local `real scale = 0.707f / horizontal_length;` | 1024 | **EXACT** |
| G: A + `horizontal_length = 0.707f / horizontal_length; i *= horizontal_length; j *= horizontal_length;` | 1024 | **EXACT** (proposed) |

Owner census delta: the candidate emits `_magnitude2d` and `_magnitude_squared2d` COMDATs. These belong to the known
systemic class of used non-static `__inline` COMDATs; the baseline already emits `_power`, `_square_root`,
`_magnitude3d`, `_distance3d`, `_scale_vector3d` and others that the January split lacks. The orchestrator must
rule on this census delta. Policy flag: brief section 0 lists "representation casts (point3d->point2d)" as
exhausted. The January assert strings above are new, authoritative evidence for this specific idiom.

### `lights_prepare_for_object_static`

| Shape | Result |
| --- | --- |
| S1 `point = center; point.x += ...; point.y += ...` | 848 bytes, worse |
| S2 `point.z` assigned first | residual |
| S3 `real_point3d point;` declared inside the corner loop body | **EXACT** |

The declaration scope is the lever: `point` is used only inside the loop, and HCEA also scopes it there.

### `lights_preprocess_scene`

These are measured January byte facts, applied cumulatively:

1. January pins NONE in `ebx` and re-reads `light->parent_light_index` for the elapsed subtraction. Our named local
   `transition_start_time` consumed `ebx`. Removing it makes the first loop exact.
2. January reads `unit.active_camouflage` through `edi` twice. Our `active_camouflage` local was the extra frame
   slot. Removing it gives frame `0x3dc`.
3. The marker loop's induction pointer is `&markers[i].matrix.forward`: +0x3c, with position at +0x24 and up at
   +0x18. **Reconstruction bug:** we compressed `matrix.left`. January compresses `matrix.forward`, consistent with
   the non-marker arm's `light->forward`.
4. January loads and stores `light_parameters.radius` before the color block copy. Moving the radius assignment
   first leaves only `[reloc-identity]`.

After step 4, the January relocations at +0x1b/+0x24 and +0x93b/+0x944 name `_data_0030b388`, while ours name
`_lights_section`. Both `.data` sections are 1,541 bytes with the same other owners. With
`--alias lights_section=data_0030b388` the function is EXACT. No authentic name exists for this private static:
atlas records for the object cover code only, and the PDB has no public near 0x30b388. Name evidence is the
January profile string `"lights"` plus the `<name>_section` convention in `config/symbols.json`
(`_render_model_section`, `_game_update_section`, ...).

## Checks

- Every candidate kept all 38 baseline-exact rows. The `_point_from_line3d` guard passed.
- Park drift: only the proposed park moves in each individual candidate. The other parks keep their
  `measurements.base` hashes.
- `tools/fake_match_scan.py`: 0 leads on the real file and on every candidate.
- Real file unchanged: `git diff --stat -- source/objects/object_lights.c` is empty.

## Do not repeat

- Do not re-test `return lights_globals.marker != light->marker;` or a flag initialised at declaration.
- Do not re-test `square_root(i*i + j*j)` over the fields or with shadow locals. January uses an inlined
  `magnitude2d` view.
- Do not re-test a struct copy of the center or a `z`-first corner assignment.
- Do not add locals to `lights_preprocess_scene`. The parked residual came from two superfluous locals.

## Residual classification and disposition

All five residuals are source/topology facts (result spelling, helper call, declaration scope, superfluous locals, a
field bug, statement order) plus one ownership/naming gap (a data label). None is a backend tie. Disposition:
**parked, proposed for reopen**. No credit is claimed until the orchestrator unparks the functions, applies the
`_data_0030b388` -> `_lights_section` rename, and lands them.

## Reopen criteria

Already met; see the proposals above. If the orchestrator rejects the `(real_vector2d *)` idiom, the only
alternative left for `build_distant_lights` is an authoritative January source statement.
