# `collision_debug.obj` - opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/physics/collision_debug.c`, target `build/split/source/physics/collision_debug.obj`.
- Compiler VC7 13.00.9254 (XDK), project flags `/nologo /c /O2 /Oy-` (no per-file override in `config/config.json`).
- Lane `opus/100k-consolidated-20260914` (base `c3e257e9a`); worker owns only `collision_debug.c`; headers,
  `config/symbols.json` and `config/parked.json` frozen.
- Evidence used: January target COFF + Capstone streams (`tools.campaign.alndiff` / relocdiff); Ghidra
  `scratch/ghidra/out/collision_debug.obj.decomp.c` (hypothesis only); the January map atlas
  `scratch/atlas/collision_debug.obj.txt` is EMPTY for this object and the PDB is public-only, so no local/static
  names exist. HCEA supplies only the hs external globals; no render body in any donor (see
  `collision_debug_obj_jonas_public_donor_audit_20260826.md`).
- Starting body: LWSS `d057c98dc` (#52), reconciled in `5d3444f79`. The 2026-08 closeout ledgers describe an older body
  (E2-E10 matrix) and are superseded as a baseline, not as negative results.

## 2. Validated baseline (lane HEAD, real file unchanged at finish)

| row | target padded / meaningful / relocs / nsha16 | base size / relocs / nsha16 | tag |
| --- | --- | --- | --- |
| `_collision_debug_render` | 4192 / 4182 / 278 / `5a365dfa90e8300c` | 4112 / 275 / `5cb9eb28c70bd40b` | residual `[size, relocs, sha]` |

Gate: `== exact 0 residual 1 unwritten 0`. Data: BSS 28962, DATA 16, cube RDATA 96 already equal. No parked entries.
First meaningful divergence at baseline: `+0x8b` (callee-saved push placement / register permutation), then the
relocation-bearing structural gaps listed below.

## 3. Findings retained in the best candidate (NOT landed - function is not strict exact)

Best candidate: `scratch/workers/collision_debug/best.c` (== `s1.c`), 4160 / 277, nsha16 `e90ada4d0dcbfea0`,
frame `0xb528` (= January), `_point_from_line3d` guard clean, fake scan 0 leads. Each shape below is proven by
January bytes:

1. **Three separate static spray arrays**, not one aggregate. January's spray code addresses the BSS region with
   three independent relocation addends in each loop (`lea [idx12+sym+0]`, `lea [idx12+sym+0x3840]`,
   `[word*4+sym+0x7080]`: 2/2/3 refs). An aggregate lets VC7 fold normals and points into one pointer IV (1/1/2 refs,
   measured on the same loop source, `s4.c`). Separate `real_vector3d normals[1200]`, `real_point3d points[1200]`,
   `unsigned long hit_flags[38]` give the 2/2/3 multiset and exactly January's BSS offsets (0x0, 0x3840, 0x7080,
   width 0x7118). Requires the symbols.json split proposed below; until then the three relocations differ by name only.
2. Assert text `count<=14` (January `.rdata` `??_C@_09PCOGIILP@count?$DM?$DN14?$AA@`, line 297).
3. Pill/sphere branch: `if (collision_debug_height <= 0.0f)` sphere else pill (call order + `jp` after `test ah,41h`);
   inputs are local copies of `debug_point`/`debug_vector`; `count++` before and after
   `collisions[count].point = clipped_position`; loop `collision_index < count` (16-bit compare).
4. Vector-test miss: `render_debug_line(TRUE, &debug_point, &collision.point, green)` + point at `collision.point`.
5. `debug_vector = collision_debug_vector;` then in-place `*= length` per component.
6. Phantom point, spray centre and features centre use `v*t + p` operand order (the `point_from_line3d` shape),
   hand-expanded: the header inline would emit a forbidden `_point_from_line3d` COMDAT (and a new
   `_scale_vector3d` owner) that January's object does not have.
