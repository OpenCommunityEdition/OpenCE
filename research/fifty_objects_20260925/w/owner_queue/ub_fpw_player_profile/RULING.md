# RULING: two authentic-January-defect items (base cdc8ebd3)

There are two independent questions. Each is apply-ready on its own; see patches/APPLY_ORDER.txt and MEASURED.md.
Class touched by both: **reproducing an authentic January defect** (WORKER_BRIEF s2/s7: target and layout proof,
a BUG comment, owner approval). House rule 21 ("reject nonsensical logic even when bytes happen to match") and the
2026-09-07 note "closeness never justifies undefined/nonsensical logic" are the rules the owner is being asked to
weigh against the first-party evidence.

---------------------------------------------------------------------------------------------------------------

## Item 1: first_person_weapons patch B

**Question (yes/no).** May `_first_person_weapon_update` read `first_person_weapon_animations->animations.count`
without first testing the pointer? The pointer comes from `count ? TAG_BLOCK_GET_ELEMENT(...,0,...) : NULL`, and
the read happens at two sites (moving animation, overcharged jitter). The patch also drops the three
`REAL_MATH_EXTERNAL_{SQUARE_ROOT,MAGNITUDE_SQUARED3D,MAGNITUDE3D}` defines, so that magnitude3d inlines as it
does in January. That makes this TU emit three header `__inline` COMDATs from a now strictly exact caller.
If yes, the object is COMPLETE (34/34, data 536/536), and the last fpw park is retired.

### Evidence for

- **January bytes** (split `_first_person_weapon_update`). At 0x23e the code runs `xor eax,eax; jmp 0x24f`,
  then 0x255 `cmp dword [eax+0x10],3`. At 0x28d it runs `xor eax,eax; jmp 0x29e`, then 0x2a5
  `cmp dword [eax+0x10],0xf`. There is no test of eax between the NULL arm and the read.
- **Later first-party /Od build.** 0x61fcde `mov eax,1; test eax,eax` is the constant-true half of the index
  check. Then 0x61fcea `cmp dword [ecx+0x10],3` runs with no NULL test; 0x61fd8d..0x61fd99 does the same with
  `cmp [eax+0x10],0xf`.
- **HCEA 2011 decompiled source** (research/halocea/src/first_person_weapon_update.c, jitter site ~lines 138-145,
  re-read this wave): `animation_set = count ? address : 0; frame_index = 0; if (animation_set->animations.count <= K)`
  is the same unguarded read. This is a decompiler rendering, so it attests semantics, not source text.
- **Worker n1.** B plus both guards restored differs from January by exactly 4 inserted instructions
  (`test eax,eax; je` twice). Everything else is identical. No defined-behaviour spelling reaches the bytes.
- **The path is latent, not live.** `first_person_weapon_switch_weapons` commits `weapon_index` only inside
  `if (animation_graph->first_person_weapon_animations.count)` (tree line 1439ff), and update reads the same
  weapon's graph. This was verified in source in this wave.
- **COMDAT conditions** (the rule-6 exception, 2026-09-20). All three copies are byte-identical to January's
  selected copies (action_charge, action_alert). The caller is strictly exact. The provider link passes in both
  orders. There are no NODUP definers. Only this TU changes.
- **Precedent.** 18 source files already carry `BUG (preserved for exact matching)` or `BUG (original)` comments,
  including indeterminate or uninitialised reads: ai_communication.c:2527 (7a1d0d91), data_packets.c:220,
  sound_manager.c:2004 and data_encoding.c:128/421.

### Evidence against

- The source reads through a pointer that its own previous statement can set to NULL. Rule 21 and "undefined
  logic" apply literally, even though the invariant makes the path unreachable.
- The defined-behaviour body, with both guards and magnitude3d out of line, is already in the tree at zero credit
  (park 95.21506). Refusing costs one function, 1,533 bytes.

### BUG comments exactly as they would land (source/interface/first_person_weapons.c)

```c
			/* BUG (preserved for exact matching): January (and the later first-party
			 * build at /Od 0x61fcea) reads the animations block through this pointer
			 * without testing the NULL an empty first_person_weapon_animations block
			 * produces.  switch_weapons only commits a weapon whose graph has the block,
			 * so the path is unreachable in practice; a corrected build should test
			 * first_person_weapon_animations first. */
```
```c
				/* BUG (preserved for exact matching): the same unguarded read as the
				 * moving animation above (January and /Od 0x61fd96). */
```

### House-rule classes touched

- Authentic-defect reproduction: the NULL dereference.
- The rule-6 header-inline COMDAT exception, which is satisfied.

The patch adds no view casts, no new names and no header edits.

---------------------------------------------------------------------------------------------------------------

## Item 2: player_profile R1-R3

**Question (yes/no; all three are needed for the object).** Does the owner reverse the 2026-09-09 rejection
(docs/object_matching_logs/saved_game_family_reconciliation_20260909.md) and admit these three January failure-path
defects, each with a BUG comment?

- **R1.** The 512-byte `union player_profile_block block;` in `player_profile_write_thread_proc` and
  `player_profile_create_default_profiles_on_disk` has no initialiser. Only the 0x30-byte profile and the 0x14-byte
  checksum are filled, so 444 bytes of stack are written to the file.
