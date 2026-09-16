# `rasterizer_lights.obj` — Opus5 250K lane, wave w3c (2026-09-15)

Lane `opus/250k-house-clean-20260915`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
Scope: `source/rasterizer/rasterizer_lights.c` only. Third pass; the priority target named by the orchestrator was
`_rasterizer_lens_flares_draw` (laws_w3 section C1, law A35).

## Result

**No source change landed.** `git diff --stat -- source/rasterizer/rasterizer_lights.c` is empty.

| gate (real file, `--all --forbid-emitted-symbol _point_from_line3d`) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 9 | 4 | 0 |
| final | 9 | 4 | 0 |

Guard: pass. `scratch/parkcheck.py`: 2 parks, drift 0. `tools/fake_match_scan.py`: 0 review leads. Owner census: unchanged.

One orchestrator proposal is re-raised (a park-body data-only correction, re-verified at this HEAD).

## `_rasterizer_lens_flares_draw` (2,240 B, parked) — priority target, NOT landed

Baseline target `05fb320827e436e1…` / 115 relocs; park base `25834daf80e88b41…` / 115; frames and slot census identical
(`sub esp,0xb4`, 669 vs 669 instructions).

The residual is exactly four items, three of them ordering:

1. `+0x202` — January `fld [ebp-0x28] (occlusion_fraction); fmul [ebp-0xc] (light_brightness)`; ours loads
   `light_brightness` first.
2. `+0x31c` — the i-term of the inlined `dot_product3d(&direction, &camera_offset)`:
   January `fld [ebp-0x68] (direction.i); fmul [ebp-0x18] (camera_offset.i)`; ours swapped. The j and k terms already agree.
3. `+0x738…+0x74e` — an interleave of `fmul st(1)` with `xor edx,edx / mov dx,[edi+4] / push edx` while the
   `rasterizer_widget_set_texture` integer arguments are materialised. Identical instruction multiset, two positions moved.
4. Five `.rdata` assertion-literal COMDATs whose text spacing differs from January's (see the proposal below).

### A35 detector run (why the C1 lever does not fire here)

Law A35(1) ("in a commutative mem × mem product VC7 `fld`s the younger leaf") **predicts our build, not January's**, at both
x87 sites:

- `occlusion_fraction` is stored at `+0x1a9`; `light_brightness` at `+0x1d9` / `+0x1f5` / `+0x2e5` — so light_brightness is
  the younger leaf, and ours loads it first.
- `direction` is stored at `+0xae` (the `uncompress_int32_to_real_vector3d` result copy); `camera_offset` is rewritten by the
  out-of-line `normalize3d` call at `+0x291` — so camera_offset is the younger leaf, and ours loads it first.

January loads the older leaf at both sites, i.e. January follows source/parameter order where our build applies the
younger-leaf swap. The two mixed global/local dot products behave identically in both builds (global-first for the k and j
terms, local-first for the i term at `+0x14f`, `+0x2b9`, `+0x37a`), so no leaf-age lever can move site 2 without breaking
those three. Both remaining x87 sites are therefore **commutative operand orders whose only justification would be x87 load
order**, which laws_w2 **R3** rejects. The A35(4) hand-expansion route is out under R14/R15. No shape was spent.

### Statement-shape evidence (new, corroborating; no shape spent)

HCEA's later DX9 reconstruction
(`halocea-review/src/blam/rasterizer/dx9/rasterizer_lens_flares_draw.c:156-158`) writes the brightness chain as one
three-factor expression, `light_alpha = uncompress_int8_to_real(...) * occlusion_fraction * distance_fade;`, with a separate
`distance_fade` local. January's own bytes refute the separate local for this build: the fade result and the product both
live in `[ebp-0xc]` inside frame `0xb4`, and wave w2's D2 (separate `distance_fade` local) measured frame `0xb8`, which
laws_w2 A6's side law (same-block locals never share a cell) explains. So January reuses one variable, exactly as the park
body does. w2's D3 (`light_brightness = occlusion_fraction*light_brightness*uncompress(...)`) is recorded as "different x87
(worse)": a single expression lets VC7 forward the PIN result into an x87 phi and drops January's memory phi
(`mov [ebp-0xc],0` / `mov [ebp-0xc],0x3f800000` / `fstp [ebp-0xc]`), which the park body reproduces exactly. Not retried.

