# `saved_game_files.obj`: `enumerate_memory_units` reconstruction handoff

Date: 2026-09-04

Status: complete natural-C reconstruction, ready for root review as an honest
fuzzy park. This document does not claim strict equality or whole-object
`Matching` admission.

## Scope and frozen evidence

- Published baseline supplied by root: `56635021ee5f664a56680f7c805cafd8e76d5e71`.
- Baseline focused gate: 39 strict exact, 6 existing residuals, 1 unwritten.
- Final focused gate: 39 strict exact, 7 residuals, 0 unwritten.
- Target: `_enumerate_memory_units`, 992 padded bytes, 52 relocations,
  normalized SHA-256
  `bb78e47ec36edad43dd58bc245c26cc86944a94a7eb4858c963516799bcfe75c`.
- Candidate: 1008 padded bytes, 52 relocations, normalized SHA-256
  `ccb50a7ffa83865a349947689eb2e27427ab5efe0031e980bb95df3340de6096`.
- Target/candidate relocation destination sequences are identical. Relocation
  addresses shift after the residual loop-layout window, so strict relocation
  equality is not claimed.
- Candidate object:
  `scratch/saved-game-enumerate-memory-units-final-natural-20260904.obj`, file
  SHA-256
  `529C093FE561644FE147EA59C9C287396B65B2486FD4442A4BAA0E31673F421A`.
- Source edit manifest:
  `scratch/saved-game-enumerate-memory-units-first-shot-20260904.json`, file
  SHA-256
  `4A21830DB2D0BCA1AB7740695E8EC4BCC7FF7CD89145A3DFDF7E9605CDC81F51`.
- Exact gate command:

  `python tools/campaign/gate.py "source/saved games/saved_game_files" --edits scratch/saved-game-enumerate-memory-units-first-shot-20260904.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/saved-game-enumerate-memory-units-final-natural-20260904.obj`

The emitted-symbol guard passed with the exact underscored COFF name. A full
code-owner census found 46 target and 46 candidate code definitions, with no
candidate-only or target-only owner names. All 39 inherited exact functions
remain exact. Direct BASE-versus-candidate comparison also found all six
inherited residual sections unchanged.

## Authority and ABI

January is the byte and ABI authority. The target is public/external COFF
storage class 2, cdecl `_enumerate_memory_units`, and has the already-correct
owner declaration `void enumerate_memory_units(void)` in
`source/saved games/saved_game_files.h`. Known direct callers are
`create_enumerated_saved_game_file`,
`saved_game_files_enumerate_available_to_local_player_index`, and the 16-byte
tail-jump `enumerate_memory_units_test`; none passes arguments or consumes a
result.

The candidate uses the XDK's actual `XGAME_FIND_DATA`, `HANDLE`,
`XCALCSIG_SIGNATURE`, `XFindFirstSaveGame`, `XFindNextSaveGame`, and
`XFindClose` declarations already reached through `<xtl.h>`. It adds the
owning `memory/crc.h` include for `crc_new` and `crc_checksum_buffer`; it does
not add caller-local prototypes. No playlist-profile structure, field, or
reserved Fable-owned header is required. The function only calls the existing
no-argument default-playlist enumerator.

January's 0xA84 frame independently fixes the genuine local layout:

- `wchar_t message[256]` at `-0xA84`;
- `XGAME_FIND_DATA` (0x344 bytes) at `-0x884`;
- `byte block[512]` at `-0x540`;
- `enumerated_saved_game_file` (0x206 bytes) at `-0x340`;
- `file_reference` (0x10C bytes) at `-0x138`;
- `XCALCSIG_SIGNATURE` (20 bytes) at `-0x2C`;
- `char root_path[8]` at `-0x18`;
- find handle, legacy CRC, memory-unit index, and enumerated count in the final
  four dword slots.

The candidate reproduces that frame exactly without opaque padding or a fake
aggregate.

## Reconstructed behavior

The ordinary typed implementation:

1. Takes `saved_game_files_globals.general_mutex`, begins the memory-unit
   mapfile, and enumerates only the hard-drive unit.
2. Uses the real eight-byte memory-unit root and `XFindFirstSaveGame`.
3. For each entry, zero-initializes a genuine
   `enumerated_saved_game_file`, recognizes `blam.sav` before `blam.lst`, and
   assigns the semantic player-profile/game-variant enum and authenticated
   checksum spans (48 and 104 bytes).
4. Preserves January's unusual unknown-entry diagnostic exactly, including
   `find_data.szSaveGameName`, `find_data.wfd.cFileName`, and the deliberate
   in-place `wide_to_ascii(message, (char *)message, sizeof(message))` call.
5. Copies and terminates the 128-wide-character display name, opens the save
   read/write with project `FLAG` macros, reads the 512-byte block, and verifies
   the 20-byte XCALCSIG signature.
6. On signature failure, checks the legacy four-byte CRC at the same payload
   boundary. A legacy match writes the new signature into the block, seeks to
   zero, rewrites all 512 bytes, and marks the entry valid only after success.
7. Closes every successfully opened file, appends recognized entries, advances
   with the target-authenticated byte-result interpretation of
   `XFindNextSaveGame`, and closes the XDK search handle.
8. Calls both already-owned default enumerators using the genuine three-`short`
   accumulation shape, ends the mapfile, releases the mutex, and unconditionally
   clears `memory_units_dirty`.
9. Ends with an explicit `return;`.

This also preserves January's less-obvious failure semantics: no diagnostic is
emitted when mutex acquisition or mapfile begin fails; unknown directories are
not appended or counted; close is attempted after any successful open even if
read or validation fails; and the dirty flag is cleared regardless of earlier
failures.

## Bounded compiler residual

The candidate's meaningful tail is 14 bytes later than January's (candidate
`ret` at `+0x3E3`, target at `+0x3D5`), yielding the 1008-versus-992 padded
envelope. The first divergence is immediately after the initial
`number_of_enumerated_files < 100` guard: January enters the zero-initializer
directly, while current VC7 emits a five-byte jump/alignment window and rotates
the repeated count guard to the bottom. At the outer one-element memory-unit
loop, January reuses `inc` flags for a short `je`; current VC7 emits an explicit
`cmp ...,1`/`jb` pair. The remaining body has the same call/data destination
sequence and the same 0xA84 local frame.

One bounded, source-backed top-guard/infinite-loop spelling and one natural
`for` spelling for the outer loop compiled to the exact same 1008-byte hash as
the retained straightforward `while` source. They were removed from the final
manifest because they add source structure without improving output. An early
explicit `csmemset` variant introduced two non-January call relocations and was
also rejected. No declaration-order search, register or volatile forcing,
pragma, assembly, fake dependency, raw offset, forced inline, undefined
behavior, or inert control flow was retained. The fail-closed classifier reports
`UNKNOWN`; no compiler mechanism is asserted.

Recommendation: integrate the natural body and `memory/crc.h` owner include,
then add an unclassified fuzzy park using the freshly regenerated production
artifact's authoritative objdiff percentage. Reopen only for authoritative
January source/local records or a natural same-compiler context that explains
both loop layouts. Do not chase the residual with source-shape lottery.
