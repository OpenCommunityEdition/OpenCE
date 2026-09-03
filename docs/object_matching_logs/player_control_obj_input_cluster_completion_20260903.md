# `player_control.obj` input-cluster completion (2026-09-03)

## Verdict

The January local-input cluster is now completely written in production C.
The isolated lane advances `source/game/player_control.obj` from 42 exact,
two residual, and five unwritten functions to 45 exact, four residual, and
zero unwritten functions.  Three functions close strictly for 656 exact padded
bytes.  The two complete semantic callers account for another 5,024 target
bytes and are retained as honest fuzzy parks rather than being forced through
optimizer-only source.

This work was performed on `agent/player-control-unwritten-20260903` from
`4c4ba569d19678992f9fa95180142a1a1d5d7916`.  It was committed only in the
isolated worktree; canonical was not modified and nothing was pushed.

## Authorities and provenance

- January's split `source/game/player_control.obj` is the byte authority.
  XDK 3911 compilation and `tools/campaign/gate.py` establish padded size,
  normalized bytes, and relocation count/identity.
- The typed behavioral donor is HaloCEA commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, especially
  `_research_cache/halocea/src/blam/game/get_local_player_input_blob.c`,
  `handle_one_player_input.c`, and `player_control_update.c`.
- The January-specific control flow and field offsets were checked against
  Pastudan history commit
  `918af885935ec470a31256ecce9a977b12b01f80`, path
  `research-cache/pastudan-halo-full-history-20260828/src/halo/game/player_control.c`.
  Its raw-offset lift was evidence only; no raw-offset or address-derived
  spelling was copied into production source.
- The Stian January lift independently authenticates the otherwise easy-to-
  miss `boolean player_control_update(real)` result and final `return FALSE;`.
  That signature, rather than padding manipulation, closes the update wrapper.
- January assertions and relocations authenticate the look-function and
  acceleration conditions, the external `MOUSE_YAW_SCALE` and
  `MOUSE_PITCH_SCALE` owners, the input calls, and the private helper graph.

## Strict result

Baseline:

```text
== exact 42  residual 2  unwritten 5  (of 49 listed)
```

Final:

