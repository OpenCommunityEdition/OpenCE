# `source/ai/props.obj` strategy ledger

Last validated against `build/split/source/ai/props.obj` with the normal XDK
3911 build. Baseline closeout was `6a1e60ae9`; exact-helper checkpoint was
`8cd09ac34`; the authenticated Fable/HCEA packet was revalidated on
2026-08-31. This ledger records strict COFF results only. No result below
uses inline assembly, byte patches, volatile scheduling anchors, or compiler
flag changes.

## Current inventory

- Code: 13/17 strict exact.
- Target-owned non-code: 13/13 strict exact.
- Protected exact code: `_props_initialize`, `_props_dispose`,
  `_props_initialize_for_new_map`, `_props_dispose_from_old_map`,
  `_prop_new_blank`, `_prop_remove`, `_prop_iterator_new`,
  `_prop_iterator_next`, `_prop_setup_orphan`, `_prop_orphan_transition`,
  `_prop_orphan_from_friend`, `_prop_orphan_update_information`, and
  `_prop_delete`.
- `_prop_setup_orphan`: `0xE0/0xE0`, 6/6 exact relocations, normalized SHA-256
  `c99acd468ec22ffc06c4bc8833aca8bc5eefb18a362eab04043e9cad87134c16`.

## Authenticated 2026-08-31 packet

