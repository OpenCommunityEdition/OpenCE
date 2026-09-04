# `damage.obj` object-damage update (2026-09-04)

## Result

This isolated January-PC packet reconstructs `object_damage_update` in
`source/objects/damage.obj` as ordinary typed C and makes it **strict exact**.
The target and candidate are both 1,056 padded bytes, carry the same 39
relocations, and have normalized SHA-256
`3f30cad51b41611c1576b503d45dabfb17a763b1c74842b07f17bcfea7ea1103`.
The function contains 1,042 meaningful bytes.

The focused Damage gate advances from **25 exact / 1 residual / 8 unwritten**
to **26 exact / 1 residual / 7 unwritten**. The pre-existing
`area_of_effect_cause_damage_to_object` fuzzy result remains unchanged. A
rename-stable sweep of all 8,245 target function sections records **+1 exact
function / +1,056 padded bytes / zero regressions**. Overall strict reporting
advances by **+1 function / +1,042 meaningful bytes**, from the inferred
886,242 bytes and 6,085 functions at the lane base to 887,284 bytes and 6,086
functions after this packet.

## Isolation and authority

Work was performed only in
`worktrees/damage-object-update-20260904`, branch
`agent/damage-object-update-20260904`, from canonical commit
`aafd066dfab61f595ccaa1477e339121ddd2face`. The canonical checkout was not
edited and this packet was not pushed.

The regenerated January split object is
`build/split/source/objects/damage.obj`, SHA-256
`D66824B6C5EDE819391847D4B7540C485173685DE2704494BD1E681FE6A04DEB`.
Its COFF sections, storage classes, ABI, instructions, relocations, padding,
and bytes are the sole match authority. The final rebuilt candidate object is
`build/base/source/objects/damage.obj`, SHA-256
`94E2B2487C2F7CD0771D4FE662F374D893877613296D350161DD42DFE98E7248`.

All existing Damage ledgers were reviewed before reconstruction. Independent
semantic/name corroboration came from:

- HCEA's `src/blam/objects/object_damage_update.c` and
  `src/headers/game_globals_falling_damage.h` under
  `research-cache/halocea-full-blobs-20260830`;
- Stian's January-PC lift in `src/halo/objects/damage.c` under
  `research-cache/stian-halo-current-20260827`;
- Pastudan's disassembly-backed `src/halo/objects/damage.c` under
  `research-cache/pastudan-halo-20260828`; and
- Marathon's open source only for broad Bungie source conventions.

No donor was allowed to override the January object. In particular, the
later HCEA recharge-on-melee gate and counters, its dead-biped network
baseline synchronization, and its later object-datum role logic are absent
because January's code and relocation topology do not contain them.

## Reconstructed behavior

The public owner now uses the existing semantic `object_damage_update` name
and keeps its declaration in `objects/damage.h`. The implementation:

- obtains the typed object, object definition, and collision-model definition;
- handles the three pending act-of-god death flags by constructing typed
  `damage_data`, resolving the falling-damage tag from game globals, applying
  the January kill/silent/no-statistics flag combination, and clearing the
  pending object flags;
- performs shield overcharge accumulation, engine-dependent overcharge drain
  and HUD tick reporting, stun countdown, difficulty-scaled recharge, and the
  shield-depleted recharge effect/permutation reset; and
- advances and clamps both body and shield recent-damage decay state before
  retiring each timer.

The TU-private `game_globals_falling_damage` definition reproduces the
authenticated 0x98-byte January layout and asserts the damage-effect tag index
at offset 0x1c. Keeping that definition in the owner C file avoids the known
VC7 definition-position sensitivity caused by unnecessary shared-header
changes. Access uses the project object/tag macros and typed owners; source
uses project `real`, `boolean`, enum constants, flag macros, `TICKS_PER_SECOND`,
and `MAX` rather than raw offset or hand-written bit tests where project
facilities exist. Every new parameter is on its own line and the function has
an explicit terminal `return;`.

## Exactness convergence and credibility

The first complete typed reconstruction produced a natural 1,024-byte
candidate. Correcting January-authenticated branch and expression shapes
produced a 1,056-byte candidate at 91.549296% normalized instruction
similarity, with only one meaningful store-scheduling difference. Placing the
source-authentic `damage.scale = 1.f` assignment before the kill flag update
then made the entire function strict exact. This is an ordinary independent
statement-order choice with direct semantic meaning, not a synthetic
dependency or compiler-control device.

No inline assembly, volatile/register forcing, compiler barrier, pragma,
forced inline/noinline, address-derived name, raw offset, representation pun,
dummy dependency, undefined behavior, contradictory predicate, or
nonsensical branch is present. The rebuilt symbol table contains no
`point_from_line3d` symbol or COMDAT, preserving the required January inline
schedule. Because the function is strict exact, no fuzzy park is needed.

## Verification

- Full `ninja halobetacache_build libcmt_build semantic_progress progress`:
  pass.
- Final focused Damage gate: **26 exact / 1 residual / 7 unwritten**.
- Rename-stable snapshots: 8,245 functions, 6,133 to 6,134 exact;
  **+1 / +1,056 padded bytes / 0 regressions**.
- Semantic report: 473 units scanned, 6,519 functions evaluated, 6,143
  semantic exact, 169 hidden exact / 101,147 hidden code bytes, 6,159 accepted
  exact, and zero unit errors.
- Overall strict progress: **887,284 / 2,198,102 meaningful code bytes** and
  **6,086 / 11,060 functions**.
- Protected Units gate: **189 exact / 0 residual / 0 unwritten**.
- `python -B -m tools.parked_functions`: **233 active / 0 stale / 0
  invalid**.
- `python -B tools/audit_object_admission.py`: zero candidates,
  contradictions, or revocations; the two inherited reviewed rejections are
  unchanged.
- Changed-file fake-match scan: one source file, zero review leads.
- `python -B -m pytest tools -q -p no:cacheprovider`: **261 passed**.
- `git diff --check`: clean apart from informational line-ending warnings.

The final source hashes are:

- `source/objects/damage.c`:
  `9660CCCBAB9205EAA1A7F465DF5F78BAFF4369E41663C4DCE552D3DAFFD9DB02`;
- `source/objects/damage.h`:
  `E11E985935F129F8569B79B9E2F7C00A4498030476C1468479FE6AEBED525FFB`.

Lane-local stable snapshots and diagnostic candidates remain under `scratch/`
and are intentionally uncommitted.