```text
== exact 45  residual 4  unwritten 0  (of 49 listed)
```

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_player_action_clear` | 16 | 1 | `6e6690b65743dd4aed7c2480d704558a599ac4455958d5692dc9adff935cf92d` |
| `_player_control_action_test_check_reset_input_blob` | 512 | 13 | `468f32f7420ac3729550516a5d7794b91f2d906dff6809219ae92f876e932cf2` |
| `_player_control_update` | 128 | 12 | `ca839661040e79993c2eefea0ce84594a26e1071e43e6ab956a224e7a6f94eb9` |

The exact padded-byte total increases from 3,264 to 3,920.  No inherited exact
function is lost.

## Complete fuzzy bodies

| Function | Target/base padded bytes | Target/base relocs | Objdiff | Class |
|---|---:|---:|---:|---|
| `_get_local_player_input_blob` | 3,456/3,456 | 130/130 | 98.305176% | unclassified |
| `_handle_one_player_input` | 1,568/1,568 | 95/95 | 99.010185% | unclassified |

`get_local_player_input_blob` clears the typed blob; obtains the player,
control slot, gamepad, and January control constants through subsystem
accessors; applies unit/seat look rates, circular stick normalization,
doubled-spin curves, zoom and body-stun scaling, acceleration, aim assist and
magnetism; implements the inhibited-button release latch and all target-backed
gamepad mappings; retains the Xbox build's dormant keyboard/mouse fallback;
normalizes throttle; and always runs the action-test helper.  The January
`game_input_state` is the 0x1C, twelve-button layout, not HaloCEA's later
thirteen-button/fancy-look layout.  The local input/mouse representations have
compile-time size and offset guards.

`handle_one_player_input` consumes that blob through named fields.  It honors
director inhibition, local debug possession, weapon/grenade readiness and
rotation, zoom, facing, and the target-backed on-foot autolevel state, then
commits and validates the player-control state and queues a fully typed
`player_action`.

Both residuals have exact padded envelopes and relocation counts.  The caller
alignment diff leaves equivalent local/x87 scheduling, one floating comparison
branch form, stack-slot allocation, and compiler-local literal ownership.  The
consumer residual is likewise local/register and independent-instruction
scheduling.  The fail-closed codegen classifier returns `UNKNOWN` for both, so
the manifest uses `unclassified` and grants no exact credit.

## House-rule reconciliation

- Player and control data use `player_get` and `player_control_get`; units and
  bipeds use `unit_get` and `biped_try_and_get`; tag blocks use
  `TAG_BLOCK_GET_ELEMENT`; and all scalar and flag operations use cseries
  types/macros.
- Private functions carry PDB-authenticated names.  The clear helper's
  behavior-backed `player_action_clear` name is retained from the prior
  admitted provenance ruling.  New public declarations live in their owner or
  closest-associated headers.
- The `player_control` layout now has semantic field names with compile-time
  offsets, without changing its size.  The January input and mouse details stay
  TU-local to avoid shared-header definition-position drift.
- The update queue's January no-extra-argument signatures replace the
  placeholder parameters.  `player_control_update` uses its independently
  authenticated boolean return type.
- Every new no-argument function uses `void`, parameters are multiline, and
  all functions have explicit returns.
- No raw offset, address-derived new symbol, representation pun, volatile or
  register forcing, barrier, pragma, gameplay assembly, forced inline, fake
  dependency, dummy reachability, or nonsensical matching logic is retained.
  The final object does not emit `point_from_line3d`.

## Experiment matrix

| Experiment | Caller result | Disposition |
|---|---:|---|
| Declare the private caller but leave it unwritten. | The real call graph cannot be emitted; the two proven helpers remain latent. | Rejected.  No dummy reachability or fake caller. |
| First typed HCEA/January translation. | 2,896 bytes, 128 relocations. | Useful semantic base; incomplete January source shape. |
| Restore the complete January button and keyboard/mouse topology. | 3,424 bytes, 132 relocations. | Retained, then minimized to authenticated owners. |
| Reuse the target-backed curve/facing owners and remove duplicate materializations. | 3,424 bytes, 130 relocations. | Retained. |
| Preserve separate clamped inputs, use boolean-normalized key differences, and restore the final January local schedule. | 3,456 bytes, 130 relocations; 98.305176%. | Accepted fuzzy park. |
| Keep `player_control_update` as `void`. | 112/128 padded bytes, 12 relocations; 97.5%. | Rejected after independent signature evidence. |
| Restore `boolean` and `return FALSE;`. | 128/128, 12/12, strict exact. | Accepted. |

## Verification

- Final isolated gate: 45 exact, four residual, zero unwritten.
- Focused exact checks: all three new strict functions reproduce target padded
  bytes, normalized SHA-256, and relocations.
- Objdiff report generated directly from the scratch candidate records
  98.305176% and 99.010185% for the two parks.  A focused parked-manifest audit
  reports two active, zero stale, and zero invalid new entries.  The canonical
  full-manifest audit is deferred to the orchestrator's force-clean rebuild
  because this isolated lane intentionally has no cached full report.
- A scratch transitive include census found 80 translation units affected by
  the seven edited headers.  Every TU was compiled twice from source: once
  against the exact `4c4ba569d` source/header archive and once against this
  candidate.  Exact sets were compared to January target objects: 80/80 passed,
  with zero compile failures and zero lost exact functions.  Notable sentinels
  are `units` 188->188, `objects` 121->121, `game_engine` 109->109,
  `weapons` 43->43, `players` 31->31, and `main` 11->11.
- The changed-file fake-match scan reports zero review leads.
- Focused COFF-comparator, fake-match-scanner, and parked-manifest tests pass:
  77/77.
- `git diff --check` passes.

## Do not repeat and reopen criteria

- Do not import HaloCEA's later thirteen-button/fancy-look layout into this
  January Xbox function.  Byte 11 is the scope-zoom control used for doubled
  spin in this target.
- Do not collapse clamped stick inputs into curve-mutated deltas: acceleration
  and magnetism activation consume the former.
- Do not move the local input/mouse representations into a shared header merely
  for tidiness; VC7 definition-position sensitivity has already caused silent
  unrelated exact regressions in this campaign.
- Do not alter the odd swapped/unswapped suppression flow in the 512-byte reset
  helper.  It is target-proven byte exact.
- Do not chase the two parked bodies with raw offsets, aliases, volatile or
  register qualifiers, barriers, forced inlining, assembly, or inert source.
  Reopen only for authoritative January source/local records or a natural
  compatible-compiler donor explaining their remaining allocation schedules.
