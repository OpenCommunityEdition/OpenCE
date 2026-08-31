# actor_combat.obj collateral-damage rejection (2026-08-31)

## Scope

This audit tested only `_actor_combat_check_collateral_damage` from
`C:\halo-worktrees\claude-breakable-surfaces-20260830`.  The donor is a
readable HCEA-correlated reconstruction: typed prop and encounter-actor
iteration, named actor/prop fields, ordinary distance tests, and no raw
overlays, inline assembly, forced inlining, or code-generation carriers.

## Exact candidate proof

After moving the encounter-actor iterator layout and declarations into the
existing iterator-owner header, the cleaned body compiled strict-exact:

| measurement | target and candidate |
| --- | ---: |
| meaningful bytes | 872 |
| padded bytes | 880 |
| relocations | 21 |
| normalized SHA-256 | `eb4aee9d9f4765a5505126a61d5ef2bdabe4c40f9de3cba7d3cd7cc9771ee0d0` |

`tools/campaign/gate.py source/ai/actor_combat` moved the unit from 11 to 12
exact functions with no residuals.  The target really does call
`actor_variant_definition_get` near the start of this function.  Removing
that HCEA operation produced an 864-byte/20-relocation residual, so the call
was not introduced as a synthetic match carrier.

## Rejection reason

The function is called from `actions.c`, but its implementation owner is
`actor_combat.c`.  Moving its prototype from the foreign translation unit to
the existing `/* prototypes/ACTOR_COMBAT.C */` section of `actors.h` is the
correct placement under the header-ownership house rule.  That otherwise
source-only cleanup reopened a protected complete object:

| gate | before | with correct `actors.h` prototype |
| --- | ---: | ---: |
| `source/units/units` | 189 exact / 0 residual | 188 exact / 1 residual |
| `source/ai/actor_combat` | 11 exact / 23 unwritten | 12 exact / 22 unwritten |

A sweep of all 42 direct `actors.h`/`actor_iterators.h` consumers found no
other exact-function loss.  A new `actor_combat.h` would avoid the compiler's
header-position sensitivity, but it violates the campaign rule that a C file
without its own header must use the closest associated header.  Keeping the
prototype local to `actions.c` would also violate prototype ownership.

The entire source/header packet was therefore reverted.  Post-revert one-TU
gates restore the protected baseline:

- `source/units/units`: 189 exact, 0 residual, 0 unwritten;
- `source/ai/actor_combat`: 11 exact, 0 residual, 23 unwritten;
- `source/ai/actions`: 57 exact, 6 inherited residuals, 0 unwritten;
- `source/ai/encounters`: 12 exact, 0 residual, 49 unwritten.

This function remains an exact candidate blocked by header ownership and the
protected `units.obj` regression.  It must not be admitted until the correct
shared-header placement preserves all 189 `units.obj` functions.
