# `rasterizer_xbox_environment.obj`: opus5 250K house-clean lane, wave w3c (2026-09-15)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`, target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj` (the January split COFF is the byte authority).
- Lane `opus/250k-house-clean-20260915` (continues `opus/150k-house-clean-20260914`). Worker-only measurements with
  `tools/campaign/gate.py`, `tools.campaign.alndiff`, `tools.campaign.tinfo`, `scratch/parkcheck.py`,
  `tools/fake_match_scan.py`. No ninja, header, config or tool edits; no other source file touched.
- Prior ledgers read in full: `_codex_small_exact_20260902`, `_diffuse_texture_reconciliation_20260909`,
  `_lightmap_draw_fuzzy_20260912`, `_opus_reflections_reconciliation_20260909`, `_opus5_100k_20260914` (wave B),
  `_opus5_100k_waveC_20260914`, `_opus5_150k_w1_20260914`; worker notes
  `scratch/workers/rasterizer_xbox_environment.md`; the w1 verifier verdict in `scratch/w1/w1-results.json`.
- Laws applied as detectors: WORKER_BRIEF section 9, `scratch/w2/laws_w2.md` (A1-A22, R1-R12, D) and
  `scratch/w3/laws_w3.md` (A23-A45, R13-R19, C, D).
- Worker notes for this wave: `scratch/workers/w3c_rasterizer_xbox_environment.md`;
  scratch dir `scratch/workers/w3c_rasterizer_xbox_environment/`.

## Baseline and final gate (identical; nothing landed)

`python -B tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --all --forbid-emitted-symbol _point_from_line3d`

**exact 42, residual 2, unwritten 0 of 44** - point guard passed, `scratch/parkcheck.py` 1 park drift 0,
`fake_match_scan.py` 0 leads, `git diff --stat -- source/rasterizer/xbox/rasterizer_xbox_environment.c` empty.

| Row | Target size/relocs/sha16 | Real file | Status |
| --- | --- | --- | --- |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | residual [size, relocs, sha] |
| `_rasterizer_environment_specular_spot_light_begin` | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | residual, PARKED |

Context: `__rasterizer_environment_diffuse_light_begin` (w1 park reopen) is EXACT at HEAD and
`_IDirect3DDevice8_SetTexture@12` is inherited exact; neither was revisited. The w1 `lightmap_draw` landing was
rejected by its verifier and reverted, so the row is back at the 100K baseline.

## `__rasterizer_environment_lightmap_draw` - SKIPPED-EXHAUSTED (0 shapes spent)

**What is left.** Against the wave-B candidate (`scratch/workers/rasterizer_xbox_environment/v0.obj`, 4000/248) the
entire remaining 16-byte / 2-relocation gap is the `t` argument of the two `_point_from_line3d` calls:

```
January  @b88  fld dword [ebp-4] ; add esp,0xc ; lea eax,[ebp-0x5c] ; push eax ; push ecx ;
               lea ecx,[edi+0x19c] ; fstp dword [esp] ; push ecx ; lea edx,[ebp-0x5c] ; push edx ; call
ours     @b88  mov ecx,[ebp-4] ; lea eax,[ebp-0x5c] ; push eax ; push ecx ; ... (and one merged add esp,0x44)
```

Everything else in the self-illumination block already matches byte for byte: the three
`periodic_function_evaluate` argument stages, both `fld __real@3f800000; fsub [ebp-N]` scale arguments, both plasma
`push 0x3f800000` constants, and - decisively - the **integer** push `mov eax,[ebp-0x50]` of `plasma_value` into
`real_alpha_to_pixel32` in the same function. Same declaration class, same block, integer-copied: the trigger is not
the declaration of the local.

**New evidence gathered this wave.**

