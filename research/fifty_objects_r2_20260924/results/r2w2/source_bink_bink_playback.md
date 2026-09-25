# source/bink/bink_playback (hygiene)

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/bink_playback/PACKET_bink_playback.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/bink_playback/01_bink_playback_static_storage.patch",
 "C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/bink_playback/02_symbols_json_static_storage.patch"
]

## production_changes
Apply PACKET_bink_playback.patch, then run a csplit-only regeneration; both halves must land together. The source makes bink_pointer_blocks, bink_bitmap, bink_texture and bink_pointer_block_count static. symbols.json rows 23308, 23309, 23310 and 23313 get static:true. The round-1 storage.patch was a plain diff -u that no longer applied, so both halves were regenerated from HEAD.

## evidence
cachebeta publics contain only _debug_bink and _bink_globals among the six .bss rows, and no other source file names the four objects. Emulated split: only bink_playback.obj changes. Gate: 23/1 before and after (the q3 owner row). object_audit: FAIL(1) before and after, that function only, with 59 symbols and 0 differing. pdb_storage goes from 5 to 1. Surplus is unchanged and provider_link PASSes. Sweep: 611/612. /W3: 14 in both. fake_scan: 1 pre-existing int 3 lead, unchanged. objdiff is identical.

## blockers
_bink_query_analog_controller_buttons needs the owner's q3 ruling. _code_001b5850 has an address-derived name (owner naming item; a static copy would be dropped).

## reopen_criteria
The owner rules on q3 and on the _code_001b5850 name.

## manifest
C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/bink_playback/MANIFEST.md

ledger: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/LEDGER.md (index MANIFEST: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/hygiene_r2w2/MANIFEST.md; checksums patches_sha256.txt)
