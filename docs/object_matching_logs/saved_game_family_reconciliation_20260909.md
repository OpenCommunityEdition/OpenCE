# Saved-game family exact-looking candidate rejection (2026-09-09)

## Decision

The saved-game packet from Opus lane
`opus/30k-ui-script-profiles-20260906` at `08d6bea92` is rejected.  Its three
strict-looking closures total 1,090 meaningful / 1,120 padded bytes, but each
restores a failure-path defect already excluded by the campaign house rules.
The accompanying near-exact write-thread change also restores serialization of
uninitialized stack padding.  All four candidates receive **zero exact
functions and zero exact bytes**.

The candidate source changes were removed.  Canonical's existing defined,
house-rule-compliant implementations remain in
`source/saved games/player_profile.c` and
`source/saved games/saved_game_files.c`.  No header, compiler flag, symbol map,
semantic allowlist, or parked-function record changes as part of this audit.

## Mechanical measurements of the rejected packet

Fresh production-flag compiles through `tools/campaign/gate.py`, including the
`--forbid-emitted-symbol _point_from_line3d` guard, measured the rejected
candidate as follows:

| Function | Meaningful | Padded | Candidate gate | Credit |
| --- | ---: | ---: | --- | ---: |
| `_saved_game_files_enumerate_available_to_local_player_index` | 359 | 368 | strict exact | 0 |
| `_player_profile_new` | 456 | 464 | strict exact | 0 |
| `_player_profile_create_default_profiles_on_disk` | 275 | 288 | strict exact | 0 |
| `_player_profile_write_thread_proc@4` | 349 | 352 | residual `[sha]` | 0 |

The complete rejected-candidate gates reported `46 exact / 0 residual / 0
unwritten` for `saved_game_files.obj` and `20 exact / 2 residual / 0 unwritten`
for `player_profile.obj`.  The write thread had 116 target and 116 candidate
instructions, the same padded extent and relocation count, and one raw code
byte difference at `+0x10b` (`09` target versus `21` candidate).  Those strong
mechanical results do not waive rules 20 and 21: byte similarity cannot admit
undefined or nonsensical source.

`tools/fake_match_scan.py --fail-on-findings` reported zero lexical review
leads across the two candidate source files.  This is expected: the scanner is
a review aid and does not diagnose the invalid-index, lifetime, or
uninitialized-storage failures below.

## Semantic rejection reasons

### Saved-game enumerator

The exact candidate removed the declaration initializer for
`number_of_available_profiles` and assigned zero only after the general mutex
was acquired and the mapfile entry count was read.  The final
`*number_of_profiles = number_of_available_profiles` store is unconditional.
When acquisition of `saved_game_files_globals.general_mutex` fails, the local
therefore remains uninitialized.  The candidate ledger's assertion that the
failure-path output stays zero was false.

This is the same defect already called out by the existing
`config/parked.json` evidence for this function.  Canonical keeps the
declaration initializer, remains a 368-byte residual, and receives zero exact
credit.

### New player profile

The exact candidate moved `saved_game_file_close` after the write-failure
block.  That block deletes the enumerated file and sets
`player_profile_index = NONE` before the close call.  On seek or write failure,
the candidate consequently calls `saved_game_file_close(&file, NONE)`.
`saved_game_file_close` immediately decodes and asserts the type, memory-unit,
and file-index fields of that argument; `NONE` is not a valid opened index.

The existing parked evidence explicitly requires closing with the valid opened
index before deletion and before changing the returned index to `NONE`.
Canonical retains that ordering and its fully initialized 512-byte file block.
The rejected 456 meaningful / 464 padded exact result receives zero credit.

### Player-profile write thread

The near-exact candidate replaced the zero-initialized 512-byte typed file
block with an uninitialized byte array.  It initialized only the 48-byte
profile payload and the 20-byte signature, then wrote the full 512 bytes.  The
remaining 444 padding bytes came from uninitialized stack storage.

That is both an information-disclosure risk and a serialized nondeterministic
representation.  The existing parked evidence specifically requires
initialized padding.  Canonical retains the initialized typed wrapper, remains
384 padded bytes and residual, and receives zero exact credit.  The rejected
352-byte near-match also receives zero credit regardless of its single branch
displacement difference.

### Default profiles on disk

The exact candidate likewise replaced the initialized typed block with an
uninitialized 512-byte array, writing 444 bytes of stack padding after the
profile and signature.  It also folded `file_set_position` into the
`file_create && file_open && file_set_position` condition while leaving
`file_close` inside the condition body.  If open succeeds and seek fails, the
opened file is never closed.

Both defects are explicitly excluded by the existing parked evidence, which
requires initialized serialized padding and a close after every successful
open, including seek failure.  Canonical preserves both properties.  The
rejected 275 meaningful / 288 padded exact result receives zero credit.

## Preserved baseline and accounting

After removing all candidate source changes, the complete guarded gates return
to the established baseline:

```text
source/saved games/saved_game_files: exact 45, residual 1, unwritten 0
source/saved games/player_profile:   exact 18, residual 4, unwritten 0
```

No inherited exact owner is lost.  The forbidden `_point_from_line3d` emitted
symbol remains absent.  Net accepted source gain is **0 meaningful / 0 padded
exact bytes**; net fuzzy-source gain is also zero because the unsafe candidate
bodies were not retained.
