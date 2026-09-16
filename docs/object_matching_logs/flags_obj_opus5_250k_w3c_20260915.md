# `flags.obj` — Opus5 250K house-clean lane, wave w3c (2026-09-15)

## Outcome

Nothing landed. `source/objects/widgets/flags.c` is unchanged (`git diff --stat` is empty).

Baseline and final real-file gate (with the `_point_from_line3d` guard): **exact 14, residual 2, unwritten 0**.
The guard passes. `scratch/parkcheck.py` reports 1 park with 0 drift. The fake-match scan finds 0 leads.

| function | January padded / relocs | status | result |
| --- | --- | --- | --- |
| `_flag_render_proper` | 2112 / 84 | residual, not parked | not landed: zero-constant register-web tie (mechanism now measured) |
| `_flag_update` | 1184 / 26 | parked | no reopen: scalar stack-home ranking; A6/A33 scoping refuted |

This is the wave-w3 pass over a unit last worked in wave w2. Both rows were re-triaged against
`scratch/w2/laws_w2.md` (A1-A22) and `scratch/w3/laws_w3.md` (A23-A45) as detectors before any shape was cut.

## Duplicate-prevention record

- All eleven `flags_obj_*` ledgers were read, including `flags_obj_opus5_150k_w2_20260914.md` and the park
  ledger `flags_obj_flag_update_credible_fuzzy_park_20260903.md`.
- The w2 worker notes `scratch/workers/flags.md` were read, and **every** shape they record
  (A, Q1-Q5, R1, R2, P1, v0-v6b, H2a, S1, D1-D3) was re-measured with `alndiff` in this run. All reproduce.
- HCEA `blam/rasterizer/dx9/flag_render_proper.c` and `blam/objects/widgets/flag_update.c` were read.
  HCEA is a PPC/later build reconstruction: it declares `int vertex_count = 0;` after the two scale divides
  and before the column loop, and it has no `csmemset` of the working vertex at all, so it cannot arbitrate
  January's x86 store position.
- `git log --all -- flags.c`; `branch_sweep` (w2 record: no branch carries an exact body for either row).
- Neither function appears in the laws_w2 section D table, the laws_w3 section D table, or the brief's
  do-not-spend list.

## Law-detector pass (before cutting shapes)

| law | fires? | why |
| --- | --- | --- |
| A1 static linkage | no | no TU global participates in either residual row |
| A3 / A29 single exit | no | both functions already have exactly one bottom `return;` |
| A7 / A32 prototype packets | no | `scratch/gate4/c4013.txt` lists no site in `flags.c` |
| A4 / A22 / A31 / A35 helper routing, leaf age | no | the x87 schedule of both rows already aligns 1:1 with January |
| A23 for-increment frequency | no | no inline-decision delta (all call sites align) |
| A24 parameter-home offer | no | parameter homes `[ebp+8]`/`[ebp+0xc]` are reused identically in both builds |
| A25 join store, A28 MAX phi, A42 redundant store | no | no store-count or phi-width signature present |
| **A38** initializer / zero-store placement | **yes** (`_flag_render_proper`) | January's store sits between the outer `for`'s `cmp` and its `jle` |
| **A6 side law / A33** block scoping | **yes** (`_flag_update`) | January packs two roles into `[ebp-8]`; ours splits them |

Both detectors were measured. Both are negative; the measurements are recorded below because they refute the
laws for these rows and sharpen the two reopen criteria.

## `_flag_render_proper` — NOT LANDED

`alndiff` aligns 677 instructions against 677. All 84 relocations, the jump table and the whole x87 schedule
match. The single code difference is unchanged from w2:

| | instruction | position |
| --- | --- | --- |
| ours | `mov dword ptr [ebp-0x34], 0` | 0xa3, between `push edx` and `call _object_get_and_verify_type` |
| January | `mov dword ptr [ebp-0x34], eax` | 0x10e, in the outer-`for` preheader |

January's preheader block is

```
102  xor eax, eax
104  add esp, 0x24              ; csmemset cleanup
107  cmp word ptr [esi+0xc], ax ; x < definition->width
10b  mov dword ptr [ebp-0x2c], eax   ; x home
10e  mov dword ptr [ebp-0x34], eax   ; vertex_index home  <-- only January
111  jle 0x2e8
```

### New mechanism (two facts, measured this wave)

