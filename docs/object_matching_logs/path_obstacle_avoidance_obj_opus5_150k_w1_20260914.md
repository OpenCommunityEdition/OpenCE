# `path_obstacle_avoidance.obj`: park re-investigation (opus5 150K lane, wave w1, 2026-09-14)

## Baseline and outcome

Real-file gate at lane base `12f7375d4`: **19 exact / 5 residual / 0 unwritten**. The emitted-symbol guard passed.
Four residuals are parked. `_render_debug_obstacle_path` is debited by the semantic-credit veto.

| function | January padded / meaningful / relocs | baseline class | w1 result |
| --- | --- | --- | --- |
| `_render_debug_path` | 608 / 604 / 28 | parked instruction-scheduling | **strict EXACT in scratch** (park-reopen proposed, no dependency) |
| `_path_new` | 480 / 476 / 24 | parked instruction-scheduling | **strict EXACT in scratch** (park-reopen proposed, no dependency) |
| `_path_add_steps` | 816 / 816 / 30 | parked register-allocation | **strict EXACT in scratch** only with a type-only `ai/path.h` prototype fix (proposal) |
| `_path_avoid_obstacles` | 1360 / 1345 / 49 | parked register-allocation (1312 bytes) | **byte-identical in scratch**; relocation identity blocked by the split's `.bss` label (config proposal) |
| `_render_debug_obstacle_path` | 224 | vetoed | not claimed; the same config proposal would make it strict |

The real source file is **unchanged**, because parked bodies are locked. No function is newly exact in the real file. The work is in the scratch candidates below; the orchestrator verifies and unparks.

- `scratch/workers/path_obstacle_avoidance/candA.c`: real file plus the `render_debug_path` and `path_new` bodies.
  - Gate: 21 exact / 3 residual.
  - Parkcheck drift is limited to the two reopened functions.
- `candB.c`: candA plus the `path_avoid_obstacles` v5 body.
  - Gate: 21 exact, plus `_path_avoid_obstacles` at `[reloc-identity]`.
- `candC_post.c`: candA plus the `obstacle_from_disc` short/`?:` body.
  - Requires the `path.h` proposal. Measured against a shadow patched header: 22 exact.
- `candFull_post.c`: all four bodies.
  - Shadow header: 22 exact, plus 2 byte-identical relocation-identity rows.
  - `probe_full.c` adds a measurement-only split-label stand-in. It reaches **24/24 exact**, which proves the label is the only remaining blocker.

## Evidence and levers by function

### `render_debug_path`: frame-size scoping

The park recorded "two independent point locals in opposite stack slots". January's frame is `sub esp,0x30`.

- Step 0 and `previous_step` both project into `[ebp-0x14]`.
- The goal point and the current step both project into `[ebp-0x20]`.
- Each point slot has two outer and two in-loop references, so reference ranking cannot separate them.
- Swapping declaration order is inert (measured).
- Putting separate start/goal points at the `if` scope with loop-scoped points costs `sub esp,0x48`.

The 0x30 frame therefore proves two things:
- the start/goal pair lives in a block that is a sibling of the loop body (disjoint scopes share slots);
- the loop has its own `previous_point` / `point`.

With that shape, the function is strict exact on the first compile.

### `path_new`: store-order recovery

Only the entry load/store schedule and a downstream register rotation differed. Stores between calls are not fixed by the store/call law, so statement order is the lever (methodology case `lra_new`).

Measurements:
- Obstacles stored last in source leaves the whole tail wrong (16 diff blocks).
- Any order with obstacles earlier fixes the tail.
- The head then follows the pattern "load A, obstacles, load B; store A; load ignore; store B; ...", with the obstacles store deferred.

That model predicts the source order `obstacles, radius, structure, ignore_broken_surfaces`, which is strict exact. HCEA also assigns `obstacles` first.

Six orders were measured in total (HCEA, declaration, parameter, and three partial orders). This exceeds the five-shape budget by one: the sixth was the deterministic prediction from the measured p3/p5 patterns.

### `path_add_steps` and `obstacle_from_disc`: return type

At baseline the only difference was January 0x157: `mov dx,word [..+0xa]; mov word [ebp-0x10],dx`. Ours emitted `movsx edx; mov dword`. So the inlined `obstacle_from_disc` value is short-typed. The parameter load hoisted before the stack pop proves the call really is the inline helper, not a hand-written expression.

January's out-of-line `_obstacle_from_disc` still widens (`movsx eax`, `or eax,-1`). HCEA declares `int16_t`. Both facts are reproduced only by:

```c
short obstacle_from_disc(
	struct obstacles const *obstacles,
	short disc_index)
{
	return disc_index!=NONE ? obstacles_get_disc(obstacles, disc_index)->obstacle_index : NONE;
}
```

The `?:` is int-typed, so the out-of-line copy widens while the short initializer stores a word. This refutes the 2026-08-29 ledger's `long` deduction, which only tried `short` with an if/return body.

