# `rasterizer_xbox_environment.obj`: next-150K house-clean lane, wave n3a (2026-09-15)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`; target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj` (the January split COFF is byte authority).
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/next-150k-house-clean-20260915`,
  frozen baseline `ae10935da`. Structural park pass, wave n3.
- Worker-only measurements: `tools/campaign/gate.py`, `tools.campaign.tinfo`, `tools.campaign.relocdiff`,
  `tools.campaign.alndiff`, `scratch/parkcheck.py`, `tools/fake_match_scan.py`, plus a micro-TU lab compiled with this
  unit's own `build.ninja` cflags and CL (`scratch/workers/n3_rasterizer_xbox_environment/lab.py`). No ninja,
  configure, pytest, header, config or tool edits; no other source file touched.
- Prior ledgers read in full: `_codex_small_exact_20260902`, `_diffuse_texture_reconciliation_20260909`,
  `_lightmap_draw_fuzzy_20260912`, `_opus_reflections_reconciliation_20260909`, `_opus5_100k_20260914`,
  `_opus5_100k_waveC_20260914`, `_opus5_150k_w1_20260914`, `_opus5_250k_w3c_20260915`; worker notes
  `scratch/workers/rasterizer_xbox_environment.md` and `scratch/workers/w3c_rasterizer_xbox_environment.md`.
- Laws applied as detectors: WORKER_BRIEF section 5/9, `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D),
  `scratch/w3/laws_w3.md` (A23-A45, R13-R19, C, D).
- Worker notes for this wave: `scratch/workers/n3_rasterizer_xbox_environment.md`; scratch dir
  `scratch/workers/n3_rasterizer_xbox_environment/`.

## Baseline and final gate (identical; nothing landed)

`python -B tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --all --forbid-emitted-symbol _point_from_line3d`

**exact 42, residual 2, unwritten 0 of 44** at baseline and at the end of the wave. Point guard pass;
`scratch/parkcheck.py` 1 park, drift 0; `tools/fake_match_scan.py` 0 leads; owner census vs
`build/base/.../rasterizer_xbox_environment.obj` 91 = 91, no candidate-only and no lost owner;
`git diff --stat -- source/rasterizer/xbox/rasterizer_xbox_environment.c` empty.

| Row | Target size/relocs/sha16 | Real file | Status |
| --- | --- | --- | --- |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | residual [size, relocs, sha] |
| `_rasterizer_environment_specular_spot_light_begin` | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | residual, PARKED |

`__rasterizer_environment_diffuse_light_begin` (w1 park reopen) and `_IDirect3DDevice8_SetTexture@12` are inherited
exact and were not revisited.

## `__rasterizer_environment_lightmap_draw` - NOT LANDED (1 shape; the search space is now closed on the C side)

This row is **not** in `config/parked.json`; it is an ordinary residual. The applicable recorded criterion is the
w3c one: "January or donor evidence of a real intensity/brightness term or a double/cast intermediate - OR an
owner/policy ruling on the laws_w3 C3 x87-staged-argument family that also covers stack-local leaves."

### Relocation census by target

Every relocation target agrees except two, and both deficits are in the same struct:

| Target | January | real file |
| --- | --- | --- |
| `_pixel_shader+32` = `final_combiner_inputs_abcd` | 9 | 8 |
| `_pixel_shader+36` = `final_combiner_inputs_efg` | 2 | 1 |

**Cause proven.** Our two illumination arms (`illumination->map.index == NONE` and the animated arm) end with the
identical two-statement tail `final_combiner_inputs_abcd = 0x0A0F000C; final_combiner_inputs_efg = 0x1C011800;`, and
VC7 cross-jumps those tails into a single copy. January does not merge them: in January the animated arm's constant
block *ends* at `+32`/`+36` (0xd1d, 0xd27) and is followed by the five `real_*_to_pixel32` calls, so the two arms
share no tail. That is precisely wave B's recorded correction "all constant `pixel_shader` stores before the five
`real_*_to_pixel32` calls". A missing relocation here is therefore a store-ORDER defect, not a missing statement.

### Frame census

January `sub esp,0x218`; ours `sub esp,0x218`. A complete `[ebp±N]` census of both bodies gives **132 distinct slots
on each side with identical per-slot reference counts**. The per-slot instruction lists are identical too: for
`[ebp-4]` both sides have the same 11 references and for `[ebp-0xc]` the same 3, differing in exactly one instruction
each. **No local is missing, extra, differently sized or differently scoped.** The census refutes every
missing-statement / extra-slot / wrong-width / wrong-control-flow hypothesis for this row.

### Shape n1 (wave B reproduced on the current baseline)

