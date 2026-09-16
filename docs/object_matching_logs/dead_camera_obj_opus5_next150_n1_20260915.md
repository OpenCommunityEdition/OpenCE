# `dead_camera.obj` - next-150K house-clean lane, wave n1 (2026-09-15)

## Outcome

No source change landed. `source/camera/dead_camera.c` is byte-for-byte unchanged
(`git diff --stat -- source/camera/dead_camera.c` is empty). The unit stays at
**2 exact / 2 residual / 0 unwritten**, the `_point_from_line3d` guard passes, the unit has no
parks, and `fake_match_scan` reports 0 review leads.

| Function | January padded / relocs | Baseline | Result |
| --- | --- | --- | --- |
| `_player_has_allies` | 112 / 6 | EXACT | EXACT (untouched) |
| `_player_get_next_player_with_a_unit` | 176 / 6 | EXACT | EXACT (untouched) |
| `_dead_camera_new` | 288 / 19 | residual (sha) | NOT-LANDED: emission-order tie; the tail region is now closed in scratch by an R4-safe form, one 5-instruction transposition remains |
| `_dead_camera_update` | 1,248 / 46 | residual (sha) | NOT-LANDED: the uninitialised-local original bug is now MECHANICALLY PROVEN; needs an owner ruling |

The wave's product is two proofs, not bytes: `_dead_camera_update`'s residual is no longer an
inference, and `_dead_camera_new`'s statement-order search space is now closed.

## Prior ledgers and do-not-repeat items

Read before any work: `dead_camera_obj_codex_checkpoint.md`,
`dead_camera_obj_fable_semantic_names_20260831.md`,
`dead_camera_obj_jonas_hcea_constructor_topology_rejection_20260826.md`,
`dead_camera_obj_opus5_150k_w1_20260914.md`,
`dead_camera_render_particles_obj_jonas_closure_freshness_audit_20260830.md`,
`legacy_residual_donor_truth_audit_20260913.md`, plus `scratch/w2/laws_w2.md` and
`scratch/w3/laws_w3.md`.

The frozen HCEA combined topology (direct pitch assignment plus current-player-before-watched-unit,
272 bytes) was NOT repeated. `branch_sweep source/camera/dead_camera` lists 9 unique history blobs;
the integrated source is the best of them (every other blob is 0 exact / 2 unwritten, or
compile-fail), so no historical donor was re-mined.

## `_dead_camera_update` - the original bug is now proven, not inferred

### Census (structural: the two objects are structurally identical)

* **Frame.** Neither side has a prologue `sub esp`. The only `sub esp,0x90` is the `csprintf`
  argument area inside the assert. Zero frame locals; every local is enregistered.
* **`[ebp+0x10]` slot life**, identical on both sides: the `result` parameter is copied to ESI at
  `0x20` and `0x3b` (both dominating paths of the `if (unit)`), and the slot is then reused as a
  scratch spill by the assert's `valid_real` temporaries at `0x1db`, `0x21d`, `0x25f`, `0x29f`,
  `0x2e1`, `0x323`, `0x377`, `0x3b5`, `0x3eb`. This is laws_w2 **A24** parameter-home reuse, and
  our build already has it.
* **Relocations.** 46 target / 46 candidate, identical by target and order; the only address
  deltas are a uniform one-byte shift after `0x176`.
* **Instructions.** 373 target / 374 ours, and the extra one is a trailing padding `nop`.
  Exactly ONE real instruction differs, at `0x176`:

```text
T  176  mov eax, dword ptr [ebp + 0x10]
O  176  mov eax, esi
```

It is the `next_player_index == NONE` fallback value of `next_unit_index`, a phi into EAX
(`jmp 0x179` out of the then-arm, `mov eax,<fallback>` in the else-arm).

### Why this is not a register tie

`result` is enregistered in ESI for every other use on both sides, so January's memory read is not
a read of the `result` variable; it is a read of a DIFFERENT local that VC7 gave the `result`
parameter's home. Two measured candidates settle it:

| Shape | Source form | Result |
| --- | --- | --- |
| `scratch/workers/n1_dead_camera/v1.c` | `long next_unit_index = (long)result;` at the top of the switch block, `if` with no `else` | VC7 assigns the local the parameter home and emits January's fallback instruction **byte-identically** (`mov eax, dword ptr [ebp + 0x10]`). The ONLY remaining difference in the whole 1,248-byte function is one extra initialising store `mov dword ptr [ebp + 0x10], esi` at `0x13f`. 1248 / 46, not exact. |
| `scratch/workers/n1_dead_camera/v2.c` | the same declaration hoisted to function scope | the copy is NOT elided (`mov esi,[ebp+0x10]; mov [ebp+0x10],esi` at `0xe`/`0x12`) and the later spill slot moves to `[ebp+8]`; strictly worse, 4 differing regions. |

