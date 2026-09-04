# HUD Unit state migration and ownership cleanup — 2026-09-04

## Scope and result

Canonical baseline: `96086c9c4c187dd539af13bdaa9252d6e46ad8f6`.
The previously unwritten `hud_fix_unit_data` is reconstructed as ordinary typed
C and passes the strict gate at 128 padded / 118 meaningful bytes. HUD Unit advances from
17 exact / 0 residual / 5 unwritten to 18 exact / 0 residual / 4 unwritten.
Every inherited exact function is preserved. This is not a whole-object
completion claim.

January COFF is the byte/ABI authority. The current canonical HUD ledgers,
campaign throughput lessons and matching methodology were reviewed first.
The read-only HCEA corroboration is
`research-cache/halocea-full-blobs-20260830/src/blam/interface/hud_fix_unit_data.c`
under the parent campaign workspace. Its later-build implementation is not
allowed to override January's assertions or calling convention.

The function asserts both local-player indices are not NONE at January source
lines 0x1AB and 0x1AC, retrieves the two real HUD-state records through the
existing private accessor, and copies the complete 88-byte record by typed
structure assignment. There are no raw offsets, casts, fabricated loops,
artificial lifetimes, assembly, forced inlining or optimizer controls.
The public prototype already belonged to `interface/hud_unit.h`.

## House-rule reconciliation

The three vitality/fade scalars now use the project's `real` typedef instead
of raw `float`. Their authenticated layout remains unchanged.

The old external address placeholder `bss_00453ac0` and its two macro aliases
are replaced by the actual file-static `unit_hud_globals` pointer initialized
to NULL. January identifies a four-byte zero BSS owner at file offset
4,537,024, and its references are confined to this object. The semantic name
is also present in January's assertion text. `config/symbols.json` records
that name and private linkage; it does not alter contribution extents or
target bytes. The source now owns the real storage instead of relying on an
undefined address-labelled external. Independent COFF review verifies its
logical zero-storage SHA-256 as
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`,
size four, with static storage class 3 in the candidate. Final split
regeneration must also apply the corrected static linkage to the target.

The six scripted show/blink API declarations are moved from the consumer
`hs.c` into the owning `hud_unit.h`, which HS now includes. The direct header
consumers found before the edit were HUD Unit, HUD, Players and Damage (with
HS added by this packet); all are covered by the final dependent build and
stable sweep. No shared structure layout was edited.

This wave does not speculate about the two inherited anonymous empty
initializer bodies or claim to have resolved the remaining HUD render/sound
functions. In particular, the private 64-byte state initializer is not given
fake public linkage or an artificial reference to keep it emitted without
its authentic, still-unwritten rendering caller.

## Isolated validation

The migration-only candidate is
`scratch/hud-unit-migration-20260904.obj`. The owner-cleanup candidate is
`scratch/hud-unit-owner-cleanup-oldalias-20260904.obj`; before regenerating
the split, the latter used the diagnostic compiler macro
`/Dunit_hud_globals=bss_00453ac0` solely to compare with the old target label.
That alias is not in production source or build flags. Final integration
must use the renamed target and no diagnostic alias.

Both candidates retain all 17 inherited strict exact functions and close
only `hud_fix_unit_data`. The complete emitted-symbol guard reports no
`point_from_line3d` owner. The HS owner-header cleanup gates at its inherited
445 exact / 3 documented residual / 0 unwritten. The conservative fake-match
scan reports no leads in HUD Unit or its header.

Full-build, stable-sweep, parked-policy, tests and publication results belong
in the integration batch ledger; isolated gate results alone are not a claim
that this packet has been published.
