# `items.obj`: complete `_item_update` reconstruction and fuzzy park

Date: 2026-09-01

## Result

`source/items/items.obj` is complete under the campaign's fuzzy-park rule.
All 18 functions have complete source: 17 are strictly exact and the sole
residual, `_item_update`, is a coherent behavior-complete reconstruction with
the exact target size, stack frame, and relocation inventory.

- January target: 2,352 padded bytes, 110 ordered relocations, normalized
  SHA-256
  `78237a8528e5d6a858b7aced1df0d8037225ec19da1b3b625c74c900d8ee8b02`.
- Retained candidate: 2,352 padded bytes, 110 ordered relocations, normalized
  SHA-256
  `3daa5a96ffd38c2e133967126ca722610b145121e9690eb92a76f5a026b736f5`.
- Objdiff score: 95.414246%.
- Strict gain: zero functions and zero padded bytes.
- Parked scope: one complete function and 2,352 padded bytes. The strict
  progress reporter correctly reserves matched-byte and matched-object credit,
  so this remains 17/18 strict while the residual is explicitly parked.

The source does not claim byte exactness. The repository classifier reports
`UNKNOWN`, so `config/parked.json` deliberately records the fail-closed class
`unclassified` rather than assigning a more specific diagnosis from visual
inspection.

## Source census and reconstruction lineage

The audit covered every existing Items object ledger, complete repository
history, and all 1,554 registered worktrees. The worktree population collapses
to five distinct `items.c` source blobs. Their significant `_item_update`
frontiers were:

- canonical `5d507e24a`: 2,368 bytes and approximately 86.31%;
- lineage `1ed2fd9e`: 2,352 bytes and approximately 93.31%;
- lineage `e02971df`: 2,352 bytes and approximately 95.25%.

No worktree, dangling commit, or uncommitted artifact contained an exact body.
The `e02971df` lineage was therefore used only as the strongest complete
topology and was reconciled against the current canonical types and house
rules rather than copied as an authority.

External evidence was also read directly:

- the HCEA item-update donor establishes the complete later-build behavior;
- Stian's full-history reconstruction contains the closest January-shaped
  x86 topology and target-disassembly notes;
- Pastudan is incomplete in this function;
- Marathon's open source corroborates period C style and item-state ownership.

The January COFF object remains authoritative for layout and code generation.
The resulting body covers initialization, orientation repair, gravity and
collision integration, material effects and collision sound, rest detection,
structure and object supports, bounce response, angular damping, interpolation,
garbage collection, and lifetime deletion. No unwritten block remains.

## Admitted source changes

The retained source replaces the artificial combined vector carrier with the
natural `real_vector3d velocity` and `real_point3d candidate` locals. This
recovers the target's 0xC8 frame and its `[ebp-0x18]` velocity /
`[ebp-0x0c]` candidate ownership. The early orientation scratch intentionally
uses the candidate storage as a `real_vector3d`, matching the January lifetime
reuse without inventing a carrier type.

The complete topology also retains a natural collision-sound index lifetime,
the target-corroborated angular-dot grouping, collision point / reflected
velocity interleaving, and direct readable rest-support control flow. Flag
changes use `SET_FLAG`, typed object and item access remains behind subsystem
macros, and scalar types use cseries definitions.

Address-derived identifiers were removed:

- `code_000e6150` became `verify_item_location`;
- `code_000e6370` became `item_adjust_for_angular_velocity_change`;
- `code_000e6900` became `item_align_to_normal_and_point`;
- `rdata_00278fb0` became `item_maximum_impact_velocity`;
- `data_00306498` became `item_update_section`.

The public `item_update` prototype now belongs to `items/items.h`.
`objects/object_types.c` includes that owner header and no longer carries ad
hoc item declarations. The change does not move structs into a shared header,
so it avoids the previously measured C2 definition-position blast radius.

The existing January inline schedule remains intact. In particular, this
translation unit continues to reference the configured `point_from_line3d`
helper without defining or owning an additional COMDAT.

## Residual boundary

The target and candidate have the same padded envelope, exact 0xC8 frame, and
the same 110 relocation identities in order. An aligned normalized-disassembly
comparison reduces the residual to bounded compiler scheduling choices:

1. early candidate integration and collision-call setup;
2. two final collision-sound location stores;
3. angular-dot term scheduling;
4. flag read/modify/write selection;
5. support-object index and rotation-axis copy scheduling;
6. the final reflected-velocity x87 multiply form;
7. rotation-cosine load placement.

After the rested-item validation region, the meaningful tail is
instruction-for-instruction equivalent modulo a seven-byte accumulated offset.
The candidate reaches its meaningful return at offset 0x928 and the remaining
seven bytes are ordinary target-envelope alignment NOPs, not fabricated logic.

## Exhausted natural probes

Each bounded candidate was compiled with the PID-isolated campaign gate and
inverse-restored if it lowered the result. The following shapes either emitted
the same object or worsened size and score:

- raw flag operations versus `SET_FLAG`;
- alternate angular-dot association and split temporaries;
- alternate bounce operand ordering and source interleaving;
- storing the support object index before the node-matrix call;
- a separate support-index local;
- explicit rotation-axis component copies;
- vector-helper integration instead of direct component expressions;
- clamp and declaration-order variants.

The store-before-call and explicit-copy candidates reopened the 2,368-byte
bucket. The support-index local and vector-helper forms retained size but
scored lower. None explained the residual as a whole.

No `register` or `volatile` steering, compiler barrier, pragma, inline
assembly, fake dependency, inert carrier, undefined behavior, forced inline,
raw offset access, address-derived name, or nonsensical exactness control is
retained. Reopen this function only for authenticated January source or local
records, or a natural same-compiler donor that explains all remaining
scheduling windows together.

## Verification

- focused Items gate: 17 strict exact, one validated parked residual, zero
  unwritten;
- `object_types.obj`: 31/31 strict exact after prototype-owner cleanup;
- every direct `items.h` consumer remains strict exact:
  `equipment.obj` 3/3, `garbage.obj` 2/2, `hs_library_external.obj`
  36/36, and `object_types.obj` 31/31;
- protected family gates remain exact: `units.obj` 189/189 and `objects.obj`
  121/121;
- full Ninja build, semantic report, and progress report: pass; 472 units and
  5,162 functions evaluated, 5,026 semantic-exact, 5,037 accepted-exact, and
  zero unit errors;
- parked-function validation: 72 active, zero stale, zero invalid;
- fake-match scan: no findings in the three changed source/header files;
- full tooling test suite: 255 passed;
- admission audit: zero contradictions or revocations; its sole candidate is
  the unrelated pre-existing `player_rumble.obj` ownership review;
- Git whitespace check: pass.
