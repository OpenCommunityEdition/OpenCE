# `rasterizer_xbox_environment.obj`: next-150K house-clean lane, wave n4 (2026-09-15)

## Scope and provenance

- Translation unit `source/rasterizer/xbox/rasterizer_xbox_environment.c`; target
  `build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj` (the January split COFF is byte
  authority).
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
  `opus/next-150k-house-clean-20260915`, frozen baseline `ae10935da`. Evidence-led relocation-census
  pass, wave n4.
- Worker-only measurements: `tools/campaign/gate.py`, `tools.campaign.tinfo`,
  `tools.campaign.relocdiff --count-by-target`, `tools.campaign.alndiff`, `tools/campaign/branch_sweep.py`,
  `scratch/parkcheck.py`, `tools/fake_match_scan.py`, a direct COFF symbol-table read through
  `tools/campaign/_coff_tools.py`, and `DIA2Dump.exe -sym` against HCEX.pdb (SHIP). No ninja, configure,
  pytest, header, config or tool edits; no other source file touched; the real file is unchanged.
- Prior ledgers read in full: `_lightmap_draw_fuzzy_20260912`, `_opus_reflections_reconciliation_20260909`,
  `_opus5_100k_20260914`, `_opus5_100k_waveC_20260914`, `_opus5_150k_w1_20260914`,
  `_opus5_250k_w3c_20260915`, `_opus5_next150_n3a_20260915`; worker notes
  `scratch/workers/rasterizer_xbox_environment.md`, `scratch/workers/w3c_rasterizer_xbox_environment.md`,
  `scratch/workers/n3_rasterizer_xbox_environment.md`.
- Worker notes for this wave: `scratch/workers/n4_rasterizer_xbox_environment.md`; scratch dir
  `scratch/workers/n4_rasterizer_xbox_environment/`.

## Baseline and final gate (identical; nothing landed)

`python -B tools/campaign/gate.py source/rasterizer/xbox/rasterizer_xbox_environment --all --forbid-emitted-symbol _point_from_line3d`

**exact 42, residual 2, unwritten 0 of 44** at baseline and at the end of the wave. Point guard pass;
`scratch/parkcheck.py` 1 park, drift 0; `tools/fake_match_scan.py` 0 leads; owner census vs
`build/base/.../rasterizer_xbox_environment.obj` 215 = 215 with no candidate-only and no lost owner;
`git diff --stat -- source/rasterizer/xbox/rasterizer_xbox_environment.c` empty.
`branch_sweep.py`: 17 unique historical blobs, the current tree is the best (42 exact).
The atlas lists 6 records, all already exact - this object has nothing unmined or unwritten.

| Row | Target size/relocs/sha16 | Real file | Status |
| --- | --- | --- | --- |
| `__rasterizer_environment_lightmap_draw` | 4016 / 248 / 7818e559325c1524 | 3984 / 246 / 02f716db92e8099f | residual [size, relocs, sha] |
| `_rasterizer_environment_specular_spot_light_begin` | 960 / 64 / e0331f6b94a74f9c | 944 / 64 / d103843ee5f01bad | residual, PARKED |

## `__rasterizer_environment_lightmap_draw` - NOT LANDED (1 shape)

The orchestrator's leverage statement for this wave was a relocation-by-target census delta:
January owns `{'_pixel_shader': 2}` that we do not.

### The census delta is real, and it is not a missing call

`relocdiff --count-by-target --allow-structural`, 248 target vs 246 candidate, two differing rows:

| target | January | ours |
| --- | --- | --- |
| `_pixel_shader+32` = `final_combiner_inputs_abcd` | 9 | 8 |
| `_pixel_shader+36` = `final_combiner_inputs_efg` | 2 | 1 |

Every call target multiset is equal (`_point_from_line3d` 2/2, `_scale_vector3d` 4/4,
`_real_rgb_color_to_pixel32` 6/6, `_real_alpha_to_pixel32` 1/1, `_csmemset`, `_display_assert` 7/7,
`_system_exit` 7/7, all D3D wrappers). Both deficits are data stores to the same two struct fields, so
the delta is a duplicated store pair, not a missing statement and not a wrong helper choice.

