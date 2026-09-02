# `players.obj` January-safe harvest — 2026-09-02

## Admitted result

This pass audited the farther local Players candidates from canonical
`429497bbe` and admits only two source-authenticated corrections:

| Function | January padded bytes | Relocations | Result |
|---|---:|---:|---|
| `players_initialize` | 112 | 13 | strict exact |
| `players_initialize_for_new_map` | 176 | 19 | strict exact |

Both functions contained an extra reconstruction-time write to
`players_globals->local_player_triggered_switch`.  January's target contains no
such store, and the independent HCEA `players_initialize.c` and
`players_initialize_for_new_map.c` donors omit it.  Existing game-loop sites
continue to own the packed BSP-switch trigger lifetime.  Removing those two
unsupported writes is ordinary semantic cleanup, not compiler steering.

The focused strict gate moves from 52 exact, 17 residual, and one unwritten
function to 54 exact, 15 residual, and one unwritten function: 288 padded code
bytes gained.  Every exact sibling and all non-code ownership remain intact.

## Candidate and donor audit

Every existing Players ledger was read before editing.  Candidate source and
objects were compared across `fable/exact-finish-20260829`,
`claude-breakable-surfaces-20260830`, and the registered local worktrees.  The
semantic donors were the extracted HCEA Players routines under
`../../research-cache/halocea-full-blobs-20260830/src/blam/game` and the
independent Stian/Pastudan PC lifts.

The open Marathon source at commit
`ce4fdc63f20f6fa39616a86e717118f4d8bdf25c` was checked as a first-party style
analogue, not a Halo code oracle.  Its `marathon2/items.c::swipe_nearby_items`
keeps nearby interaction in a normal typed traversal and
`marathon2/player.c::process_player_powerup` uses semantic enum cases and
direct state updates.  No Marathon-specific logic was transplanted.

## Farther bytes rejected by the credibility gate

The Fable object's larger exact set depends on source shapes that are not
authenticated as original January source:

- `player_handle_powerup` manually copies two private helper bodies.  Keeping
  ordinary calls preserves the exact private sections; manual expansion makes
  `player_powerup_additional` disappear unless a dead `if (0)` symbol emitter
  is added.  The Fable `long powerup_index` carrier is likewise unsupported.
- `player_handle_powerup_minor` manually duplicates `player_powerup_on`.
  HCEA confirms the later behavior, but that semantic equivalence does not
  prove January used the duplicated source instead of an ordinary helper call.
- `player_set_action_result` manually expands `distance3d` through two
  specially ordered vector calculations, and the farther candidate also uses
  `__declspec(noinline)`.  Target code shape alone does not authenticate that
  spelling over the retained named math helper.
- `player_update_powerups` needs `_ReadWriteBarrier` and a copied
  `player_powerup_off` body; the copy otherwise removes the exact private
  helper section.
- `player_examine_nearby_vehicle` re-fetches the same vehicle into a second
  pointer and needs forced `noinline` to close its tail.  HCEA uses one typed
  vehicle pointer, so the duplicate fetch is not admitted as source.
- `player_examine_nearby_device` needs a volatile reinterpretation of the
  sphere radius solely to force x87 argument materialization.
- `player_examine_nearby_item` needs a volatile reinterpretation of the weapon
  count solely to force an EBX spill.

Ordinary declaration-order, local-lifetime, parameter-width, and source-order
variants were also tested.  They either returned to the retained fixed point,
removed an exact private section, or disturbed more code.  No barrier,
volatile access, dead emitter, forced inline/noinline, register hint, assembly,
fake dependency, duplicate lookup, raw address field, or nonsensical branch is
retained.

The three task-specified canonical parks -- `unit_should_autopick_weapon`,
`player_teleport_on_bsp_switch`, and
`players_reconnect_to_structure_bsp` -- remain byte-for-byte unchanged.  The
existing `players_compute_combined_pvs` park is unchanged too.

## Target-only one-byte function and header blast

`player_examine_nearby_unit` remains target-only.  January owns one `ret` plus
alignment, while Stian's recovered declaration and both PC callers establish
the two-handle boolean ABI.  Pastudan records the XBE stub, but no first-party
source explains an undefined boolean return.  A `void (void)` no-op is an ABI
fiction; a boolean false return does not match.

The no-argument experiment also changed
`units::_unit_preprocess_node_orientations` from exact to residual through C2
definition-position sensitivity.  It was fully reverted.  `players.h` is
content-identical to `429497bbe`; the full rebuild covers every includer; and
Units remains 189/189.

## Validation

- full `ninja all_source progress build/report.json`: pass;
- strict whole-tree verdict against the fresh `429497bbe` build: two gains,
  zero regressions across 8,245 mapped functions;
- focused gates: Players 54/70 exact with one target-only function; Units
  189/189 exact;
- `python -m tools.parked_functions`: 86 active, zero stale, zero invalid;
- `python -m tools.audit_semantic_matches`: zero unit errors;
- `python -m tools.audit_object_admission`: zero contradictions and zero
  revocations;
- fake-match scan of `players.c`: zero findings;
- `python -m pytest -q -p no:cacheprovider`: 255 passed;
- built Players object: no `point_from_line3d` symbol or COMDAT;
- `git diff --check`: clean.
