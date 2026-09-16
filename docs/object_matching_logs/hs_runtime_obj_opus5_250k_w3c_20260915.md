# `hs_runtime.obj` second pass, Opus5 250K lane wave w3c (2026-09-15)

## Scope and baseline

- Unit `source/hs/hs_runtime`. Worker-owned file: `source/hs/hs_runtime.c` (the only file this wave could modify).
- Baseline real-file gate with the `_point_from_line3d` guard: **61 exact / 5 residual / 0 unwritten (of 66 listed)**.
  The guard passed. There are **no UNWRITTEN rows** in this unit, so the brief's first work priority is empty.
- All five residuals are locked in `config/parked.json`:

  | function | January padded | gate tag | park class |
  | --- | ---: | --- | --- |
  | `_hs_inspect_enum` | 144 | reloc-identity | csplit-relocation-alias |
  | `_hs_evaluate_begin_random` | 528 | sha | private-register-convention |
  | `_hs_global_reconcile_read` | 1408 | sha | instruction-scheduling |
  | `_hs_global_reconcile_write` | 400 | size 416!=400, sha | instruction-scheduling |
  | `_render_debug_trigger_volumes` | 1088 | sha | unclassified |

- **Outcome: nothing landed.** Final real-file gate is 61 / 5 / 0 with a row listing byte-identical to baseline,
  `git diff --stat -- source/hs/hs_runtime.c` is empty, park drift is 0, and `fake_match_scan` reports 0 leads.
- w1's landing (`_hs_evaluate_inequality`, 528, via the TU-local `HS_EVALUATE_INEQUALITY` macro) is already in HEAD
  and is not re-claimed here.

Ledgers and notes read first: every `hs_runtime_obj_*` ledger (w1 150K, debug_scripting_pair, the four
`jonas_*`, opus_reconciliation), `hs_runtime_inequality_reconciliation_20260908`,
`hs_runtime_interface_leaf_reconciliation_20260908`, `small_unwritten_threshold_wave_main_hs_ai_20260913`,
the 100K handoff, `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, and `scratch/workers/hs_runtime.md`
(w1's per-function blocks: 9 shapes on the reconcile pair, 7 on trigger_volumes, 2 on begin_random, 0 on inspect_enum).
`branch_sweep` shows no blob better than the current file.

## Law-as-detector pass (brief sections 9b / 9c)

Every w2/w3 law was run as a detector over all five rows before any shape was chosen.

- **A1 static-linkage — refuted, do not spend.** `_hs_global_data`, `_hs_thread_data` and `_hs_syntax_data` are UNDEF
  externals in January's `hs_runtime.obj`: 61 exact rows (including `_hs_runtime_initialize`, which *stores* to
  `hs_global_data`) match with `extern` relocation identity, so a `"static": true` split would break them.
  Consistent with the HEAD static census warning in brief 9b.
- **A7 / A32 missing prototype — measured inert.** The unit's only C4013 sites (`scratch/gate4/c4013.txt`:
  hs_runtime.c(1073) `object_list_gc`, (1075) `hs_node_gc`) are both inside `_hs_runtime_update`, which is **already
  strict EXACT (176)**. Probe `d1.c` (real file + the two owner-shaped declarations at include position) leaves all
  66 gate rows byte-identical. See "Orchestrator observations" below.
- **A21 `= {0}` emission** and **A22 / A35 leaf age** both fire on `_render_debug_trigger_volumes` and are already the
  recorded blockers there (see that section).
- **A45 csplit alias boundaries** fires on `_hs_inspect_enum` and is exactly its recorded park class.
- **A3 / A29 single-exit, A5 / A33 alias removal, A6 frame recipes, A10 conditional initializer, A12 hoist order,
  A13 / A23 inline frequency, A20 store order, A24 parameter-home offer, A28 MAX-phi width, A31 constructor routing,
  A36 float-precision constant, A38-A44** — no detector fires. The four sha rows are aligned instruction-for-instruction
  with January (342/342, 191/191, 625/625, 177 code instructions + padding) and none of the residual hunks is of those
  kinds.

## `_hs_global_reconcile_write` + `_hs_global_reconcile_read` — NOT LANDED (one shared mechanism, 1,808 B)

Both functions were re-measured with `alndiff`. The entire residual of both is **one instruction's position**.

- **read**: 625 target / 625 candidate instructions; every hunk after the prologue is the same instruction displaced by
  one byte.
- **write**: target code extent `0x190` with no padding; ours `0x194`. Twelve of the thirteen "extra" candidate
  instructions are trailing `nop` padding and the last is jump-table alignment, so the 416-vs-400 `[size]` tag is
  **one code byte plus three bytes of table alignment**, not a structural gap. The 39-case jump table maps onto the
  same 8 cross-jumped blocks in both builds (w1), so the case bodies already reproduce January.

January's write prologue:

```
test bh,bh / jns end / push esi / mov eax,ebx / and eax,0x7fff / push edi / push eax
    / mov eax,[_hs_global_data] / push eax / call _datum_get / mov ecx,ebx / and ecx,0x7fff / push ecx