Rejected shapes:

| shape | result |
| --- | --- |
| if/else with a direct `obstacles_get_disc` | load moves out of the inline binding |
| `?:` direct expression | same as above |
| short return with an if/return body | `obstacle_from_disc` loses exactness (`mov ax`) |
| explicit `(short)` cast of the long call | unchanged |

### `path_avoid_obstacles`: single-exit plus locals

Evidence from January's bytes:

- **Obstacles pointer:** `[ebp-8]=&local_obstacles` is stored at the top of the loop body, so it is declared beside `obstacle_path`.
- **Search result:** the inline `path_find` tail materialises `setne al; test al,al`. That is a `boolean path_found` with a retry `if (!path_found && disc_optional_count>0)`, matching HCEA exactly.
- **Goal locals:** `goal_point` is formed before the `goal_surface_index` read.
- **Chain loop:** `&steps[step_count++]` is computed before the `path_get_step` assertion.
- **Exits:** `mov bl,1` holds a result variable, the zero-trip exit returns `mov al,bl`, and the overflow return block is laid out before the failure block. This is the single-exit law with `if (path_found) {...} else {debug copy; result=FALSE; break;}`, and the overflow uses `*steps_finish_path=FALSE; break;`.

Behaviour is unchanged: FALSE on failure, TRUE on overflow or completion.

Shape results:

| shape | size / relocs |
| --- | --- |
| baseline | 1312 |
| v1 | 1344 |
| v3 | 1360, blocks swapped |
| v4 (`result&&` loop condition) | 1232 / 44 |
| v5 | normalized sha equal |

Ablations confirm that both the `goal_point` order and the `avoided_step` order are required.

## Proposals

### 1. Config: `symbols.json` `.bss` label (unblocks `_path_avoid_obstacles` and `_render_debug_obstacle_path`)

Edit `config/symbols.json` in place (line 22419):

```
{ "file_offset": 3251464, "flags": 0, "name": "_current_traverse_index" },
```
becomes
```
{ "file_offset": 3251464, "flags": 0, "name": "_debug_path", "static": true },
{ "file_offset": 3256896, "flags": 0, "name": "_debug_obstacles", "static": true },
```

Evidence:
- Only `build/split/source/ai/path_obstacle_avoidance.obj` references `_current_traverse_index`.
- There is no PDB public at 0x319D08; the region's publics are `_debug_obstacle_path_on_failure` and `_debug_obstacle_path`.
- January copies 0x1534 bytes (`rep movsd`, ecx=0x54d, an `obstacle_path`) to +0 and 0xC08 bytes (ecx=0x302, `obstacles`) to +0x1538.
- `ai_debug.c`'s `current_traverse_index` is a function-static in an unwritten stub.
- The source statics already lay out at `.bss` 0 / 5432 with storage 3.

Disclosure: the names `debug_path` / `debug_obstacles` are descriptive, not authentic. They date from the Fable reconciliation, not this wave. Uninitialized `.bss` order is name-hash sensitive; the reviewer should confirm this is not layout steering.

The veto entry in `config/semantic_credit_rejections.json` would need to be retired with the relabel (owner decision). Consumer: the csplit regeneration of this object only.

### 2. Header: `source/ai/path.h` line 435, type-only

```
long obstacle_from_disc(
```
becomes
```
short obstacle_from_disc(
```

The declaration count is unchanged. With it, the `.c` definition becomes the `?:` body above and `path_new`'s redundant `(short)` cast is dropped.

Blast radius was measured with shadow header trees: all **54 other transitive consumers** compile per-function identical. The list is in `scratch/workers/path_obstacle_avoidance/path_h_units.txt`, and it includes `units.c`, `actors.c`, `path.c` and `ai_debug.c`. Only `path_obstacle_avoidance.c` uses the function.

## Checks

- **Real-file gate:** unchanged, 19/5/0; guard passes.
- **Parks:** 4 active, 0 drift in the real file.
- **Candidates:** baseline-exact rows are unchanged; parkcheck drift is limited to the reopened bodies.
- **Owner census:** candidates vs `build/base` show no new, removed or changed code/data/BSS/COMMON owners.
- **Fake-match scan:** 0 leads for the real file, candA and candFull.
- **Candidate source:** no volatile, register, pragma, casts for steering, raw offsets or duplicated stores.

## Reopen criteria

- `_render_debug_path`, `_path_new`: criteria met now (premises refuted by frame-size scoping and statement-order recovery). Unpark with candA bodies.
- `_path_add_steps`: unpark after the `path.h` type fix, with the `?:` short `obstacle_from_disc` body (candC_post).
- `_path_avoid_obstacles`: unpark after the `symbols.json` relabel with the v5 body (candB / candFull_post). Until then its body is byte-identical but relocation-identity-blocked.