1. *January-wide census* (re-using the 100K lane's `fldscan.py` over every `build/split` object): of the 71 pure
   `fld dword [mem]` -> `fstp dword [esp]` argument stages in the whole January image, only three read a STACK slot -
   these two sites and `_ai_test_line_of_fire @70` (`[ebp+eax-0x4e4]`, an indexed stack array element). The other 68
   stage a pointer field (`[reg+off]`). `lightmap_draw` is therefore a member of the unresolved x87-staged-real-argument
   family recorded in `laws_w3` C3 (`_object_test_vector`, `_player_examine_nearby_device`, `_ai_test_line_of_fire`,
   `_biped_update_moving`), not an isolated puzzle. The family's only known reproduction - a parenthesised argument
   produced by a genuine macro - is measured INERT on a stack-local leaf (w1 p1 = 4000), so the `_object_test_vector`
   form does not transfer here.
2. *Donor re-check.* Ghidra `scratch/ghidra/out/rasterizer_xbox_environment.obj.decomp.c:1416-1430` shows a bare local
   copy for the `t` argument (`local_264 = (undefined4 *)local_8;`). HCEA's later-build
   `_rasterizer_environment_lightmap_draw_pp.c:156-213` computes `on*blend + off*(1-blend)` componentwise with **no**
   intensity term and copies the plasma colours directly. Both corroborate the w1 verifier's rejection of the constant
   factor; neither satisfies the reopen criterion. The punpckhdq and Stian trees contain no body for this object.

**Detectors run, none fires.** A23 (no loop), A24 (frame 0x218 and all three value slots already identical),
A25/A29 (single return, no join store), A31(a) (the `add esp` flush positions already match), A33 (block scoping
without the factor measured 4000: w1 f2/g2n), A35 (not a commutative product), A36 (the constant is exactly 1.0f),
A38 (inner-block declare+init temporaries measured 4000: w1 f2), A42 (no store to prove).

**Why no shape was spent.** Every exact form on record (w1 c1/e2/f1/g2 and waveC k16/k17/k22) requires the constant
1.0 identity factor or an explicit cast on a c2-folded expression. The w1 verifier rejected that tree as fake
arithmetic and required a full revert including the wave-B hunks; R13 binds that rejection, and no w2/w3 law or donor
supplies a genuine intensity/brightness term or a double intermediate. Wave B's four structural corrections alone
reach only 4000/248 and cannot land for an already-written residual (brief section 6).

**Classification.** Unresolved source form (x87 argument staging), not a register tie.

**Reopen criterion.** Unchanged from w1 - January or donor evidence of a real intensity/brightness term or a
double/cast intermediate - OR an owner/policy ruling on the `laws_w3` C3 x87-staged-argument family that also covers
stack-local leaves. Start from `scratch/workers/rasterizer_xbox_environment/v0.c` (4000/248).

## `_rasterizer_environment_specular_spot_light_begin` - NOT LANDED (park locked; 1 shape, 2 refutations)

w1 closed the structural half in scratch: the frame is a `real_matrix4x3 matrix` whose `scale` (-0x34) and `position`
(-0xc) are never written (forward -0x30, left -0x24, up -0x18), and `s3a.c`
(`matrix.forward = light->forward; matrix.up = light->up; normalize3d(cross_product3d(&light->forward, &light->up,
&matrix.left));`) reaches 960/64 with frame parity and 15 small blocks left.

**Shape p1 (new, evidence-led, refuted).** `source/math/matrix_math.c:337-346` shows the house idiom for building a
basis - `matrix->forward = *forward; cross_product3d(...); matrix->up = *up;` - i.e. the up copy AFTER the cross.
Under A35 that should make `up` the younger leaf and flip the j/k operand order towards January's.
Measured (`scratch/workers/w3c_rasterizer_xbox_environment/p1.c`, `p1.obj`): 960 [sha], but structurally worse than
s3a - the i-term flips to forward-first (`fld [esi+0x14]`), the up copy is pushed past the cross and loses January's
`lea ecx,[esi+0x1c]; mov edx,ecx` twin-pointer form, and the `push ebx` prologue register disappears. This **refutes**
the matrix_math ordering for this function: January's interleaved twin-pointer copies require both copies before the
cross, exactly as s3a spells it.

**Two controls that pin the source form (no shape spent).**

- Re-reading w1's `s3b.obj`: taking the cross operands from the matrix members makes VC7 read the LOCAL slots
  (`fld [ebp-0x10]; fmul [ebp-0x2c]`), whereas January reads light-relative addresses
  (`[eax+..]`, `[ecx+..]`, `[esi+0x20/0x24]`). VC7 does not forward the struct copy, so January's cross arguments are
  `&light->forward, &light->up`; every mixed spelling (`&matrix.forward, &light->up` and friends) is excluded by the
  same address evidence.
- Sibling control inside this object: the EXACT `__rasterizer_environment_diffuse_light_begin` inlines the same
  `real_math.h` `cross_product3d` from local matrix members, and January loads operand b (up) first in **all six**
  products. The spot light loads b first in 5 of 6 (the exception is the i-term subtrahend, whose up operand is read
  through the light base ESI before `lea eax` exists); s3a loads b first in only 2 of 6. The order tracks leaf age and
  pointer availability inside the shared inline; this TU cannot change it without moving the operand addresses away
  from January's.

**Remaining 15 blocks in s3a:** four cross-product `fld`/`fmul` operand orders, one eax/ecx/edx rotation in the
`light->position` copy, one `fxch st(1)` before the last vertex constant, and the `rasterizer_set_pixel_shader` push
placement - the last three all downstream of the operand order (the two x87 stack residents at the tail are
`cone_scale` and `radius*0.5f`, swapped only because of the earlier product order).

**Classification.** x87 leaf-age / allocator tie inside the shared `real_math.h cross_product3d` inline; the source
spelling is pinned by January's addresses; no TU-local lever remains. Do not re-spend on cross-operand permutations
(9 shapes total across w1 and w3c).

**Reopen criterion (refined; supersedes w1's).** w1 asked for "a natural cross-product spelling that loads up.i/up.j
first in the j/k terms"; this wave shows no such spelling exists from this TU. Reopen only on (a) a measured VC7 law
explaining the `fld` order for pointer-read commutative operands, or (b) an owner-approved `real_math.h`
`cross_product3d` change measured across its consumers - noting that the current k,j,i body is REQUIRED by the exact
sibling `__rasterizer_environment_diffuse_light_begin`, so any such change must keep that row exact. Best scratch body
remains `scratch/workers/rasterizer_xbox_environment/s3a.c` (960/64, frame 0x84, sha16 9c8a4202366a9d91).

## Rejected / do-not-repeat (added by this wave)

- The matrix_math.c copy/cross/copy ordering in the spot light (p1): structurally worse; both copies precede the cross.
- Cross operands read from the matrix members or from a mixed member/pointer pair: wrong operand addresses (s3b proof
  that VC7 does not forward the struct copy).
- Re-proposing the constant `intensity = 1.0f` factor (or any identity/cast wrapper) in `lightmap_draw`: rejected by
  the w1 verifier, corroborated as absent by HCEA and Ghidra; R13 binds.

## Proposals

None. No header, config, symbols.json or parked.json need arises from this wave; the park stays locked and the real
file is unchanged.