### ORCHESTRATOR-PROPOSAL (re-raised from w2, re-verified at this HEAD)

Park-body **data-only** correction, no code drift. In `source/rasterizer/rasterizer_lights.c`,
`rasterizer_lens_flares_draw`, replace the five `match_assert` expression texts (lines 1034, 1058, 1062, 1066, 1070) with
January's `.rdata` spellings:

```
reflection->animation_period!=0.0f
animation_color.alpha>=0.0f && animation_color.alpha<=1.0f
animation_color.red >=0.0f && animation_color.red <=1.0f
animation_color.green>=0.0f && animation_color.green<=1.0f
animation_color.blue >=0.0f && animation_color.blue <=1.0f
```

Candidate `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc/lights/d4.c` (a copy of
`scratch/workers/rasterizer_lights/D4.c`; it differs from HEAD only in those five lines). Measured on 2026-09-15 at this
HEAD: gate 9/4/0, guard pass, `parkcheck` 2 parks / 0 drift (the draw code sha is unchanged), and the five wrong `??_C@…`
literal owners disappear from the draw diff, leaving only the three ordering sites above. Zero code credit; it removes five
candidate-only `.rdata` owners. The body is park-locked, so this is an orchestrator edit.

**Reopen criterion:** an owner ruling on R3 for the two commutative products (with the HCEA factor-order corroboration
above) **and** a lever for the `+0x738` integer/x87 interleave. Either one alone still leaves the function non-exact.

## `_rasterizer_lens_flare_submit_for_cluster` (400 B) — skipped, evidence-exhausted

laws_w2 section D lists this row with a tie verdict after five shapes (c1-c5), and the orchestrator's prompt says to skip it
absent genuinely new evidence. The w2 evidence was re-read: same frame `0x54`, same 18 relocations, same store order; the
residual is an integer-load hoist before `fild` plus a loop-tail load order. No w2/w3 law fires. Reopen criterion unchanged.

## `_rasterizer_lens_flares_submit_occlusion_tests` (400 B, parked) — skipped, ruling-blocked

laws_w2 C3 and laws_w3 C2 both list this row as reachable only through the `_point_from_line3d` COMDAT, which WORKER_BRIEF
section 9b declares OUT as a lane rule, and laws_w3 **R14** says re-proposing that ruling is itself rejected. The w2
candidate `scratch/workers/rasterizer_lights/p1.c` (removal of this TU's `#define`/`#undef
REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`) is strict exact but fails the emitted-symbol guard. Nothing else is required: the body
is otherwise exact.

## `_rasterizer_lights_reset_for_new_map` (48 B) — owner-blocked, preserved buffer overrun

Re-verified with `alndiff` at this HEAD: the only difference is instruction 0, `push 0x7722` versus `push 0x7700`; the other
ten instructions and all five relocations are identical. January's `.bss` proves
`local_lens_flare_occlusion_test_results` is exactly `0x7700` bytes (`…results2` at 0, `…results` at 262,176,
`local_lens_flare_parameters` at 292,640, whose base is pinned at addend 0 by `lens_flare_submit_parameter_get`'s
`lea eax,[eax*8+parameters]`), so `0x7722` clears 34 bytes of `local_lens_flare_parameters[0]`. Every exact spelling is a
preserved buffer overrun, which WORKER_BRIEF section 5 forbids and laws_w2 R12 lists explicitly for this function.
Reopen criterion: an explicit owner ruling admitting a `/* BUG (preserved for exact matching): … */` overrun here.

## Evidence used

January split COFF (`build/split/source/rasterizer/rasterizer_lights.obj`) bytes, relocations, `.bss` labels and `.rdata`
texts; `tools.campaign.alndiff` and `tools/campaign/gate.py`; `scratch/parkcheck.py`; `config/parked.json`;
`scratch/workers/rasterizer_lights/` (w2 candidates D1-D4, p1, p2) and `scratch/workers/rasterizer_xbox_lights.md`
(the A35 local-first scan); HCEA DX9 reconstructions for statement shape only. No supplied binary was executed.
Worker notes: `scratch/workers/w3c_rasterizer_lights_shader_transparent_chicago_preprocessor_etc.md`.
