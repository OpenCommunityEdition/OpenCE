# `path.obj` opus5 250K house-clean lane, wave w3c (2026-09-15)

Worker `w3c:path`. Unit `source/ai/path.c`, target `build/split/source/ai/path.obj`
(January 2002 Xbox debug, VC7 13.00.9254, XDK 3911). Worker notes `scratch/workers/w3c_path.md`;
candidates, labs and tooling in `scratch/workers/w3c_path/`.

## 1. Baseline and disposition

`gate.py source/ai/path --all --forbid-emitted-symbol _point_from_line3d` at HEAD and again at the end:

```
== exact 28  residual 4  unwritten 0  (of 32 listed)
```

| Function | Target | Park base / real file | Tag |
| --- | --- | --- | --- |
| `_closest_point_to_attractor` | 208 / 2 / `5d7c8671785a0916` | 208 / 2 / `d96eb8493bcb04e1` | sha |
| `_path_state_begin` | 592 / 20 / `039ed79debe1e371` | 608 / 20 / `ad863f4591f7a569` | size, sha |
| `_path_state_estimated_distance` | 496 / 13 / `24719f50ab8c668f` | 528 / 13 / `e286347823095bc8` | size, sha |
| `_path_state_traverse` | 2096 / 65 / `3e33ddd8b81a7b51` | 2096 / 65 / `f52af9dafed6defa` | sha |

All four non-exact rows are in `config/parked.json`, so every experiment in this wave was scratch-only.
**`git diff --stat -- source/ai/path.c` is empty; nothing landed and no row became strict EXACT, so
there is no park-reopen proposal.** Final checks: emitted-symbol guard passed, `scratch/parkcheck.py`
reports parks 4 drift 0, `tools/fake_match_scan.py source/ai/path.c` 0 leads, owner census unchanged
(the real file was never edited).

Ledgers read: `path_obj_opus5_150k_w1_20260914.md`, `path_obj_opus5_100k_waveD_20260914.md`,
`path_obj_opus5_house_clean_20260913.md`, `path_obj_attractor_pair_natural_reconciliation_20260906.md`,
`path_obj_3d_pair_reconciliation_20260906.md`, `path_obj_jonas_*`, plus `scratch/workers/tiny_stubs.md`.
Laws applied as detectors: WORKER_BRIEF s9, `scratch/w2/laws_w2.md` A1-A22 / R1-R12 / D,
`scratch/w3/laws_w3.md` A23-A45 / R13-R19 / C / D.

## 2. `_path_state_estimated_distance` — size gap CLOSED in scratch (496/13), allocation tie left

`scratch/workers/w3c_path/e4.c` reproduces **every January instruction offset** (191 vs 191
instructions, 496 padded, 13 relocations). Four provable source defects were fixed:

1. **Argument roles (laws_w2 A15).** `vector_from_points3d(a,b) = b - a`, and January emits
   `@0x59 fld [ecx]  (end_point->x); @0x5d fsub [esi+0xc]  (node->entry_point.x)`, so the call is
   `distance3d(&node->entry_point, end_point)`; the real file had the arguments the other way round.
   Same for the attractor distance: `@0xa6 fld [ebp-0x14]; @0xa9 fsub [ebx]` gives
   `distance3d(&state->input.attractor_point, &closest_point)`.
2. **MIN argument must be a named local.** January compares with a NON-popping
   `@0xd5 fcom [esi+0x1c]` and has exactly one copy of the attractor distance. `MIN`/`PIN` duplicate
   their arguments into the `?:` arms (WORKER_BRIEF s9 inline-frequency clause), so
   `MIN(distance3d(...), node->closest_approach_to_attractor)` emitted the whole inline helper twice
   (~30 extra instructions). Binding `real attractor_distance = distance3d(...)` first removes the
   second copy — this alone is the entire 32-byte size gap.
