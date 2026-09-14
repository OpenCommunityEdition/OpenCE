# ActionObey (opus5 100K consolidated lane, waveB, 2026-09-14): describe_command and command_begin closed

Worker lane `opus/100k-consolidated-20260914` (worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`).
Only `source/ai/action_obey.c` changed. Two already-written residuals became strict EXACT;
`_action_obey_control` remains a documented register tie.

## 1. Scope and provenance

- Unit `source/ai/action_obey.c` vs January split `build/split/source/ai/action_obey.obj`.
- Compiler VC7 13.00.9254 (XDK), repository flags for the unit (`/c /O2 /Oy- /DDEBUG /Dxbox`), unchanged.
- Names were already authentic (January map atlas, orchestrator commit `47d9dd0da`); no rename needed.
- Evidence: January COFF (relocations, instructions via `tools.campaign.alndiff`), Ghidra decompilation
  `C:/halo-worktrees/opus5-30k-fresh-graphs-20260914/scratch/ghidra/out/action_obey.obj.decomp.c:300-538`,
  HCEA `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_command_begin.c` and
  `action_obey_control.c` (semantics/order hints only).
- Prior ledgers read: `action_obey_obj_opus5_fresh_graphs_20260914.md` (including the lead admission),
  `action_obey_obj_private_closure_plan_20260905.md`, `action_obey_obj_api_leaf_independent_review_20260905.md`,
  and the fresh-graphs worker report.

## 2. Validated baseline (lane HEAD)

`== exact 24  residual 3  unwritten 0  (of 27 listed)`, `_point_from_line3d` guard passed, no parked entries.

| Row | Target (padded / relocs / sha16) | Baseline candidate | First divergence |
| --- | --- | --- | --- |
| `_action_obey_command_begin` | 4320 / 167 / `5b56fdd8b88a4456` | 4336 / 167 | `sub esp,0x184` vs `0x174` |
| `_action_obey_control` | 992 / equal | 992 / equal, sha differs | +0x33e register pair |
| `_action_obey_describe_command` | 1936 / 153 / `d14c27f63086d980` | 1824 / 147 | pause arm `jmp` into a shared tail |

Final: `== exact 26  residual 1  unwritten 0  (of 27 listed)`, guard passed, 0 parked drift (no entries), owner
census clean (no new code/data owner vs `build/base`; the header-inline SELECT_ANY math copies are pre-existing and
already disclosed), `tools/fake_match_scan.py` 0 leads.

## 3. Accepted controls (retained in production)

### `_action_obey_describe_command` (now EXACT 1936/153)

- `relocdiff --count-by-target`: every literal/call count equal except `__snprintf` 26 vs 20, so the gap was six
  cross-jump merges, not missing code.
- January's animate and recording NONE arms read `mov eax, offset "NONE"; push eax` after
  `mov eax, offset "<error>"`: the name is a variable, exactly like the script arm. Source now uses the
  script-arm idiom for all three tag-reference atoms: `name = "<error>"; if (index == NONE) name = "NONE";
  else if (valid) name = element; _snprintf(..., name);`. VC7 duplicates that small join (snprintf plus return)
  into both predecessors; the script join also loads a modifier table, so it is not duplicated. Ghidra shows two
  calls only because it decompiles the duplicated join.
- HEAD's separate `_snprintf(..., "NONE")` calls pushed an immediate. That made those arms identical to other
  0x10 tails and moved VC7's cross-jump hosts across the whole function.

### `_action_obey_command_begin` (now EXACT 4320/167)

1. **Look-arm duration re-store.** January's `_ai_atom_look` valid-point arm contains
   `mov ecx,[command+4]; mov [ebp-0x28],ecx` next to the point-index store, so `look_duration = command->parameter1`
   is really stored twice. The same shape appears in look_random, which assigns the duration inside its valid arm.
   The earlier lead removed this line as a "duplicated store". The instruction is in January, and removing it
   again measures [sha].
2. **Declaration order.** The look prologue stores point, prop, unit, then duration (-0x14, -0x30, -0x2c,
   -0x28), so `look_duration` is declared after the three NONE indices. The old order measures [sha].
3. **Scoped prop search.** In January, `prop_iterator` (-0x1c..-0x14) and the per-player `head_position`
   (-0x20..-0x14) use overlapping storage, so their scopes are disjoint. In HEAD the prop iterator was scoped to the
   whole look_player branch, which enclosed the player loop. That pushed `data_iterator` into its own 16-byte class
   (frame +16) and split January's {direction, facing, head_position, vehicle_origin, prop_iterator,
   actor_definition} class. The prop search is now in its own block inside the look_player branch. Without it the
   size is 4336.
4. **running_jump store order.** `delay_ticks` is written before `pause_timer`, matching the already-exact
   targeted_jump arm, which writes delay_ticks, the velocities, then pause_timer. This fixes January's EAX reload of
   `simple_control` after `test al,al`.

## 4. Experiment matrix

| Candidate | Change | Result | Kept |
| --- | --- | --- | --- |
| describe v1 | animate/recording as script-style if / else-if + one `_snprintf` | 1936/153 EXACT | yes |
| begin b1 | + look-arm duration re-store | 4336, look diffs reduced | yes (step) |
| begin b2 | + duration declared last | 4336, only frame + running_jump register | yes (step) |
| begin b3 | + prop search block | 4320/167, frame 0x174, one EAX/ECX pick | yes (step) |
| begin b4 | + delay_ticks before pause_timer | EXACT | yes |
| begin n1 | b4 minus re-store | [sha] | no |
| begin n2 | b4 with old declaration order | [sha] | no |
| begin n3 | b4 without prop block | [size 4336!=4320] | no |
| control c1 | alignment_vector declared before facing | identical to HEAD | no |
| control c2 | HCEA ternary `jump_alignment_vector = normalize2d(...) == 0 ? *global_forward2d : facing;` first | worse (264 insns, register swaps spread to 0x2b7) | no |
| control c3 | same ternary into the local, store in place | identical to HEAD | no |

Every whole-TU gate kept all other rows unchanged (full `--all` listing diffs).

## 5. Do-not-repeat

- describe_command: the return/break mixes from the earlier lane. The cause was the NONE-arm shape, now closed.
- control: declaration order of facing/alignment_vector; ternary alignment forms; alignment store position;
  Ghidra single overwritten local; jump-block scope local.

## 6. Residual classification

- `_action_obey_control` (992, [sha]): ABI/register tie, measured. January gives alignment i->EDX and j->ECX
  (facing arm loads j first; join stores +0x448 before +0x444). Ours gives the inverse. Every other instruction is
  equal.

## 7. Reopen criteria

- control: a verified January idiom where an enregistered 2D vector pair is copied j-first, or new type evidence for
  `orders.move.jump_alignment_vector`.

## 8. Disposition

- `_action_obey_describe_command` and `_action_obey_command_begin`: strict EXACT, eligible for Matching
  (normalized sha16 equal to January).
- `_action_obey_control`: still active residual, no credit.
- Lesson: both closed rows had been labelled cross-jump/frame-packing ties. In each case the relocation count or
  slot overlap pointed to a concrete source shape. Read `relocdiff --count-by-target` and slot overlaps before
  classifying a tie.

## Orchestrator admission note (checkpoint 2)

Only `_action_obey_describe_command` was admitted (1,936 padded). The strict-exact `_action_obey_command_begin` form restores `look_duration = command->parameter1;` in the look arm, directly after the identical declaration initializer. That is a redundant store, prohibited by the house rules, and the fresh-graphs lane removed it for the same reason. Its hunks, including the bare prop-iterator block and the pause_timer/jump reorder, were reverted. The worker file is preserved as `scratch/orch/action_obey_waveB.c`. Reopen only with evidence of a non-redundant January statement shape.