v1 proves the storage; v2 proves VC7 never elides the initialising store even when source and
destination are the same slot. The only source form that removes the store is a bare
`long next_unit_index;` with no initialiser and no else arm - January's original bug, an
indeterminate read, which brief section 7 forbids. The 20260913 donor audit rejected the
1,235-byte donor for exactly this reason; this wave supplies the missing proof that **no defined
initialiser can reproduce the byte**.

### Owner packet

Requested ruling under the original-bug policy (`docs/matching_methodology.md`). Exact text, in
`source/camera/dead_camera.c` only - no header, config or consumer change, and no other file in
the tree is affected:

```c
		long next_player_index;
		/* BUG (preserved for exact matching): January never initializes
		 * next_unit_index; when no next player is found it tests whatever
		 * the reused parameter home happens to hold. */
		long next_unit_index;
		boolean match_team;
		...
		if (next_player_index != NONE)
		{
			next_unit_index = player_get(next_player_index)->unit_index;
		}
```

(the current `else { next_unit_index = (long)result; }` is deleted). Value: 1,235 meaningful /
1,248 padded bytes, the largest single row in this object. Note the contrast with the current
source: `(long)result` is a *defined* stand-in that happens to name the same value January's frame
holds, but it costs the one store, so it can never be exact.

## `_dead_camera_new` - statement-order space closed, one transposition left

### Census (structural: identical)

* **Frame.** No `sub esp` on either side. Frame size 0; all locals enregistered
  (EBX = `local_player_index`, ESI = `camera`, EDI = `observer` then `unit_index`,
  EDX = `dead_timer`). There is no frame slot to gain or lose, so the frame-layout lever and
  laws_w2 A6 cannot apply.
* **Relocations.** 19 target / 19 candidate, identical by target and count: 3x
  `_get_global_local_random_seed_address`, 3x `_real_seed_random_range`, 3x
  `_dead_camera_constants` at +0/+4/+8, and one each of `_observer_get_camera`, `_display_assert`,
  `_system_exit`, `_game_engine_running`, `_local_player_get_player_index`, `_player_data`,
  `_datum_get`, `__real@7f7fffff` and the two string literals. No missing or extra call, global or
  literal.
* **Instructions at baseline.** 105 target / 106 ours, in two regions.

### R2 (tail) - closed in scratch

January stores `camera->unit_index` from a single phi register in both tail-duplicated arms
(`mov eax,[eax+0x38]` / `mov eax,edi`, then `mov [esi+0x28],eax` and `mov [esi+0x24],ecx` in each
arm); the baseline source stored the per-arm value directly. `nA.c` writes the tail as a local phi
parallel to the block that already exists for `switch_timer`:

```c
	long watched_unit_index;
	...
	if (unit_index == NONE)
	{
		watched_unit_index = player_get(player_index)->dead_unit_index;
	}
	else
	{
		watched_unit_index = unit_index;
	}
	camera->unit_index = watched_unit_index;
```

This reaches w1 shape B's fixed point (105 instructions vs 105, R2 gone) in an **R4-safe** form:
the sibling `switch_timer` block also uses if/else into a local plus one store, so this is not an
"if/else -> ternary while a parallel block keeps if/else" permutation.

### R1 - the residual

```text
T   99  mov edi, dword ptr [ebp + 0x10]      O   99  fchs
T   9c  mov edx, [_dead_camera_constants]    O   9b  mov edx, [_dead_camera_constants]
T   a2  fchs                                 O   a1  fstp dword ptr [esi + 0x10]
T   a4  add esp, 0xc                         O   a4  mov edi, dword ptr [ebp + 0x10]
T   a7  fstp dword ptr [esi + 0x10]          O   a7  add esp, 0xc
```

Identical instruction multiset, identical registers, one basic block (it ends at the `je` after
`cmp edi,-1` / `mov [esi+0x1c],edx`, which are identical on both sides). Exactly one instruction
precedes the constants load on each side: the `fchs` in ours, the `unit_index` parameter load in
January. January's two earlier random calls both emit `add esp,0xc` before the member store and its
third call keeps that pattern, so the divergence is the flush timing of the x87 result.

### Shapes measured this wave (2, on top of w1's 7)

| Shape | Change | Result |
| --- | --- | --- |
| `nA.c` | local declarations in usage order (observer, distance, yaw, pitch, switch_timer, player_index) + the local-phi tail above | **R2 exact**, 288 / 19, R1 the only remaining region. Declaration order is INERT for R1. |
| `nB.c` | nA + the `pitch` local removed, the negated call left in place after the timer store (the variant w1 shape C never isolated, because C also moved the statement) | strictly worse: the whole timer statement migrates in front of the call (`mov edx,[const]` @0x84, `mov [esi+0x1c],edx` @0x94, then the call), 4 differing regions. |