- **R2.** create_default folds `file_set_position` into `file_create && file_open && file_set_position`, so a failed
  seek skips `file_close` and leaks the open file.
- **R3.** On a failed write, `player_profile_new` deletes the enumerated file while it is still open, sets the index
  to NONE, and then calls the shared `saved_game_file_close(&profile_file, NONE)`, whose index assertions fail.

If yes, the object is COMPLETE (22/22, data 1644/1644), and all three player_profile parks are retired.

### What is new since the 2026-09-09 rejection

The rejection rested on the house rules and the parked evidence. It had no first-party source attestation. There
now is some, for all three shapes:

- **R1.** The later /Od build's write routine (0x8596d0, RTC `profile_file`/`block`) and create_default (0x85acf0,
  RTC `file`/`full_path`/`block`) both declare `block` with no initialiser. January's create_default does no fill
  before `file_write(..., 0x200, [ebp-0x40c])`. In `player_profile_new`, by contrast, January's 1-byte head store is
  the attested `= {0}`, and the candidate keeps it. HCEX.pdb types the local as
  `union player_profile_block { unsigned char data[0x200]; struct player_profile_internal profile_block; }`.
- **R2.** /Od 0x85ae02 calls `file_set_position` inside the open condition; a failure jumps to 0x85ae5e and skips
  the close. In January, 0xa3 `je 0xec` skips 0xc6 `file_close`.
- **R3.** /Od 0x85d1f2 deletes, 0x85d1fa sets `index = 0xffffffff`, and 0x85d20c calls the close shared by both arms,
  which pushes that index. January does the same: 0x151 delete, 0x159 `or esi,-1`, 0x164 `close(&file, esi)`.
- **January-byte attestation.** No defined spelling reproduces these bytes. Earlier waves measured the
  alternatives: static buffer / csmemset (27 -> 28 relocs), the union block, both branch polarities, and the
  two-variable result. The landed packet A already carries the /Od `if (failed)` placement that solved the
  +0x10b byte.
- **Precedent.** The same authentic-bug class sits in the source with BUG comments (see item 1). The ai_communication
  uninitialised `cause_point` read landed in 7a1d0d91 on the same day as the saved-game rejection. I did not check
  whether an explicit owner ruling covers it; the owner should say whether it is a precedent or an oversight.

### Evidence against

- **R1 is information disclosure.** 444 bytes of stack go into a file on disk. The /Od attestation covers only the
  missing initialiser. In the later format, 0x1ffc profile bytes plus a 4-byte checksum fill the whole block, so the
  later build does not disclose. The January disclosure is a consequence of the 0x200 format size. Its declaration
  is authentic, but the harm is real.
- **R3 reaches a failing assertion.** It passes NONE to an asserted close API and deletes a file that is still open.
- **R2 leaks a handle** when a seek fails.
- The owner has already rejected exactly these three shapes once (2026-09-09). The parked evidence records that
  the owner required initialised padding and a close after every successful open.

### BUG comments exactly as they would land (source/saved games/player_profile.c)

R3 (player_profile_new; amended this wave per reviewer 3 to state the delete-while-open):
```c
			/* BUG (preserved for exact matching): after a failed write the file is deleted while it
			   is still open, and the index is already NONE, so saved_game_file_close() fails its
			   index assertions */
			saved_game_file_close(&profile_file, player_profile_index);
```
R1 (player_profile_write_thread_proc and player_profile_create_default_profiles_on_disk; identical text):
```c
	/* BUG (preserved for exact matching): only the profile and checksum are initialised, so the
	   rest of the 512-byte block is written from uninitialised stack storage */
	union player_profile_block block;
```
R2 (create_default):
```c
			/* BUG (preserved for exact matching): a failed seek skips file_close() and leaves the
			   file open */
			if (file_create(&file) &&
				file_open(&file, FLAG(_permission_write_bit)) &&
				file_set_position(&file, 0))
```

### Other content of patch 04 (not policy; disclosed)

- The HCEX `union player_profile_block` / `struct player_profile_internal` replaces the padded
  `struct player_profile_file_block`, and its `byte padding[]` member is removed.
- /Od RTC local names are used: `profile_file` in new and in the write thread, and `full_path` in create_default.
- `boolean succeeded` is removed from new.

### Optional (reviewer-3 advisory)

HCEX type and member names are available: `struct _player_profile_globals`,
`struct player_profile_write_internal_input { long index; ... }` and member `thread_input`.

- Use patches/optional/04a + 04b instead of 04. The result is byte-inert: 60/60 sections are identical.
- 04a alone is also landable at zero credit whatever the ruling.
- Still not addressed, and non-blocking: player_profile_read and player_profile_get_from_path keep
  `byte block[512]`, although HCEX types read's local as the union.

### House-rule classes touched

- Authentic-defect reproduction: R1 uninitialised serialised storage, R2 handle leak, R3 asserted-API misuse and
  delete-while-open.
- This is a reversal of an earlier owner rejection.

The patch adds no view casts, no header edits, no new surplus, no COMDATs and no storage changes.