Cause, re-derived directly from January's bytes:

```
NONE arm      009fd  mov [_pixel_shader+32], 0xa0f000c
              00a07  mov [_pixel_shader+36], 0x1c011800
              00a11..00a2e  texture_modes (+216) and rgb_inputs[0] (+136)   <- arm ends here
              00a34  jmp 0xd7f
animated arm  00d1d  mov [_pixel_shader+32], 0xa0f000c
              00d27  mov [_pixel_shader+36], 0x1c011800
              00d31..00d7a  the five real_*_to_pixel32 calls                <- arm ends here
```

Neither January arm ENDS with the `+32`/`+36` pair. Our source writes that pair as the last two
statements of both arms; the two tails become byte-identical and VC7 cross-jumps them into a single
copy. It is a **store-order defect**, precisely wave B's recorded correction, re-measured by n3a.

### Frame census

January `sub esp,0x218`; ours `sub esp,0x218`. 132 distinct `[ebp+-N]` slots on each side with identical
per-slot reference counts and per-slot instruction lists. No frame-slot defect. The
`point_from_line3d` prototype in `source/math/real_math.h:1071` is `real t` and January's call pushes
4 arguments x 4 bytes with `add esp,0x10`, so there is no ABI/type defect either.

### Where the remaining bytes are