`scratch/workers/n3_rasterizer_xbox_environment/n1.c` = the real file plus only wave B's four structural corrections
(stage-0 MAG/MIN/MIP filters written inside each test-pattern arm; genuine XDK `PS_TEXTUREMODES(...)` for both
`texture_modes` stores; all constant `pixel_shader` stores before the five `real_*_to_pixel32` calls; the
`specular_mask` case placed after the `bump_edge` case). Gate: **4000 / 248 [size, sha]**, whole TU 42 exact /
2 residual / 0 unwritten, point guard pass. Relocation count and every relocation target now equal January.
Not landed: it is a non-exact body replacing an already-written residual (brief sections 6 and 8), and the w1
verifier required exactly these hunks reverted (R13).

### The entire remaining gap

947 vs 939 instructions / 16 padded bytes, in two instruction pairs:

```
January  0xb88  fld dword [ebp-4]   ; add esp,0xc  ; ... ; push ecx ; fstp dword [esp]
ours     0xb88  mov ecx,[ebp-4]                      ; ... ; push ecx
January  0xba6  fld dword [ebp-0xc] ; add esp,0x10 ; ... ; push ecx ; fstp dword [esp]
ours     0xba0  mov edx,[ebp-0xc]                    ; ... ; push edx
```

January stages the `t` argument of both `point_from_line3d` calls through the x87, which forces it to flush the two
pending cdecl cleanups so `[esp]` is the argument slot; we integer-copy and merge five cleanups into one
`add esp,0x44`. Everything after that is offset and allocation cascade.

### NEW MEASURED LAW: VC7 float-argument staging (15 controls)

A micro-TU that reproduces the block in shape (`rep1.c`, `rep2.c`, `rep3.c` with
`scratch/workers/n3_rasterizer_xbox_environment/lab.py`, compiled with this unit's own cflags and CL) was used to
enumerate the rule. The baseline probe emits our exact integer-copy shape; a `double`-parameter control emits
`fstp qword [esp]`, so the detector is sound.

Integer-copied (no FPU staging) for every one of: a plain `real` local defined by a call return; named `1.0f - v`
off-scale locals; a field of a local struct; an element of a local `real[2]`; an address-taken local; a `real` global;
a `real` parameter; a parenthesised `(v)`; declare+initialize; a value with an extra later fp use; a helper whose
pointer return is discarded; a value with no other fp use at all; a local *defined* by an fp expression and passed
unchanged; a tag-block field read through the pointer; an element of a `real const *` parameter.

FPU-staged only for: an fp-VALUED argument expression, and a directly nested call result.

> **Law.** VC7 integer-copies every plain float lvalue argument - local, parameter, global, struct field, array
> element or address-taken local, however it was defined. Only an fp-valued expression (or a call result consumed
> directly in the argument list) is staged `push reg; fstp dword [esp]`.

This also corrects the w3c framing that the 68 other January `fld [reg+off] -> fstp [esp]` sites are "pointer field"
staging: a pointer field on its own does not do it.

### Consequence and classification

January's `t` argument must be an fp expression whose emitted code is a single `fld dword [ebp-N]` - i.e. an
expression that c2 folds to the bare local value. Every such expression is an identity (`v*1.0f`, `v+0.0f`,
`(real)v`, `+v`): the w1 `intensity = 1.0f` family, rejected by the w1 verifier as fake arithmetic, bound by R13 and
by this wave's orchestrator note, and separately R7 for the cast forms. The one non-identity form that does stage
through the FPU - a directly nested `periodic_function_evaluate` call - is refuted by January's call order
(scale, scale, point, point; nesting would emit scale, point, scale, point).

**Classification:** unresolved source form - x87 argument staging - with the C-side search space now closed. Not a
register tie; not a missing statement.

**Reopen criterion (refined).** Reopen only on (a) an owner/policy ruling admitting a named self-illumination
intensity of 1.0 - the January-side argument being the two `push 0x3f800000` constants at the plasma
`scale_vector3d` calls, the only constant-1.0 pushes among January's twelve out-of-line `_scale_vector3d` calls - or
(b) donor/January evidence of a genuine non-identity factor or double intermediate. Do **not** spend further shapes
on plain-lvalue spellings of the argument: fifteen controls in this wave say they cannot work. Start from
`scratch/workers/n3_rasterizer_xbox_environment/n1.c` (4000/248, relocation- and frame-exact).

## `_rasterizer_environment_specular_spot_light_begin` - NOT LANDED (park locked; 1 shape) - PARK CLASS REFUTED

Park's recorded criterion (`config/parked.json`, class `register-allocation`): *"... January is 960 padded bytes and
the candidate 944, with all 64 relocations shared ... The remaining difference is compiler vector/local allocation
and scheduling, not absent renderer behavior; bounded helper and declaration-shape probes did not close it. ...
Reopen only for authoritative January source/local records or a natural same-compiler donor that explains the vector
lifetime schedule."*

### Frame census - the park premise does not survive it

January `sub esp,0x84`; the parked body `sub esp,0x74`. January's slots are vertex constants `[-0x84,-0x34)`, an
unreferenced 4 bytes at `-0x34`, forward `-0x30`, cross result `-0x24`, up `-0x18`, and an unreferenced 12 bytes at
`-0xc`: exactly `real_matrix4x3 {scale; forward; left; up; position}` with `scale` and `position` never written. The
parked body declares three separate `real_vector3d` locals, so its slots pack with no gaps and it is one aggregate
short. **The 16-byte gap is a wrong declared local TYPE - a structural source defect - not "compiler vector/local
allocation and scheduling".** The recorded class `register-allocation` is refuted.

### Relocation census

64 / 64 identical by target for both the parked body and the corrected candidate. No call or global is missing or
extra on either side.

### Shape s1

`scratch/workers/n3_rasterizer_xbox_environment/s1.c` = the current real file with only this body replaced by w1's
`s3a` shape (`real_matrix4x3 matrix; matrix.forward = light->forward; matrix.up = light->up;
normalize3d(cross_product3d(&light->forward, &light->up, &matrix.left));`, vertex constants read from `matrix.*`).
Gate: **960 / 64 [sha]**, frame `sub esp,0x84`, whole TU 42 exact / 2 residual, point guard pass. Size, relocations
and frame are January-exact; 247 vs 248 instructions. It supersedes `s3a.c`, which also carries the rejected
`intensity` tree and the pre-reopen diffuse body.

### What is left

Three commutative x87 operand-order pairs inside the shared `real_math.h cross_product3d` inline, plus their
downstream allocation (`eax = &light->forward = esi+0x10`, `ecx = &light->up = esi+0x1c`):

```
j term  January  fld [ecx]   ; fmul [eax+8]      ours  fld [eax+8] ; fmul [ecx]
        January  fld [ecx+8] ; fmul [eax]        ours  fld [eax]   ; fmul [ecx+8]
k term  January  fld [ecx+4] ; fmul [eax]        ours  fld [eax]   ; fmul [ecx+4]
```

plus one eax/ecx/edx rotation in the `light->position` copy, `lea ecx` vs `lea edx` at 0x1b5, and one extra
`fxch st(1)` at 0x1ea. The i term already matches byte for byte on both sides, and January itself is not uniform
(5 of 6 products load `up` first, ours 2 of 6), which is the fingerprint of leaf age rather than of a source operand
order. Three independent pins block a source lever:

- **argument order** is pinned - the matching i term computes `up.k*forward.j - forward.k*up.j` =
  `cross(forward, up).i`, so swapping the arguments (and dropping the `vertex_constants[2]` negations, which is
  mathematically equivalent) would break a currently-exact hunk;
- **operand addresses** are pinned by w3c's `s3b` proof (matrix members make VC7 read the local slots, while January
  reads `light`-relative addresses);