7. Point spray (length <= 0): `centre +/- 0.125f` bounds, `<=` loop tests, radius `0.0625f`.
8. Ray spray write loop: explicit 30/40 downcounters; `spray_index = row*COLUMNS` per row with `++`; arrays indexed by
   `spray_index`; bit vector indexed by `row*COLUMNS + column` (January computes the shift as
   `(row*8 - row*40) + index`, VC7's `&31` narrowing of `row*40+column`). This reproduces January's write loop
   instruction for instruction.
9. `real_rectangle2d projection_bounds` in its own inner block (January shares its slots with `direction`); fixes the
   frame from `0xb534` to `0xb528`.
10. `surface->plane_designator & LONG_MIN` (January `cmp [eax],0 / jns`, surface pointer homed), as in
    `breakable_surfaces.c:75`.
11. Features: `center.z = v.k*0.5f + p.z; center.z += collision_debug_height*0.5f;`.

## 4. Experiment matrix (all under `scratch/workers/collision_debug/`)

| id | shape | size / relocs / nsha16 | result |
| --- | --- | --- | --- |
| v0 | lane HEAD body | 4112 / 275 / 5cb9eb28 | baseline |
| v1 | items 2-7 + for-loop spray with `row*COLUMNS+column` everywhere, aggregate | 4080 / 277 / 901211ac | closer structure |
| v2 | v1 + separate arrays | 4096 / 277 / 769910f9 | reloc multiset = January |
| e0 | downcounters, spray_index for bits+arrays | 4128 / 277 / 3a735245 | write loop IVs right, shift direct |
| e1 | downcounters, expression everywhere | 4144 / 277 / 3fec840c | all IVs continuous |
| e2 | item 8 | 4176 / 277 / b1701373 | write loop == January |
| e3 | per-iteration `spray_index = row*COLUMNS+column` | 4096 / 277 / 071e7bcd | all continuous |
| rg/xe | continuous spray_index in render loop | 4192 / 277 / 50d667a7 | size equal, IVs inverted |
| f1 | + item 10 | 4160 / 277 / 27f98537 | jns + homed surface |
| f2 | + item 11 | 4160 / 277 / 9a5eda74 | features centre fixed |
| s1 | + item 9 | 4160 / 277 / e90ada4d | frame 0xb528 (best) |
| s2 | s1 with `flags = 0; SET_FLAG(...front...)` | identical bytes to s1 | inert |
| s3 | swap horizontal/vertical declaration order | identical skeleton | inert |
| w2 | `column*h + row*v + base` operand order | identical | inert |
| h1/h3/h4 | flags/ignore declaration order or function scope | identical | inert |
| s4 | s2 loops with aggregate struct | 4144 / 275 | refutes aggregate |
| render sweep | 21 generated render-loop forms (downcounter/for x none/reset/continuous index x bit/array index spelling x increment order) + gr, xa, xc, ri, short/unsigned/word counters | best skeleton diff 11 of 52 | none reproduces January's render loop |

No sibling rows exist; `_point_from_line3d` guard passed for every candidate.

## 5. Do-not-repeat list

- Aggregate spray struct with any loop form (pointer-IV fold, 275 relocs).
- `point_from_line3d` / `scale_vector3d` header inlines in this TU (forbidden/extra COMDAT owners).
- Declaration-order swaps of the step vectors, flags or ignore index; flags initialisation spelling; commutative operand
  order of the direction terms: all measured inert.
- The 21-form render-loop sweep above and the short/unsigned/word counter types.
- The E2-E10 matrix of the 2026-08 ledgers (older body) remains exhausted.

## 6. Residual classification (best candidate s1)

- **ownership/relocation (blocked, config):** the three spray relocations need separate BSS symbols (measured fact:
  separate arrays reproduce January's reloc multiset and BSS offsets).
- **ABI/register (global tie, unresolved):** January assigns flags=esi (home -0x10), ignore index=ebx (home -0x4c),
  cube counter=edi; ours ebx/edi/ebx. Measured consequences: flags home ranks at -0x3c instead of -0x10 (4-byte slot
  shifts), the sphere/pill argument pushes hoist ahead of the branch (the missing 8th `_collision_debug_width`
  relocation, 277 vs 278), width reload placement after `collision_test_sphere`, and the phantom-flag test
  scheduling. Inference: this is one allocator decision; bisection shows it flips only when the vector-scaling form
  (item 5) and the spray rewrite are both present, so it is not attributable to a single statement.
- **source/topology (unresolved):** render loop. January: word index reset per row from a row*40 IV, 12x offset
  continuous, shift `(row*8 - row*40) + index` via a stored difference. Our closest forms either keep the direct shift
  (e0) or create a separate `row*8+column` IV with a continuous word index (e2).
- **scheduling (coupled):** direction-term FP evaluation order in the write loop follows the step-vector slot ranking.

## 7. Reopen criteria

- After the orchestrator lands the symbols.json split below: resume from `best.c`; the remaining work is the render
  loop IV form and the flags register choice. Do not reopen for further render-loop spelling permutations without a
  new hypothesis for why January's render loop narrows the shift through the row*40 IV while no column variable is
  materialised.

## 8. Disposition

NOT landed. The production file is unchanged (`git diff --stat` empty); final gate
`residual 4192 _collision_debug_render [size 4112!=4192, relocs 275!=278, sha]`, guard passed, owner census
identical to `build/base`, fake scan clean. No credit.

### ORCHESTRATOR-PROPOSAL (config/symbols.json)

Replace line 22607 `{ "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_globals" },` with:

```
{ "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_normals", "static": true },
{ "file_offset": 4564736, "flags": 0, "name": "_collision_debug_spray_points", "static": true },
{ "file_offset": 4579136, "flags": 0, "name": "_collision_debug_spray_hit_flags", "static": true },
```

Evidence: section 3 item 1. Names are descriptive reconstructions (no atlas/PDB/HCEA name exists; the current
`_collision_debug_spray_globals` is equally invented). Consumer: only `source/physics/collision_debug.c` (the
declaration and the spray loops); must be paired with the source change to three static arrays (the split alone
leaves the current aggregate `collision_debug_spray_globals` as a candidate-only BSS owner). The function stays a
residual either way until sections 6 (register) and render-loop items are solved, so the split has no exact credit
by itself; its value is removing the naming blocker so a future byte-exact candidate can gate strictly.
