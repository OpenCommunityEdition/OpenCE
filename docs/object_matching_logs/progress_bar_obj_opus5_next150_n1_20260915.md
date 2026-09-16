# `progress_bar.obj` opus5 next-150K lane, wave n1 (2026-09-15)

Worker `n1:hud_draw_interface_etc`. Unit `source/interface/progress_bar.c`. Baseline `ae10935da`.
Real file UNCHANGED (`git diff --stat` empty). Notes: `scratch/workers/n1_hud_draw_interface_etc.md`.

## Baseline / final (real file)

| gate | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline | 52 | 1 | 1 |
| final | 52 | 1 | 1 |

No parks in this unit; owner census clean; fake scan 0 leads; `branch_sweep.py` finds nothing better in any of
the 19 historical blobs (all are 52/1/1 or worse, every one with the same `sizedelta 16`).

## `_draw_gravy_layer` (352 vs 336) — NOT LANDED, structural census recorded

January 352 padded / 349 meaningful / 22 relocations / sha `a9509aab99edb2bd`; ours 336 / 22.
Target 125 instructions, ours 117 + 4 pad nops.

### Frame census — there is no frame gap

January `sub esp,0x1c`; ours `sub esp,0x1c`. **Identical.** No missing or extra local.

| slot | contents | notes |
| --- | --- | --- |
| `[ebp-0x1c]` … `[ebp-0x10]` | `rect.x0 / y0 / x1 / y1` | filled by `generate_gravy_rect`; ours re-reads `x0` at `+0x127` for vertex 4 |
| `[ebp-0xc]` | `half_width` | last read `+0xc8`; **TARGET ONLY** re-stored `+0xe4` with `x1` (dead) |
| `[ebp-8]` | `half_height` | last read `+0x7e`; **TARGET ONLY** re-stored `+0xa3` with `y0` (dead) |
| `[ebp-4]` | `0.f - mask_position` | last read `+0x73`; **TARGET ONLY** re-stored `+0xa6` with `x0` and RELOADED at `+0x13b` for vertex 4 |
| `[ebp+0xc]` | parameter `progress` → `mask_position` → `640.f - mask_position` | **TARGET ONLY** re-stored `+0x11b` with `y1` (dead) |

Parameters (both builds, static custom convention): `layer`=EAX, `color`=ECX, `alpha`=`[ebp+8]`,
`progress`=`[ebp+0xc]`.

### Relocation census — 22 vs 22, nothing missing or extra

Identical multiset, order and per-instruction placement: `__real@3f000000` ×2, `_generate_gravy_rect`,
`_D3DDevice_Begin@4`, `_D3DDevice_SetVertexData4f@20` ×5, `__real@44400000`, `__real@42800000` ×2,
`__real@44200000`, `_D3DDevice_SetVertexData2f@12` ×10, `_D3DDevice_End@0`. The only listing difference is the
`symbol:` vs `defined-noncode:.rdata:` spelling of three `__real@` literals, a csplit labelling artefact.

### Derived defect

The whole 16-byte gap is eight instructions of one family. January materialises each rectangle corner as a
live VALUE — load into a scratch register, `mov <carrier>,<scratch>`, and a store into a recycled dead slot —
whereas ours consumes `rect.<field>` as a memory operand and re-reads `[ebp-0x1c]` for the fourth vertex.
Only the `x0` store is ever reloaded, so the other three are dead spill stores VC7 left behind after
coalescing removed their reloads. Concretely January has four cross-call live values at vertex 1
(`esi`=−mask, `edi`=half_height, `ebx`=y0, `eax`=x0) and must spill `x0` because EAX is caller-saved; ours has
three.

### Shapes tried (6, all scratch)

| shape | result |
| --- | --- |
| `gravy_S1` (w1) named corner locals assigned after `generate_gravy_rect` | 336 / 121 stream entries — identical to base |
| `gravy_S3` (w1) `struct gravy_rectangle2d *screen_rect = &rect;` | identical |
| `gA` four corner locals declare+init inside a nested block around the whole draw | identical |
| `gC` two reused locals `real x, y;` re-assigned before each vertex | identical |
| `gD` whole-aggregate copy `struct gravy_rectangle2d screen = rect;` | identical |
| `gH` `static void draw_gravy_vertex(real x, real y)` called 4× (inline parameter homes) | identical |

VC7 13.00.9254 forwards every spelling of a corner copy back to the `rect` memory operand — through a named
local, a pointer alias, a nested block, a two-variable rotation, a whole-aggregate copy and an inlined
helper's parameter homes. No admissible C source in this TU makes `x0` a live virtual register.

**Classification: structural-size / allocator rematerialisation.** The census shows the gap is entirely spill
materialisation, not a missing statement, call, local or control-flow arm.

**Reopen criterion:** a measured VC7 lever that keeps a local initialised from an address-taken local struct
alive through copy propagation without `volatile` or barriers (none exists in laws_w2 A1–A22 or laws_w3
A23–A45), or January-side evidence that `rect` is written between the vertices.

## `_code_000d16d0` (16 / 1) — EXACT-PENDING-RENAME, still owner-gated on its caller

The w1 body `scratch/workers/tiny_stubs/pb_v1.c` was re-measured at this baseline and is unchanged:

```text
python -B tools/campaign/gate.py source/interface/progress_bar --source scratch/workers/tiny_stubs/pb_v1.c \
  --all --alias progress_bar_create_noise_texture=code_000d16d0
== exact 53  residual 1  unwritten 0   (every other row unchanged)
```

Without the alias the same file gates 52/1/1, i.e. the body is byte-exact and only the COFF **name** differs.
New corroboration found this wave: `scratch/atlas/progress_bar.obj.txt` (the January debug `cachebeta.exe` map
atlas) independently names `0x000d2460` `_progress_bar_create_mask_texture` — our
`_progress_bar_build_glow_texture` — so the create-noise / create-mask pair that the two 2001-09-25 Xbox maps
show is January-attested on the mask side.

The part `progress_bar_obj_base_layer_reconciliation_20260907.md:142` actually forbids — inventing the caller —
is still open: the call site inside `progress_bar_setup` is byte-invisible. Needs an owner ruling plus the
`config/symbols.json` rename (see the orchestrator proposals in the worker notes).

## Names-only evidence recovered from the January atlas (zero bytes)

`scratch/atlas/progress_bar.obj.txt` gives authentic January map names for four of our reconstructed
identifiers. Renaming is a `config/symbols.json` + identifier change, so it is an orchestrator packet:

| our name | January map name | file offset |
| --- | --- | --- |
| `_draw_gravy_layer` | `draw_layer_int` | `0x000d1770` |
| `_set_blur_texture_coordinates` | `do_convoluation_coords` | `0x000d19c0` |
| `_draw_fullscreen_blur` | `draw_fade_layer` | `0x000d1a60` |
| `_progress_bar_build_glow_texture` | `progress_bar_create_mask_texture` | `0x000d2460` |

The atlas decorations (`@name@N`, and the C++-mangled rows in the sibling `interface.obj.txt`) come from other
builds keyed by January address; only the NAMES transfer, never the calling convention.
