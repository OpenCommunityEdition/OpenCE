# Collisions static-local reconciliation, 2026-09-27

Baseline: `ef79aedaba16f9d38f61666041e62e57fad34140`.
Scope: the owner-approved RF-BF `offsets` packet only. Zero new code/data credit
and no object admission. The donor worktree was not modified.

## Source evidence and implementation

The 2003 PC demo and HCEX PDB dumps independently record the same static local:
`union real_vector3d offsets[0x11]`, owned by `collision_fix_pill` (demo RVA
`0x293c18`, HCEX RVA `0x1479060`). January corroborates a 204-byte initialized
array at file offset `3198440`, with its only code reference in that function.
The later PDBs establish name, type and function scope, not the exact position
of the declaration within the block. The source comment discloses that limit.

Move the unchanged initializer from file scope to the function's natural
declaration block, rename its two uses to `offsets`, and change one existing
`symbols.json` row to `?offsets@?1??collision_fix_pill@@9@9`. No new symbol,
initializer, padding, helper, header or compiler setting is introduced.

Donor evidence: `claude-remaining-frontier-20260926`'s
`research/remaining_frontier_20260926/workers/RF-BF/`, specifically the original
PDB extracts E1, January references E2, prospective card C01 and the collisions
patch. The 2026-09-27 owner ruling admits this storage-form correction only if
fresh gates preserve existing exact rows. No scope/name tuning was performed.

## Fresh canonical checks

- Full `ninja all_source progress build/report.json` passes with the unchanged
  objdiff 3.3.1 scorer and normal split regeneration.
- All 20 collisions functions remain strict exact, including the newly exact
  `collision_move_point` (4,752 padded bytes).
- Stable board: 8,252 owner rows, 7,641 exact; zero gains and zero losses.
- All 42 January-owned sections and 44 symbols pass identity, flags,
  alignment, offsets, storage and relocation checks after the rename.
- All 40 surplus code/data sections match January's selected and current
  providers; no COMMON, new surplus, or ownership exception. All 32 two-order
  provider probes pass duplicate-definition checks. They are not a full game
  link; unresolved external diagnostics are expected in these probes.
- Parks 71 active / 0 stale / 0 invalid; admission 13 candidates / 0
  contradicted / 1 rejected / 0 revoked; fake-scan leads unchanged at 26.
- Tests: 1,161 passed / 5 skipped / 26 subtests. `git diff --check` clean.
- Halo meaningful credit remains 1,598,242 / 1,770,166; objects remain 389/468.

Receipts are under `scratch/astra_object_closeout_20260927/`: `gates/`,
`collisions_static_probe/`, `collisions_static_final/`, `collisions_bytes/`,
and `collisions_policy/`. The final audit binds current source, objects, config,
tools and providers by hashes, with a successful end-of-run input recheck.

## Whole-object admission remains held

Byte identity is not sufficient to certify the entire source. Independent
policy review found nine inherited `COLLISION_POINT_FROM_LINE3D` hand-expanded
sites and the separately held radius/component parentheses. Their existing
function-level credit is not newly approved or revoked by this packet.

Fresh bounded probes on the static-local form: restoring the genuine helper
alone leaves 19/20 exact (loses `collision_move_point`); stripping the two
decorative operand parentheses leaves 18/20; combining both leaves 17/20.
The genuine helper's extra COMDAT is identical and links against its provider
in both orders, but that does not authorize its nonexact caller. No probe is
landed, and no status flip is made. Preserve those candidates and receipts
under `collisions_helpers/`; do not repeat the same controls without new
source evidence or an applicable ruling.