nB pins January's statement order: the `camera->timer =` statement must follow the pitch random
call, and the pitch store must follow the timer store (w1 shape E2 measured the other order
worse). Every admissible statement order is now measured, and R1 survives all of them, so R1 is an
**emission-order (x87 flush-timing) tie inside one basic block**, not a structural defect.

### Evidence checked and refuted this wave

* **HCEA `HCEX.pdb`, `DIA2Dump -sym dead_camera_new`**: `struct dead_camera *camera`,
  `short local_player_index`, `long unit_index` - all three parameters and types confirm the
  current reconstruction. Every scalar is enregistered, so the PDB lists NO locals: it cannot
  settle the `pitch` / `switch_timer` / `player_index` local question and supplies no new lifetime
  fact for either function.
* **Constant spelling.** `0.47123894f` and `1.0995574f` are 27 and 63 degrees, but neither a
  double fold nor a float-step fold of `DEGREES_TO_RADIANS(27.f)` reproduces January's
  `0x3ef1463b` (both give `0x3ef1463a`); `DEGREES_TO_RADIANS(63.f)` does give `0x3f8cbe4c`. The
  decimal literals in the current source are therefore correct for the minimum, and the macro
  spelling is byte-inert in any case (both forms are compile-time immediates).

## Orchestrator proposal (authenticity, expected byte-inert - do not land alone)

`source/camera/dead_camera.c` defines a TU-local `struct dead_camera_command` (0x68 bytes) and
`source/camera/dead_camera.h` forward-declares it. HCEA `HCEX.pdb`
(`DIA2Dump -sym dead_camera_update`) gives the authentic third parameter as
`struct observer_command *result`, and `source/camera/observer.h` - already included by
`dead_camera.c` - owns exactly that layout: `flags`, the
`focus_position / focus_offset / focus_distance / field_of_view / forward / up` union,
`focus_velocity`, `timer`, `parameter_flags[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS]`, `pad51[3]`,
`parameter_timers[NUMBER_OF_OBSERVER_COMMAND_PARAMETERS]`.

The TU-local struct is therefore a local shadow of a shared owner (brief section 7, PROHIBITED),
and the bodies should read `FLAG(_observer_command_valid_bit)` instead of `FLAG(0)` and
`result->parameter_flags[_observer_command_parameter_focus_position] = 3` instead of
`result->position_flags = 3`. Sibling precedent: `source/camera/flying_camera.c` already uses
`FLAG(_observer_command_valid_bit)` from `observer.h`.

Landing this needs a header edit (the `dead_camera.h` prototype plus dropping
`struct dead_camera_command;`), which a worker may neither write nor measure. Consumers of
`dead_camera.h`: `source/camera/dead_camera.c` and `source/camera/director.c` (director.c casts
`dead_camera_update` to `director_camera_update_proc`, so the parameter type change does not reach
it). Expected byte-inert - offsets and constants are unchanged - so it must ride with a landing,
never as a lone hygiene hunk.

## Checks

* Final real-file gate: `exact 2 residual 2 unwritten 0`, `_point_from_line3d` guard passed.
* `git diff --stat -- source/camera/dead_camera.c`: empty.
* Siblings `_player_has_allies` (112) and `_player_get_next_player_with_a_unit` (176) stayed EXACT
  in all four candidate gates.
* Park drift: `scratch/parkcheck.py source/camera/dead_camera` - "no parks for
  source/camera/dead_camera".
* Owner census vs `build/base/source/camera/dead_camera.obj`: v1, v2, nA and nB each add and drop
  ZERO owners (29 owners on both sides).
* `tools/fake_match_scan.py` on the real file, `v1.c` and `nA.c`: 0 review leads each.
* No header, config, symbols.json, parks or tooling change; no build, test or git command was run.

## Reopen criteria

* `_dead_camera_update`: an owner ruling admitting the uninitialised local under the original-bug
  policy. The source packet is above; nothing else is required, and 1,235 meaningful bytes close
  with it.
* `_dead_camera_new`: start from `scratch/workers/n1_dead_camera/nA.c` (R2 already exact) and apply
  ONE new January-side fact that moves the `unit_index` parameter load in front of the `fchs` -
  a measured VC7 flush/emission law for a cdecl float return whose store is the last statement of a
  basic block, or a cache-beta source record. Do NOT re-spend statement order (all admissible
  orders are now measured), declaration order (measured inert), the HCEA combined topology (frozen
  20260826), or the `DEGREES_TO_RADIANS` constant spelling (refuted above).

Worker notes and every candidate: `scratch/workers/n1_dead_camera.md` and
`scratch/workers/n1_dead_camera/`.