Fable commit `79275995c484a2637aa5db570030d42a9e02685d` supplied the
source-shape lead.  The admitted portion is independently corroborated by
HCEA's `prop_add` body: a semantic `boolean dead` value feeds the three dead
fields, and the player arm precedes the non-player actor-type arm.  The exact
gains are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_prop_new_blank` | 48 | 3 | `1b53cc7129c7e6fd1e930379adc5149733e0db2e23f88c9eef8445afba369b56` |
| `_prop_orphan_transition` | 208 | 16 | `bb2e9672ebbb16c9c6b279c1ee9d2f4a8f478161ccbba2879c230b90844eb822` |
| `_prop_orphan_from_friend` | 240 | 18 | `95dda3e8ec8dd8915cc258fe81512fdcb6ca334eb7fffdc77c5cfae02ed509e1` |

The September 2001/Halo CE linker-map atlas independently recovers
`prop_add`, `prop_remove`, `prop_setup_orphan`, `prop_orphan_transition`, and
`prop_orphan_from_friend`; HCEA supplies `prop_new_blank` and recovers the
`prop_add(actor_index, unit_index, prop_index)` formal order.  January's call
sites prove the resulting private register/stack ABI, and the three private
helpers remain `static`.  Removing that linkage changes the calling convention
and regresses exact callers.  The reconstructed global
name `last_prop_data_full_warn_time` follows the adjacent
`last_garbage_warn_time` convention and replaces the forbidden address name.

The consumed external prototypes belong in `props.h`; private helpers remain
TU-local.  A direct sweep of all 18 C translation units that include that
header preserved every inherited exact function.  The rebuilt Props gate is
13 exact / 4 residual / 0 unwritten, and the consolidated semantic report has
zero unit errors.

## Accepted source controls

| Function | Source control | Strict result |
|---|---|---|
| `_prop_setup_orphan` | Use `vector_from_points3d`; capture identifier first, then owner/next/parent; restore owner/next/identifier/parent | Strict exact. |
| `_prop_get_active_by_unit_index` | Explicit swarm fallback plus an infinite traversal with an early next-index capture and guard exits | `0x90/0x90`, 5/5 relocation identities; four relocation addresses exact. Remaining difference is an `ECX`/`EDX` mirror and a one-byte `_prop_data` relocation drift. |
| `_prop_new_unacknowledged` | Keep separate `next_prop_index` and `prop_index`, assign current from next, and test `next_prop_index` at the header | Reduced candidate from `0x230` to target-exact padded size `0x220`; kept 21/21 relocation identities and recovered January's `EDI` next / `EBX` current roles. |
| `_prop_new_unacknowledged` | Nested allocation selection with an explicit `initialize_prop` join | Moved `datum_new` ahead of the reuse block to the target family and retained `0x220/0x220`, 21/21. Current candidate hash is `681249dad8d4...`; target is `4e1ed0d47f97...`. |
| `_prop_new_unacknowledged` | Store chosen prop index before its distance in both scan arms | Recovered the target's local store order without changing exact size or relocation count. |
| Object ownership | Keep `prop_data` declared only through the `extern` in `props.h` | The target `props.obj` carries `_prop_data` as an undefined external (`section=0`, `value=0`). Removing the local `= NULL` definition eliminated an incorrect 4-byte `.bss` section and reproduced the target symbol exactly. The storage is supplied by the campaign's `linker_common.obj`. |

## Measured and reverted experiments

| Function/family | Experiment | Result / first divergence | Decision |
|---|---|---|---|
| `_prop_get_active_by_unit_index` | Natural one-variable `while`/`for` traversal | Bottom-tested update and direct `ESI` loop index diverged before the target loop header. | Reverted. |
| `_prop_get_active_by_unit_index` | Combined predicate inside the infinite loop | `0xA0` padding and three relocation-address regressions. | Reverted; do not repeat. |
| `_prop_get_active_by_unit_index` | Declaration swaps and `prop_get(next_prop_index)` spelling | No binary change. | Reverted; do not repeat. |
| `_prop_setup_orphan` | Three component subtractions | `0xE0`, 6/6 identities, but x87 work was hoisted across preservation restores. | Reverted. |
| `_prop_add` cluster | `register` on the third formal | No private-ABI change. | Reverted; do not repeat. |
| `_prop_add` cluster | Local alias for `unit_index` | No private-ABI or COFF change. | Reverted; do not repeat. |
| `_prop_new_unacknowledged` | Declaration-order swaps | No prologue or ABI change. | Reverted; do not repeat alone. |
| `_prop_new_unacknowledged` | Declaration initializers versus separate initialization statements | Same prologue: actor index loaded before `_actor_data`; later allocation/reuse scheduling changed but did not close. | Reverted to the cleaner accepted form. |
| `_prop_new_unacknowledged` | Put `actor_get` declaration first | Actor call moved before local initialization, candidate grew to `0x230`. | Reverted. |
| `_prop_new_unacknowledged` | Comma-expression initializers inside `actor_get` | Moved `NONE` stores after float stores and worsened the prologue. | Reverted. |
| `_prop_new_unacknowledged` | Branch-specific `goto scan_next_prop` exits | VC7 still tail-folded all exits into the bottom-tested loop; no relocation improvement. | Reverted. |
| `_prop_new_unacknowledged` | Cast the required threshold to `short` | Recovered the target word compare, but moved allocation/reuse relocations three bytes farther from target. | Reverted. |
| `_prop_new_unacknowledged` | Redundant post-selection test and explicit allocation label | Compiler moved `datum_new` back behind the reuse block. | Reverted. |
| `_prop_add` cluster | Reverse the player/non-player source arms and add an explicit common-tail `goto` | VC7 still tail-duplicated the two link stores; return count stayed 3 rather than target 2. | Superseded by the authenticated HCEA branch shape. |
| `_prop_add` cluster | Hoist the actor pointer declaration to function scope | No code-generation change; return count stayed 3. | Reverted. |
| `_prop_add` cluster | Early `boolean has_unit = unit_index != NONE` | Removed the R5 3-return/2-return mismatch and triggered a two-stack-argument private convention, but VC7 promoted `actor_index` in `EAX`, not January's `unit_index`. Frame grew to `0x0C`; helper relocs began `31,38,...` versus target `22,29,...`. `_prop_orphan_from_friend` happened to recover all relocation addresses, while `_prop_new_blank`, transition, and `prop_new` regressed. | Reverted; useful only as proof that an early parameter-use anchor controls the caller cluster. |
| `_prop_add` cluster | Combine `has_unit` with a unit alias | Same wrong promoted parameter and frame. | Reverted. |
| `_prop_add` cluster | Reorder private formals to unit/actor/prop and reorder all calls | Optimizer still promoted semantic `actor_index`; no target ABI. | Reverted; this was not the authenticated actor/unit/prop order now retained. |

## Current bounded residuals and park criteria

| Function/family | Current strict state | Evidence-backed blocker | Reopen when |
|---|---|---|---|
| `_prop_add` | Target `0x260`, candidate `0x250`, 25/25 relocation identities; target hash `9b4cb38b...`, candidate `9934574e...` | The HCEA-backed `dead` local and positive player branch close the three callers naturally, but the helper retains a bounded tail-scheduling/size difference. | Authoritative January source or a natural source/lifetime form that closes the final 16-byte envelope without regressing the now-exact caller cluster. |
| `_prop_new_unacknowledged` | `0x220/0x220`, 21/21 identities; candidate relocations `11,51,74,81,210,353,359,375,382,409,414,419,425,447,452,457,463,477,484,500,520` versus target `8,51,77,84,205,362,368,384,391,418,423,428,434,456,461,466,472,486,493,509,528` | First divergence is prologue scheduling: target loads `_actor_data` at 6, initializes two `NONE` locals with `EAX`, then loads actor index and inserts an alignment NOP. Candidate loads actor index first and initializes the locals through `ESI`. Loop rotation then folds all backedges to a bottom test. | A typed `actor_get` source/lifetime shape that recovers the target prologue without raw `datum_get`, volatile, or assembly; or the correct helper ABI changes caller allocation enough to close it. |
| `_prop_get_active_by_unit_index` | `0x90/0x90`, 5/5 identities | Only `ECX`/`EDX` mirror around `prop_get`; `_prop_data` relocation is +1 while the call is exact. | A source-faithful register-lifetime distinction, not another predicate/declaration spelling. |
| `_prop_get_base_by_unit_index` | `0x190/0x180`, 15/15 identities | Target frame `0x4C`; candidate `0x44`. Ordered calls agree, but the target retains a longer scratch/register lifetime. | Layout evidence for the additional 8-byte lifetime or a newly identified typed scratch object. |

The object remains parked at 13/17 exact until one of these explicit reopen
conditions is available. The exact siblings and all target-owned non-code
sections are invariants for every future experiment.

Two select-any string COMDATs (`"prop"` and
`"prop->orphan_prop_index == NONE"`) remain locally emitted while the split
target records them as undefined external symbols owned elsewhere. They are
recorded as csplit COMDAT-attribution differences, not target-owned data
mismatches. The former local `_prop_data` BSS mismatch was not COMDAT noise
and has been corrected as described above.