```

Ours:

```
test bh,bh / jns end / mov ecx,[_hs_global_data] / push esi / mov eax,ebx / push edi / and eax,0x7fff
    / push eax / push ecx / call _datum_get / mov edx,ebx / and edx,0x7fff / push edx
```

Identical instruction multiset; January materialises the array pointer at its own push (reusing EAX, 5-byte `A1`
encoding), while VC7 hoists it to the top of the `if` block into ECX (6-byte `8B 0D` encoding), which in turn pushes
the second masked index into EDX. Nothing else in either function differs.

HCEX `HCEX.pdb` gives `static void hs_global_reconcile_read(short)` / `...write(short)` with parameter
`short global_designator`; w1 measured every `short` spelling (v2/v3/v5/v7/v9) and each either adds a `movsx` +
`test ah,ah` January does not have or returns to the hoisted schedule.

### Shapes tried this wave (3 new; 12 across all waves)

| shape | form | result |
| --- | --- | --- |
| `r1` | move the seven file-local `extern` globals (hs_global_data, hs_thread_data, hs_syntax_data, hs_external_global_count, hs_type_sizes, debug_scripting, hs_debug_data) from line 547 to include position, testing the declaration-position oracle | all 66 rows byte-identical (inert) |
| `u1` | TU-local typed accessor macro `hs_global_get(global_index)` matching the file's existing `hs_thread_get` / `hs_syntax_get`, used at both reconcile sites | all 66 rows byte-identical (inert) |
| `u2` | swap the two local declarations (`external` before `global`) in both functions | all 66 rows byte-identical (inert) |

w1's nine: long index local; short param + `TEST_FLAG(short)`; short + `(word)` test; block-scoped initialised
declarations; short + `&0x8000`/`&0x7fff`; word + unparenthesised mask; short + `(word)` test + unparenthesised mask;
`datum_get((hs_global_data), ...)` paren probe; short + `designator<0`.

**Classification: instruction-scheduling tie** — the sink position of a rematerialisable global load inside one cdecl
argument sequence. No w2/w3 law addresses it (A12 preheader order and A31 aggregate flush are the nearest and neither
applies), and 12 spent shapes with a clear verdict put this in the brief's SKIPPED-EXHAUSTED class.

**Reopen criterion (restated with the new byte proof):** a same-compiler donor, or January-side local records, showing
`datum_get(<global array pointer>, <param> & <mask>)` whose array-pointer load is *not* hoisted to the top of the
enclosing block. Closing it closes both functions at once (1,408 + 400 = 1,808 padded bytes).

## `_render_debug_trigger_volumes` — NOT LANDED (1,088 B)

342/342 instructions, `sub esp,0x120` in both, 34 relocations in both; 134 differing instructions of which **130 are
pure stack-slot renames** and **4 are x87 operand order**.

### New evidence: the complete slot map of both builds

January, from EBP: `sides[2]` -0x18..-0x01 (one 24-byte object), `local_extent` -0x24, `world_extent` -0x30,
`points[4]` -0x60..-0x31, then four 4-byte spills -0x64 (the constant 6), -0x68 (`volume`), -0x6c (`volume_index`),
-0x70 (`&scenario->trigger_volumes`), then `center` -0x7c..-0x71, `ray` -0x88..-0x7d, the `hs_debug_data` index spill
-0x8c, `color` -0x9c..-0x8d, `matrix` -0xd0..-0x9d, `result` below -0xd0.

Ours: `edge_b` -0xc, `extents` -0x18, `world_diagonal` -0x24, `edge_c` -0x30, `volume_index` -0x34, the 6 at -0x38,
scenario -0x3c, `center` -0x48..-0x3d, `points[4]` -0x78..-0x49, `volume` -0x7c, `ray` -0x88..-0x7d, spill -0x8c.

The -0x01..-0x8c region is exactly 140 bytes in both builds and holds exactly the same objects; `ray`, the
`hs_debug_data` spill and `matrix` already sit at January's offsets. Only the ORDER inside that region differs.

Two further facts recovered this wave:

1. **January's user-local slot order is exactly the reverse of the current source's declaration order**
   (`collision, frame, color, vector, center, points, world_diagonal, extents` + inner `edge_b, edge_c`
   → sides, local_extent, world_extent, points, center, ray, color, matrix, result). That independently corroborates
   the reconstructed declaration list and HCEX's local set.
2. **Our build is strictly monotone in emitted-reference density** (edge_b 15/12 > extents 14/12 > world_diagonal
   13/12 > edge_c 12/12 > spills > center 7/12 > points 27/48); January's order is NOT monotone under that metric —
   `sides` 27/24 = 1.125 sits *above* `local_extent` 14/12 = 1.167, and `points` 0.5625 sits above three 0.75-1.0
   spills. Because both builds emit the same 342 instructions, the emitted-reference census is *identical*, so the
   ranking input lives in pre-optimisation IL and is invisible in the stream. This is the recorded frame-allocation
   fixed point, now bounded precisely: one 0.042-density inversion plus the points-vs-spill group.

### The `sides[2]` structure is January-proven but does not close the row

January's byte evidence for HCEX's `real_vector3d sides[2]` is independent and strong: one contiguous 24-byte
zero-init (`xor eax,eax` plus five register stores at -0x14..-0x04 and one immediate `mov dword [ebp-0x18],0`, the
A21 first-element form), indexed `fstp [ebp+edx-0x18]` / `[ebp+edx-0xc]` under one shared scale, and
`lea &sides[0]` / `lea &sides[1]` twelve bytes apart. Our `edge_b` / `edge_c` pair emits two zero registers into two
non-adjacent 12-byte slots. w1's `t1` already measured that substitution: it reproduces January's zero-init pattern
and cuts slot-ref mismatches from 75 to 67, but the differing-instruction count stays at 134 because the ranking still
puts `local_extent` above `sides`. Re-running it this wave would repeat a measured shape, so no shape was spent.

### The four non-slot instructions

`points[0].x/y/z` in the `side` arm (January `fld world_extent.n; fadd matrix.position.n`, ours reversed) and
`points[2].x` (January `fld sides[1].i; fadd points[1].x`, ours reversed). January's own rows are internally
inconsistent with a pure leaf-age rule — `points[1].x` keeps source order while `points[1].y/z` reverse — so this is
the x87 scheduler rather than a source operand order, and R3 forbids a commutative swap justified only by load order.
w1's t6/t7 showed the only spelling that moves them is the `_point_from_line3d` COMDAT, which is OUT by lane rule
(laws_w3 C2 / R14).

Every other structural hypothesis is refuted by the target bytes themselves: the switch bodies (default block first,
oriented at 0x83, axis-aligned at 0xdb), the two separate `volume->extents` copies, the `set_real_point3d` /
`set_real_vector3d` argument routing in the axis-aligned case, the named `world_extent = local_extent` copy
interleaved into the `fstp` sequence, the assert at line 0x213, and the whole `points[1..3]` chain are already
instruction-identical to January.

**Reopen criterion (sharpened):** a donor or decode that exposes VC7's frame-ranking input beyond emitted reference
density — concretely, anything that raises `sides` above `local_extent` (a 27/24 vs 14/12 inversion) and `points[4]`
above the four 4-byte spills. If this park is ever re-baselined at zero credit, the better-evidenced spelling remains
w1's `scratch/workers/hs_runtime/t1.c` shape (HCEX local names plus `real_vector3d sides[2] = { 0 }`), re-applied on
top of the current `HS_EVALUATE_INEQUALITY` file.

## `_hs_evaluate_begin_random` — SKIPPED-EXHAUSTED (528 B)

191/191 instructions, 187 equal. The only hunk is four preheader instructions: January
`movsx eax,ax / movsx ecx,cx / mov [ebp+0x10],eax / mov [ebp+8],ecx` versus ours
`movsx edx,ax / movsx eax,cx / mov [ebp+0x10],edx / mov [ebp+8],eax`. Both builds sign-extend the same two values
(AX = the `seed_random_range` result, CX = the already-hoisted `mov cx,word [edi]` load of `*argument_count`) and both
spill them into the SAME dead parameter homes `[ebp+0x10]` (`initialize`) and `[ebp+8]` (`function_index`). January
extends both in place; ours rotates through EDX/EAX. Four ModRM bytes; no structural, frame, width or relocation
difference.

A24 is already satisfied (both builds reuse the parameter homes, so its lever is spent); A18/A34 confirm both operands
are `short` (`movsx` plus the 16-bit `test cx,cx` loop guard); A12 does not apply because the hoist itself is
identical. No shape was spent: w1's two are inert and no law justifies grinding a register tie (brief section 6).

**Reopen criterion:** authoritative January local records, or a same-compiler donor, for the loop-invariant
sign-extension register choice.

## `_hs_inspect_enum` — SKIPPED-EXHAUSTED (144 B)

`relocdiff`: size 144/144, relocations 11/11, **normalized sha equal**, objdiff 100.0. The single differing row is
`@0013`: January `__hs_type_scenery_default+12912`, ours `_hs_enum_table-256`. Source line 1442 is
`enum_definition = &hs_enum_table[type-_hs_type_enum_game_difficulty];` (`hs.h:242`), so VC7 folds
`_hs_type_enum_game_difficulty` (16) times the 16-byte stride into the addend and names `_hs_enum_table` with -256;
csplit resolves that same image address to the preceding image symbol `_hs_type_scenery_default`. This is the A45
negative-index alias class, and the park evidence already proves the destinations equal. No credible C spelling can
make VC7 name a different object's symbol, so no shape was spent.

**Reopen criterion:** a comparator/csplit change that resolves relocation identity by image address (the A45
`tools/parked_functions.py` admission path). Zero strict credit either way.

## Orchestrator observations (no packet recommended)

- **C4013 owners (zero bytes, measured inert).** `hs_node_gc` (defined `source/hs/hs.c:4772`) has no prototype in any
  header; `object_list_gc` (defined `source/hs/object_lists.c:187`) likewise. The genuine owners would be
  `source/hs/hs.h` and `source/hs/object_lists.h`. Probe `d1.c` shows the declarations change **no** gate row in this
  unit, and the only C4013 call sites are inside the already-exact `_hs_runtime_update`. `object_lists.h` is included
  by PROTECTED `source/units/units.c` (plus ai_script, hs, damage, unit_scripting_commands), so that packet carries
  declaration-count blast risk for zero bytes. **Not proposed**; recorded only so a later wave does not re-measure it.
- **No header or config edit is requested by this wave.** No symbols.json rename is available: all five residuals
  carry authentic January names (no `_code_` labels in this object), and `_hs_inspect_enum`'s relocation alias is a
  tooling limit, not a naming gap.

## Files

- Notes: `scratch/workers/w3c_hs_runtime.md`
- Scratch: `scratch/workers/w3c_hs_runtime/` (`baseline_gate.txt`, `final_gate.txt`, `d1.c`, `r1.c`, `u1.c`, `u2.c`
  with their `.gate.txt` / `.obj`, `tv_aln.txt`, `rd_aln.txt`, `mkvar.py`)
- w1 scratch reused as evidence (unchanged): `scratch/workers/hs_runtime/` (`t1.c`, `tv1.txt`, `v1..v9`)
