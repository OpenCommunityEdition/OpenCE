# source/game/players

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/12: unit_create_starting_weapon->create_weapon (static). Zero credit.

## evidence
Identical to Sept; the caller player_add_equipment matches. HCEX.pdb (2011, players.c) also names it create_weapon. The current name is an atlas C++ propagation from another lineage, contradicted by the Sept retail cache.exe map. Gate 68/2 with SAME rows; objcmp identical; objdiff unchanged.

## blockers
Residuals _player_examine_nearby_device and _player_teleport_internal; audit FAIL(2) and pdb 2 are unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md