- the **integer struct-copy schedule** is byte-identical on both sides, so the two member copies are pinned as well.

Buying the last `fxch` with a commutative swap of `cone_scale * (radius * 0.5f)` is R3.

**Classification:** the structural half is closed in scratch; the residual is an x87 commutative leaf-age tie inside
a shared header inline, with no TU-local handle. No park-reopen is proposed - the brief requires strict EXACT and
`s1.c` is `[sha]`.

**Reopen criterion (unchanged from w3c):** (a) a measured VC7 law explaining the `fld` order for pointer-read
commutative operands, or (b) an owner-approved `real_math.h cross_product3d` change measured across its consumers,
noting that the current body is REQUIRED by the exact sibling `__rasterizer_environment_diffuse_light_begin`.

## Rejected / do-not-repeat (added by this wave)

- Any plain-lvalue spelling of the `point_from_line3d` `t` argument in `lightmap_draw` (15 measured controls).
- Nesting the `periodic_function_evaluate` call inside the `point_from_line3d` argument list - the only non-identity
  FPU-staging form - refuted by January's call order.
- Re-landing wave B's four structural corrections alone (4000/248): brief sections 6/8 plus the w1 verifier's
  full-revert requirement (R13).
- Swapping `cross_product3d`'s arguments in the spot light: breaks the currently-matching i term.

## Proposals

- **parked.json evidence/class correction (no byte effect, orchestrator-owned).** For
  `_rasterizer_environment_specular_spot_light_begin`, change `"class"` from `register-allocation` to
  `x87-operand-order` and amend the evidence text: the 16-byte gap is a `real_matrix4x3` local whose `scale` and
  `position` are never written (frame census: January `sub esp,0x84` vs the parked body's `0x74`, January's
  unreferenced 4 bytes at `-0x34` and 12 bytes at `-0xc`), closed in scratch at 960/64 with frame parity by
  `scratch/workers/n3_rasterizer_xbox_environment/s1.c`; the remaining difference is the commutative `fld`/`fmul`
  leaf order inside the shared `real_math.h cross_product3d` inline. No source, header, symbols.json or config change
  arises from this wave, and the park stays locked.
- No header or config need. The real file is unchanged.
