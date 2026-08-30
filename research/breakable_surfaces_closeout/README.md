# breakable_surfaces closeout evidence

Reproducible evidence for the `_breakable_surface_effect` reconstruction
recorded in `docs/object_matching_logs/breakable_surfaces_obj.md`.

## 2026-08-30 part 2: the origin-mirror source recovery

Commit `fc47f9a15a0aea7f0081394c4d66db5c68ab5323` on the repository's own
`origin` remote (github.com/punpckhdq/halo) contains a
`source/physics/breakable_surfaces.c` from an earlier reconstruction lineage
that preserved the historical topology this repository had lost. It is NOT an
authentic Bungie blob (it carries `match_assert` and a `// TODO: doesn't
match` in its own headers), so it ranks as level-5 hypothesis evidence — but
its shapes were byte-verified here one at a time and all 11 sibling
functions gate strict-exact from its text unchanged.

Verified January shapes recovered from it (all now in production source):

- `real_point2d jitter` in an inner block, both products passed as
  expressions into `point_from_line3d` (explains the never-written
  `[ebp-0x1DC]` frame slot: it is `jitter.y`, elided but allocated);
- `struct new_particle_data particle` declared INSIDE the convex-hull block
  (fixes the particle-vs-temp-cluster frame placement wholesale);
- natural scoping instead of the E09 union: `surface_vertices2d` at while
  scope, `position_2d_test` in the s-loop, `s_normal`/`t_normal` in the
  first-vertex arm, `temp_2d` in the seed arm, `sound_location` plain in the
  epilogue block (the union was a compensating reconstruction device);
- `real u = real_local_random();` in a bare block for the
  `rgb_colors_interpolate` argument;
- `(tint_upper - tint_lower) * real_local_random()` alpha operand order
  (independently derived from the x87 popping form the day before, then
  found verbatim in the mirror);
- `rectangle2d bounds` naming, `real_point3d const *vertex_point` lvalues,
  `fast_ftol(ceil/floor(PIN(...)))` at the four conversion sites.

Two mirror shapes were REJECTED by January's bytes: its
`projection_from_vector3d` helper call (January open-codes the fabs block —
`test ah,1`/`jne` comparisons prove our spelling, and the attested assert
line anchors 266->348 span exactly 82 physical lines only with the
open-coded block) and its shared `bsp3d_get_plane_from_designator`
(January's `jns` at 0x177 proves our private helper's `& LONG_MIN` test).

Line-anchor validation: January's four in-function assert line numbers
(251/266/348/388, attested by the pushed constants in the binary) span
15/82/40 lines; the reconstruction spans 16/82/40 — byte-perfect spans in
the whole body, one line still unaccounted in the declaration head.

## witness_mirror_fast_ftol_20260830.c

Self-contained analysis-only witness (owner rule: no `__asm` in
production).  Under the UNMODIFIED production headers it measures
**20 differing instruction lines** against January -- 1156/1156
instructions, 4032/4032 padded bytes, 117/117 relocations, frame `0x1240`,
and every stack home identical:

```
python tools/campaign/gate.py source/physics/breakable_surfaces \
    --source research/breakable_surfaces_closeout/witness_mirror_fast_ftol_20260830.c \
    --fn _breakable_surface_effect --disas _breakable_surface_effect
```

### CORRECTION (2026-08-30, session 3): the "4 instructions" figure was wrong

An earlier pass in this session reported the witness at 4 differing lines
with a `dot_product3d` spelled `a->i*b->i + (a->j*b->j + a->k*b->k)`, and
claimed the whole-board impact of that header edit was nil.  **Both claims
were false.**  The board check that produced "BOARD IDENTICAL" was run
after the header had already been reverted, so it never tested the edit.

Re-measured properly, with a full `ninja` rebuild between header state and
board:

| `dot_product3d` body | this function | whole board |
| --- | ---: | --- |
| `a->i*b->i + a->j*b->j + a->k*b->k` (production, flat) | 20 lines | **277 objects / 4822 fns** |
| `a->i*b->i + (a->j*b->j + a->k*b->k)` (grouped) | 4 lines | 271 objects / 4805 fns — **17 functions lost** |
| k,j,i sequential accumulation | 4 lines | 271 objects / 4787 fns — **35 functions lost** |

Both "fixes" are refuted by tree-wide evidence: they break
`source/math/real_math` itself (79 -> 74 and 79 -> 69 exact functions),
plus objects, items, vehicles and units.  The flat form is *proven* by
those functions.  Any local win from a shared-header edit is meaningless
until the whole board is rebuilt and diffed -- record the board numbers,
not just the local diff count.

The sequential-accumulation form deserved the test: it is the
campaign-proven lever documented in the acceleration playbook (it closed
`collision_prism_test_vector`).  Here it is simply wrong, and the board
says so unambiguously.

### The true residual: five commutative-order sites

All 20 lines are one phenomenon -- VC7 and January order commutative FP
operands/terms differently at five sites, with identical instruction
counts and identical stack homes:

* **Four `plane3d_distance_to_point` expansions** (0x3fc, 0x426, 0x45b,
  0x47f).  Both sides start the sum with the same term; January then adds
  the j term and finally the i term, ours adds i then j.  Example at
  0x3fc: January `fld [-0x1c]; fmul [esi+4]` then `fld [-0x20]; fmul [esi]`;
  ours has those two products in the opposite order.