`scratch/workers/n3_rasterizer_xbox_environment/n1.c` (real file + wave B's four structural corrections)
gates **4000 / 248** - relocation- and frame-exact. The whole residue is 8 instructions / 16 bytes, at
the two `point_from_line3d` `t` arguments:

```
January  0xb88  fld dword [ebp-4]   ; add esp,0xc  ; ... ; push ecx ; fstp dword [esp]
ours     0xb88  mov ecx,[ebp-4]                     ; ... ; push ecx
January  0xba6  fld dword [ebp-0xc] ; add esp,0x10 ; ... ; push ecx ; fstp dword [esp]
ours     0xba0  mov edx,[ebp-0xc]                   ; ... ; push edx
```

**New January-side control recorded this wave.** The same body integer-copies a plain `real` local for
`real_alpha_to_pixel32(plasma_value)` (`0xc36 mov eax,[ebp-0x50]`, `0xc44 push eax`). January therefore
uses both forms inside one function, which independently confirms n3a's measured law - the staging
decision is per-argument-expression - and rules out register pressure, FP-state and any function-wide
explanation.

### Shape v1 and the decisive negative

`scratch/workers/n4_rasterizer_xbox_environment/v1.c` = `n1.c` with
`#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` and its `#undef` removed, testing laws_w2 **A22**
("an uninlined visible `__inline` compiles differently from an external prototype, so
`REAL_MATH_EXTERNAL_*` defines are defects"). Gate, scratch only, guard omitted by design:
**4048 / 246** - worse. VC7 inlines `point_from_line3d` at this site, so both call relocations vanish
and the body grows 48 bytes.

**COFF symbol-table evidence from January's own object.** In
`build/split/source/rasterizer/xbox/rasterizer_xbox_environment.obj`, `_point_from_line3d` and
`_scale_vector3d` are both **section 0 - undefined externals**, exactly as in our object; only the v1
candidate defines `_point_from_line3d` (section 7 COMDAT). Consequences:

1. January's TU saw an external declaration of both helpers, so the
   `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` and `REAL_MATH_EXTERNAL_SCALE_VECTOR3D` opt-ins in this file
   are **faithful**. Laws_w2 A22 is a per-TU fact, not a global rule, and it does not apply here.
2. This object is **not** a member of the laws_w2 C3 `_point_from_line3d` COMDAT owner-ruling family.
   Its 4,016 padded bytes need no COMDAT ruling, no header packet and no symbols.json change.
3. The declaration form cannot explain the x87 argument staging; that avenue is closed with January
   bytes rather than with a lab control.

### Classification and why nothing landed

Two known defects, neither in this wave's permitted scope:

- the store-order defect (measured, fixable, worth 2 relocations and 16 bytes of the gap) - but
  `n1.c` is 4000/248, a non-exact body replacing an already-written residual, forbidden by brief
  sections 6 and 8, and the w1 verifier required exactly these hunks reverted (R13);
- the x87 argument-staging residue - already measured **strict EXACT 4016 / 248 /
  `7818e559325c1524`** by w1 shape g2 using the constant `intensity = 1.0f` family, which the w1
  verifier rejected as fake arithmetic. It is an owner-policy item, not a byte problem.

Not a register tie, not a missing call, not a frame or ABI defect.

### Reopen criterion

(a) An owner/policy ruling admitting w1's named self-illumination `intensity = 1.0f` family (w1 shape
g2 is strict EXACT), or (b) donor/January evidence of a genuine non-identity factor or double
intermediate for the `t` argument. **Newly established by this wave:** no `_point_from_line3d` COMDAT
ruling and no header packet gate this row, so (a) alone unlocks 4,016 padded bytes. Do not respend on
plain-lvalue spellings of `t` (15 n3a controls), on nesting `periodic_function_evaluate` (refuted by
January's call order), or on the visible-inline declaration form (refuted here at 4048/246).

## `_rasterizer_environment_specular_spot_light_begin` - SKIPPED-EXHAUSTED (park locked, 0 shapes)

n3a closed the structural half one wave ago:
`scratch/workers/n3_rasterizer_xbox_environment/s1.c` gates **960 / 64** with frame parity
(`sub esp,0x84`) - size-, relocation- and frame-exact, still `[sha]`. The recorded park class
`register-allocation` was refuted there: the 16-byte gap is a `real_matrix4x3` local whose `scale` and
`position` are never written. 64/64 relocations agree by target on both sides.

The residue is three commutative `fld`/`fmul` operand pairs plus one `fxch` inside the shared
`real_math.h cross_product3d` inline, with no TU-local handle. This wave's directive forbids generic
x87-ordering work and forbids re-running the park-premise sweep n3a exhausted, and a park reopen
requires strict EXACT, which `s1.c` is not. Buying the last `fxch` with a commutative swap is R3;
swapping `cross_product3d`'s arguments breaks the currently-exact i term. **No park-reopen proposed.**

Reopen criterion (unchanged from w3c/n3a): (a) a measured VC7 law for the `fld` order of pointer-read
commutative operands, or (b) an owner-approved `real_math.h cross_product3d` change measured across its
consumers, noting that the current body is REQUIRED by the exact sibling
`__rasterizer_environment_diffuse_light_begin`.

## Rejected / do-not-repeat (added by this wave)

- Removing `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` from this TU to test laws_w2 A22: VC7 inlines the
  helper here (4048 / 246) and January's object proves the symbol is an undefined external on its side
  too. Never respend, and do not treat A22 as a global rule about `REAL_MATH_EXTERNAL_*`.
- Any function-wide or register-pressure explanation of the `point_from_line3d` `t` staging: refuted by
  January's own integer copy of `plasma_value` at `0xc36` in the same body.

## Proposals

- **No header, config, symbols.json or parked.json change arises from this wave.** The real file is
  unchanged.
- n3a's pending parked.json evidence/class correction for
  `_rasterizer_environment_specular_spot_light_begin` (`register-allocation` -> `x87-operand-order`)
  still stands; it is not restated here as a new packet.
- Byte-inert naming observation, recorded not proposed: HCEX.pdb (SHIP) gives the four environment draw
  entry points the parameter vocabulary
  `(const struct shader *shader, short shader_permutation_index, long dynamic_triangle_buffer_index,
  long first_triangle_index, long triangle_count, const struct vertex_buffer *vertex_buffer)`, with
  `vertex_buffers` (plural) only in `rasterizer_environment_lightmap_draw`. The plural is corroborated
  January-side because that body indexes the parameter as an array
  (`vertex_buffer + !rasterizer_environment_globals.lightmap_missing`). `shader_permutation_index` has
  no independent January corroboration - the value is only ever passed as `rasterizer_set_texture`'s
  `bitmap_sequence_index` - so under the brief's cross-build naming rule it was not adopted. Any change
  would span `source/rasterizer/rasterizer.h`, `source/rasterizer/rasterizer.c` and this file and is
  orchestrator-owned.