3. **Loop spelling.** `@0x13d cmp bx,si` (si already holds the loop's exit `-1`) plus
   `@0x142 fld [ebp-4]; fcomp 0.8f; test ah,5; jp` (laws_w3 A34: `test ah,5; jp` is `!(x < k)`) is a
   rotated `while (child_node_index != NONE && path_distance < 0.8f)`, not the real file's
   `if (child != NONE) do { if (path_distance >= 0.8f) break; ... } while (child != NONE);`.
4. **`result = TRUE;` position.** `@0x106 mov al,1; @0x108 mov [ebp+0x17],al` executes BEFORE
   `@0x10b je <epilogue>`, so the assignment precedes the `if (estimated_direction_reference)` block
   (laws_w3 A29(d)); the real file had it after.

Declaration order inside the direction block then fixed the frame: declaring
`short current_node_index = node_index;` FIRST (e4) moves `node_index` to `[ebp-4]` and `distance` to
`[ebp-8]`, exactly January's cells, and re-aligns every offset (e1 192 insns -> e4 191 insns).

Residual after e4 — one coupled allocation web:

* a pure three-cycle register rotation (January `state`=EDI, `distance_reference`/`node`=ESI,
  `node_index`=EBX; ours EBX / EDI / ESI);
* one instruction: January homes `result` in the dead top byte of the `distance_reference` parameter
  home, `mov [ebp+0x17],al` … `mov al,[ebp+0x17]` (laws_w3 A24), because its `path_distance` reuses the
  freed `node_index` cell `[ebp-4]`; ours puts `path_distance` in the whole parameter-home dword
  `[ebp+0x14]`, leaving no byte, so it rematerialises `mov al,1`.

Shapes: e1 (the four fixes) 496/13; e3 (`path_distance` declared first) 496/13, worse layout;
e4 (`current_node_index` first) 496/13, offsets aligned; e5/e6 (`direction_point` moved) byte-identical
to e4. Declaration order inside that block is exhausted.

## 3. `_path_state_begin` — one six-instruction x87 block, and the law that explains it

`scratch/workers/tiny_stubs/path_b14.c` (w1's spliced waveD-v11/v14 body) was re-gated: 592/20,
186 vs 186 instructions, and the **only** difference is the square-sum rank inside the inlined
`distance3d`: January ranks i, j, k (source order), we rank k, i, j.

### NEW measured law — the `fsqrt` consumer flips the square-sum rank

For the `real_math.h` three-term square sum, VC7 ranks the terms **i, j, k (source order) when nothing
applies `fsqrt` to the sum**, and **k, i, j when an `fsqrt` consumes it**.

* January corpus (`scratch/workers/w3c_path/chainscan.py` over the split tree): 11 IJK sites. The five
  produced by a genuine helper are `_actor_move_to_point`, `_action_flee_at_flee_position`,
  `_ai_communication_actor_talk_weight`, `ai_debug::_code_00037dd0` and
  `_action_vehicle_setup_impromptu` — every one is a bare `distance_squared3d(...)` inside a relational
  test with no square root, and our own build reproduces IJK in all of them. The remaining six are
  hand-written `dx/dy/dz` accumulations (`game_engine_ctf` x2, `s3tc` x3) or a reversed load order
  (`_biped_accelerate`).
* Lab census (`scratch/workers/w3c_path/lab{1,2,3,4,5}.c` + `lab.py`, compiled with path.c's own
  cflags), 22 spellings: every no-sqrt form is IJK; every form whose value reaches `square_root` in the
  same expression is KIJ; the `magnitude3d`/`magnitude_squared3d` COMDATs are KJI.
* `real distance_squared = distance_squared3d(a,b); d = square_root(distance_squared);` reproduces
  January's `0x87-0xab` byte-for-byte in the lab (lab_v21/w1/x1/x2).

In path.c that split does NOT survive: b1 (split inside the `destination_valid` arm) and b2 (the
intermediate declared in the enclosing block) both compile to bytes **identical to b14**
(sha `0bed38c83a82d776`) — VC7 folds `square_root(distance_squared3d(...))` back into the `distance3d`
tree there. The lab ladder x1 -> x2 -> x3 (outer guard, block-pointer local + assert, eight-double
`error` call) keeps the split effect, so none of those is the folding trigger.

Hand-writing the `dx/dy/dz` accumulation is the only other IJK source form and is refused under
laws_w3 R15 / house rule 13 (the sun_glow precedent). Row classified as an x87 term-rank tie with a
measured law but no admissible in-file spelling.

## 4. `_path_state_traverse` — one new source fact, three decoded leads

`MAX(a,b)` is `((a)>(b)?(a):(b))`, and January's entry is
`fld __real@3e4ccccd (0.2f); fcomp [edi]; test ah,0x41; jne` — the macro's first argument is the one
loaded and returned by the fall-through arm, so the source is
`MAX(0.2f, state->input.pathfinding_radius)`. Shape `t1.c` makes only that change: the entry block
becomes byte-identical and the hunk count drops 132 -> 131 (still 2096/65, `[sha]`). This is an
argument-ROLE recovery from a function-like macro expansion (laws_w2 A15 class), not an R3 commutative
operand swap.

Decoded but not yet reconstructed (leads for a dedicated wave, start from `t1.c`):

* three byte locals — `[ebp-1] = 1` (`result`) and `[ebp-3] = 0` (`reported_cost_overflow`) at entry,
  plus a third `mov byte [ebp-2],0` at `@0x12c` inside the edge loop behind a `jne`. Ours emits one
  entry byte store and uses two DWORD cells (`mov [ebp-0x10],0`, `mov [ebp-0x18],-1`) where January
  uses a byte — a `boolean`-vs-`long` mismatch in the loop state.
* the edge cursor is biased differently: January `lea edx,[ebp-0x844]` then `[eax-0x18]`, `[eax-0x14]`,
  `[ecx-0x10]`, `[ecx-4]`, `[ecx]`; ours biases 12 bytes lower and reads `[eax-0x14]`, `[eax-0x10]`,
  `[ecx]`, `[ecx+8]` (laws_w3 A33 strength-reduction bias).
* frame `sub esp,0x85c` vs ours `0x864` — 8 bytes = two extra dword cells, consistent with the two
  dword flags above (FRAME-SIZE GAP law).

## 5. `_closest_point_to_attractor`

87 instructions, two rank hunks only. January's numerator `dot_product3d(&offset, &segment)` ranks
j, k, i (ours k, j, i); its denominator `magnitude_squared3d(&segment)` — whose three leaves are all
memory, `[ebp-0xc/-8/-4]` — ranks i, j, k, i.e. source order (ours k, j, i). The same
"January takes source order" signature as `_path_state_begin`, but with no `fsqrt`, so the w3c law does
not explain it. Five lab spellings (`lab6.c`: current, named `segment_length_squared`, `/=` split,
`scale_vector3d`+`add_vectors3d` tail, swapped dot arguments) gave no new order; no in-file shape was
spent. The park's own criterion ("authentic source/compiler-context evidence, not equivalent-spelling
searches") is not met.

## 6. Rejected / do-not-repeat additions

| Function | Shape | Result |
| --- | --- | --- |
| estimated_distance | e3 `path_distance` declared first in the direction block | 496/13, worse layout |
| estimated_distance | e5/e6 `direction_point` moved before/after the indices | byte-identical to e4 |
| begin | b1 `distance_squared` split inside the `destination_valid` arm | bytes identical to b14 (folded) |
| begin | b2 same split with the intermediate in the enclosing block | bytes identical to b14 (folded) |
| closest_point_to_attractor | m2 named `segment_length_squared`, m3 `t /= mag`, m5 swapped dot args | byte-identical to the park base |
| closest_point_to_attractor | m4 `scale_vector3d` + `add_vectors3d` tail | different, worse shape |

## 7. Reopen criteria

* `_path_state_estimated_distance` — a lever that puts `path_distance` in the freed `node_index` cell
  `[ebp-4]` (leaving `[ebp+0x17]` for `result`), or a lever for the EDI/ESI/EBX rotation. Start from
  `scratch/workers/w3c_path/e4.c` (496/13, all offsets aligned). Declaration order inside the direction
  block is exhausted.
* `_path_state_begin` — a spelling that keeps `square_root` out of the `distance_squared3d` expression
  tree *inside path.c*, or January-side evidence that the distance is not a `distance3d`-class helper.
  Start from `scratch/workers/tiny_stubs/path_b14.c`.
* `_path_state_traverse` — rebuild the edge loop from the `[ebp-1]`/`[ebp-2]`/`[ebp-3]` boolean set and
  the cursor bias, starting from `scratch/workers/w3c_path/t1.c`.
* `_closest_point_to_attractor` — a mechanism for the memory-leaf square-sum rank, or January-side
  source evidence (the park forbids spelling searches).

## 8. Orchestrator proposals

Optional, zero strict credit, laws_w3 R11-caveated: re-baseline the `_path_state_estimated_distance`
park from 528/13/`e286347823095bc8` to `scratch/workers/w3c_path/e4.c` (496/13 — size and relocation
identity now equal to January, all 191 instruction offsets aligned, four provable source facts fixed).
It is strictly closer, but R11 warns that a remeasure not tied to a reopen may be rejected, so this is
the orchestrator's call. No header, config or symbols change is proposed by this wave.