* **One `cross_product3d` expansion** (0x37b/0x389).  For the two products
  containing `s_normal.k`, January loads the *other* operand
  (`fld [-0x68]; fmul [-0x20]`); ours loads `s_normal.k` first.

### Measured negatives in the TRUE (flat-dot) basin

Micro-probes (`scratch/micro.c`, `scratch/probelab.py`) isolate the tie in
a 96-byte function: `_micro_m6` reproduces our exact behaviour -- products
containing `a.k` left-loaded, all others right-loaded -- so the default
VC7 rule is *right-operand-loaded* and the a.k products are the deviation.
`_micro_m1`/`m2` prove the choice is NOT driven by the definition order of
the two vectors.

Inert (no change to the operand roles): every operand text order inside
`cross_product3d` (VC7 canonicalizes commutative multiply operands),
2-temp vs 3-temp vs direct-to-`result`, decl/init split, named `a->k`
local, named per-product locals, per-component accumulation, negated
components, `scale_vector3d` store order (i,j,k is optimal; every other
permutation is 20-410 lines worse), `plane3d_from_point_and_normal`
argument order / read-`normal`-vs-copy / d-before-n, caller statement
order, nesting the cross into the plane construction, and reversed cross
arguments (13 lines, worse).

### C2 debugger mapping against micro_m6 (2026-08-30, session 3)

Target: `probes/m6only.c`, a single-function TU whose entire FP output is
the six cross products, so every FP hit is relevant.  It emits the tie at
function offset `0x18`: `fld [ebp-0x10]` (a.k) / `fmul [ebp-8]` (b.j),
where January loads b.j instead.  Config: `tools/c2dbg32/gen_config_cross.py
--src scratch/m6only.c <VAs>`.

**Mapped (measured, not inferred):**

* **The FP path on this compile runs through the byte emitter
  `0x107455e6`** (76 hits over ~4 passes), called from `0x10744452`.
  At each hit `edi` points at a live IR node.
* **IR nodes confirmed on this path**: opcode at `node[+4]` (`0x24e` seen,
  in the documented 0x249-0x25a FP band), flags `node[+8] = 0x10040002`
  for operand nodes, and an operand/leaf pointer at `node[+0x18]`.  The
  leaf nodes for the vector members are **0x60 apart** (one node stride) —
  e.g. `...48dc`, `...493c`, `...499c` — so the members occupy consecutive
  leaves, matching the documented stride.
* **The assembled function lives in the arena and is findable by pattern
  scan.**  Scanning for the prologue `55 8b ec 83 ec 18` finds the emit
  buffer (`buffer+0`); scanning for the tied pair `d9 45 f0 d8 4d f8` finds
  it at **`buffer+0x18`**, exactly the function offset.  So the emitted
  bytes for the decision are directly addressable at runtime.
* **Determinism**: node and buffer *offsets* are identical across runs;
  only the arena base shifts (~1 MB per run).  A watchpoint therefore needs
  a runtime-resolved address, which dbg32 cannot yet do (it programs DR0-3
  when C2 loads).

**Negatives (do not repeat):**

* **Every previously-mapped FP decision site takes ZERO hits here** —
  `0x10745628` (FP jump table), `0x10744304` (FP modrm formation),
  `0x10735135` (per-node codegen).  Those were mapped from an ai_debug
  compile; they are path-specific, not general.  Live on this path instead:
  arena `0x10701000` (26), byte emitter `0x107455e6` (70-76), interference
  `0x1070943b` (30), preferred-register push `0x1075fa55` (39),
  width-class write `0x10715873` (53).
* **The emitter's hits are NOT 1:1 with FP instructions.**  Comparing the
  observed `ebx` sequence against the disassembly's operand displacements
  matches only 4 of 12 positions.  An early reading of this session that
  took hit 1 / hit 2 as the tied `fld` / `fmul` pair, and diffed their
  nodes, was therefore **invalid and has been discarded**.  Establishing a
  real hit-to-instruction correlation is the prerequisite for any node
  differential.

**Concrete next step.**  Add a "resolve the watch address at run time" mode
to `dbg32` (either `watch [reg+off]` captured at the Nth hit of a
breakpoint, or `watch <scan-hit>+<off>`), then watch the displacement byte
at `buffer+0x1a`.  Its `writer_eip` is the byte-level operand encoder; walk
back from there to the lowering site that chose which operand becomes the
load.  That is the decision this residual turns on, and it is one
instrument away rather than a blind search.

### Earlier debugger state (superseded by the section above)

`gen_config_cross.py` points dbg32 at the probe compile.  Profiled hit
counts on this path: arena `0x10701000` 26, byte emitter `0x107455e6` 70,
interference test `0x1070943b` 30, preferred-register push `0x1075fa55`
39, width-class write `0x10715873` 53; and **zero** on the mapped FP sites
`0x10745628` / `0x10744304` / `0x10735135`, which were mapped from an
ai_debug compile and do not fire here.  So the FP operand-order decision
is made somewhere not yet mapped; the byte emitter and the allocator are
live and are the anchors for the next search.  `_micro_m6` is the correct
target for that work -- 96 bytes, exhibits the tie, compiles in a second.

## witness_fast_ftol_stack_20260830.json (superseded)

The 2026-08-30 part-1 edit-stack witness against the pre-mirror production
source. Historical: its W3/X1/X2/X3 recoveries are all subsumed by the
mirror topology (X3 verbatim-confirmed, X1 as `real u`, X2 superseded — the
mirror's plain sound test matches January in the new basin; W3 as `jitter`).