**1. The store is bounded by its basic block, not by the declaration point.** The w2 ledger states that "VC7
writes a local's home at its definition and never defers it". That is wrong in detail: our `short
vertex_index = 0;` store is *not* emitted at the head of the block; VC7 sinks it to the end of the entry basic
block, where it lands between the `push edx` and the `call _object_get_and_verify_type` of the block's first
statement. It simply cannot cross a call / basic-block edge. January's store is in a *later* basic block (the
loop preheader, reached only after `csmemset` returns), so January's source statement genuinely is after
`csmemset` — the declaration-initializer spelling that the production body uses is the wrong one, and the
statement spellings v1/v2/v3/v4/v6/v6b/r2 put the store in the right block.

**2. Constant-web absorption is the real blocker.** Every spelling that puts the zero in the preheader block
adds a fourth use of constant 0 to one region — `push 0` for `csmemset`, x's zero, y's zero and
`vertex_index`'s zero. VC7 then enregisters the constant, and because that web now spans the `csmemset` CALL
it has to pick a callee-saved register. The result is identical in all seven spellings:

```
e5   xor ebx, ebx                     ; hoisted BEFORE csmemset
ed   push ebx                          ; instead of January's `push 0`
108  cmp word ptr [esi+0xc], bx
10c  mov dword ptr [ebp-0x34], ebx     ; before, not after, the x home store
124  mov dword ptr [ebp-0x10], ebx     ; y, instead of January's immediate
```

677 -> 672 instructions, 2112 -> 2096 bytes. January keeps both `push 0` and `mov [ebp-0x10],0` as immediates
and lets only `vertex_index`'s home take x's EAX, i.e. in January's IL the initial value behaves as a *copy of
the induction variable's register*, not as a fourth independent constant use. Chained and comma spellings
(`for (x = vertex_index = 0; ...)`, `for (vertex_index = x = 0; ...)`) do not produce that: VC7
constant-propagates them into the same web.

### Shapes tried this wave (scratch only; the real file was never edited)

| shape | form | result |
| --- | --- | --- |
| r1 | declaration-list order: `short vertex_index = 0;` moved to last in the block decl list | **byte-identical** to baseline (2112, store still at 0xa3). Declaration order is inert. |
| r2 | A38 shape: a nested block `{ short vertex_index = 0; for (x ...) ... }` opened after `csmemset`, with `x`/`y` left in the outer block (v6b had moved them too, so this IL is new) | 2096 / 672, the identical EBX web. A38's declare-init vs assignment distinction is inert here. |

Re-measured (w2 shapes, all reproduce exactly): v1/v4 statement after `csmemset`, v2 `for (x = 0, vertex_index
= 0; ...)`, v3 `for (x = vertex_index = 0; ...)`, v6/v6b block-scoped loops — all 2096/672 with the same EBX
web; v5 (reusing `i`) additionally permutes four frame slots.

**Classification.** Zero-constant register-web tie (class D), now with a measured mechanism rather than a bare
"register tie".

**Reopen criterion (sharpened).** A measured VC7 rule that keeps a loop-preheader zero-constant use out of a
web that spans a preceding call — i.e. a source lever under which `csmemset`'s `push 0` stays an immediate
while the preheader zero is shared with the induction variable's register — or January-side source/local
records showing that `vertex_index`'s initial value is a copy of `x`. A pure *position* lever is refuted:
every position inside the preheader basic block has now been measured.

## `_flag_update` (park re-investigation, scratch only) — NO REOPEN

Target 390 instructions against 393; frame 0x300 both; all 26 relocations and their order equal. Two
separable effects, as recorded in w2:

1. **The `row` use-count web.** Ours keeps `row` in EDX after `point_physics_update` (`mov edx,[ebp-8]` at
   0x215, then `movsx ecx,dx`); January re-reads `[ebp-4]` at every use. Three instructions.
2. **A scalar stack-home permutation.** Target shares `[ebp-8]` between the `fild` conversion temp
   (0xa6/0xae/0xb1/0xbb) and a zero-initialised scalar (0x141); ours splits those over `[ebp-0x10]` and
   `[ebp-0xc]`, and -4/-8/-0xc/-0x10/-0x14 are permuted throughout.

### The one law that fired, and its refutation

A6's side law ("same-block locals never share a cell, but nested-block locals may pack") together with A33
(declare and initialise inside the block that uses it) is the only w3 law whose signature is present: January
packs two roles into `[ebp-8]` and we do not, and `short neighbor_count = 0;` is declared at the row-loop-body
level although its only uses — `neighbor_count++`, `estimated_positions[neighbor_count].{x,y,z}` and
`for (i = 0; i < neighbor_count; i++)` — are all inside the `if (column != 0 || ...)` relaxation block.

| shape | form | result |
| --- | --- | --- |
| f1 | `short neighbor_count = 0;` moved into the relaxation block, declared after `inverse_weight` | 1168 (size regression), 383 vs 390 instructions |
| f2 | same, declared before `estimated_positions` | identical to f1 |

Both shapes move the `parent_deltas` const array in the frame (-0x2c -> -0x28) and thereby break the four
`mov word ptr [ebp-0x2c/-0x2a/-0x28/-0x26]` initialiser stores that the parked body currently reproduces
exactly. **Negative result worth keeping: this proves January declares `neighbor_count` at the row-loop-body
level, i.e. the parked body's scoping is January's, and the `[ebp-8]` sharing in January is allocator packing,
not a nested-block declaration.** A6/A33 is refuted for this function.

**Park decision.** Unchanged; not proposed for reopening. No section-9 / w2 / w3 law applies: frame size,
relocations and their order, x87 phi and operand order, inline schedule, single exit and now declaration block
scoping all already equal January.

**Reopen criterion (parked.json, plus w2 and this wave).** A measured VC7 rule for scalar stack-home *ranking*
whose input is neither the emitted reference census (refuted in w2) nor declaration block scope (refuted
here), or January-side evidence for an extra or removed `row` reference in the relaxation path.

## Proposals

None. No header, symbols.json, parked.json or other config change is needed or proposed by this wave.

## Checks

- final real-file gate with the guard: exact 14, residual 2, unwritten 0 — identical to baseline, row for row.
- `git diff --stat -- source/objects/widgets/flags.c`: empty.
- `scratch/parkcheck.py`: `OK _flag_update 1184 26`, parks 1, drift 0.
- `tools/fake_match_scan.py source/objects/widgets/flags.c`: 0 review leads.
- owner census: no source change, so the candidate owner set is the base owner set.

## Scratch

- Notes: `scratch/workers/w3c_flags.md`
- Candidates: `scratch/workers/w3c_flags/{r1,r2,f1,f2}.c` with their `.obj`
- w2 candidates re-measured in place: `scratch/workers/flags/v0..v6b`, `scratch/workers/flags/w2/`
