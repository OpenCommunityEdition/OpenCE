# RULING - authentic January uninitialised reads: dead_camera and saved_game_files

Two independent yes/no questions. Both functions fall under the original-bug policy in
docs/matching_methodology.md, which requires target proof, layout proof, a BUG comment and owner
approval. Both are listed in claude_structural_owner_packet_20260923.md, section 4.

In both cases the later first-party /Od build contains the FIX (an initialiser), so there is no
`_RTC_UninitUse` attestation. The evidence is January's own bytes plus the fact that the /Od build
added the initialiser later. This puts both functions in the non-RTC subclass of section 4, not the
RTC-attested first class.

---

## Q1. dead_camera `_dead_camera_update` (1,248 padded / 1,235 meaningful; completes the object)

**Question:** May `source/camera/dead_camera.c` declare `long next_unit_index;` with no initialiser
and no else arm? The effect is that when `player_get_next_player_with_a_unit` returns NONE, the test
reads the indeterminate local. This also deletes the current defined stand-in
`else { next_unit_index = (long)result; }` and its comment. Answer YES or NO.

### Code change exactly as it would land (patches/01_dead_camera_update_bug.patch)

```c
	if (0.f == camera->switch_timer && !game_time_get_paused())
	{
		long next_player_index;
		/* BUG (preserved for exact matching): January never initializes
		 * next_unit_index, so when no next player is found the test below
		 * reads an indeterminate value (January's +0x176 loads [ebp+0x10],
		 * the `result` parameter's home, which still holds the command pointer).
		 * A corrected build should initialize next_unit_index to NONE, as
		 * the later /Od build does.
		 */
		long next_unit_index;
		boolean match_team;
		...
		if (next_player_index != NONE)
		{
			next_unit_index = player_get(next_player_index)->unit_index;
		}

		if (next_unit_index != camera->unit_index && next_unit_index != NONE)
```

- The else arm `next_unit_index = (long)result;` and its three-line comment are deleted.
- The code is identical to the held wave-1 packet (`owner_update_bug.patch`). Only the comment text
  changed: it now uses the methodology's BUG template, and the "owner ruling required" wording, which
  would be stale after a YES, is gone.
- The comment is byte-inert: both comment versions produce identical objects on all 26 sections and
  every symbol.

### Evidence FOR

1. **January's bytes.** The else arm of the `next_unit_index` phi is `+0x176 mov eax,[ebp+0x10]`.
   January only ever READS `[ebp+0x10]` before that point (at +0x20 and +0x3b, into ESI). So January
   deterministically compares and stores the caller's `result` pointer as a unit datum index. The
   production stand-in gets the same runtime value through ESI (`mov eax,esi`), which is the one
   instruction that keeps the function residual.
2. **No defined initialiser can close it (n1 20260915).**
   - `long next_unit_index = (long)result;` at the top of the block produces January's load
     byte-for-byte, plus one extra store.
   - Declared at function scope, it is worse (4 regions).
   - VC7 never elides the initialising store.
3. **The /Od build (0x522c90) shows the fix.** It has `mov [ebp-8],0xffffffff` (`= NONE`) at the top
   of the block and no else arm. So the later source fixed the bug with an initialiser, which means
   January's form is the bare declaration. The HCEA 2011 donor also initialises the value to -1.
4. **The compiler flags the path.** At /W4 the candidate raises `C4701 'next_unit_index' may be used
   without having been initialized`. The production build uses the default warning level, so it
   stays silent.
5. **Precedent.** `source/ai/ai_communication.c:2527` (7a1d0d91, 2026-09-09) preserves a January bug
   that leaves `cause_point` uninitialised and then passes `&cause_point` on. That is an admitted
   indeterminate read with the same BUG template.
6. **The whole-object audit is clean.** See MEASURED.md section 1: 4/4, object_audit PASS, PDB
   storage 0, surplus identical, provider_link PASS in both orders (19/19), data 100%, 0 fake leads.

### Evidence AGAINST

1. **Earlier rulings rejected this exact read.**
   - dead_camera_obj_codex_checkpoint.md: "Recreating the uninitialized read is forbidden".
   - legacy_residual_donor_truth_audit_20260913.md rejected the 1,235-byte donor for reading "an
     uninitialized argument home slot".
   - opus5_150k_w1 skipped it as undefined behaviour.

   Those decisions were taken under briefs that forbade undefined behaviour outright. None of them
   was an owner ruling under the original-bug policy.
2. **It is formally undefined C.** The January value comes from VC7 coalescing the local into the
   `result` home slot, so any other build would read garbage. The current stand-in is defined C with
   the same January runtime value; it simply costs one store.
3. **House rules 20 and 21 and the source-credibility gate** reject "undefined ... source". The
   original-bug policy is the only exception, and it is deliberately narrow.
4. **No `_RTC_UninitUse` attestation.** The /Od build contains the fix, not the bug.

### House-rule classes touched

- Original-bug policy (methodology) and the brief's owner-gated "authentic January uninitialised
  read" class.
- Rules 20 and 21 (fake or nonsensical logic): the strip test is inverted here. Adding the corrected
  `= NONE` breaks the match, because the MISSING initialiser is the January fact. No decoration,
  pragma, volatile, dummy local or header edit is involved.

**If YES:** dead_camera becomes COMPLETE (+1,235 meaningful).

**If NO:** keep production as it is. It is defined, but its stand-in is itself a disclosed January-value
imitation; the only change would be to rewrite its comment to the BUG template. The residual then stays
at zero credit, with no park.

---

## Q2. saved_game_files `_saved_game_files_enumerate_available_to_local_player_index` (368 padded / 359 meaningful)

**Question:** May the count `number_of_available_profiles` be declared without an initialiser and zeroed
only after `count_enumerated_profiles_in_mapfile(...)`, inside the successful saved-game-files-mutex
branch? On the mutex-failure path, `*number_of_profiles` then receives an indeterminate count (January
stores the low word of the caller's `player_index`). This reverses the 20260904/20260908 rejection that
is recorded in the config/parked.json park. Answer YES or NO.

### Code change exactly as it would land (patches/03_saved_game_files_enumerate_available_bug.patch, on top of 02)

```c
	struct enumerated_saved_game_file file;
	/* BUG (preserved for exact matching): January zeroes the count only after
	 * taking the saved game files mutex (xor edi,edi at +0x94), so the mutex
	 * failure path stores an indeterminate count (+0x156 loads the dead
	 * player_index home [ebp+8]). A corrected build should initialize
	 * number_of_available_profiles to zero here.
	 */
	long number_of_available_profiles;
	word memory_unit_index = _memory_unit_hard_drive;
	...
		number_of_entries = count_enumerated_profiles_in_mapfile(memory_unit_index);
		number_of_available_profiles = 0;
		if (take_mutex(saved_game_files_globals.mapfile_mutex, SAVED_GAME_FILES_MUTEX_TIMEOUT))
```

This is the held wave-1 comment, verbatim. It already follows the methodology template, and I
re-verified its offsets against January's bytes (+0x91, +0x94, +0x156).

### Evidence FOR

1. **January's bytes.**
   - +0x91 `mov [ebp+8],eax`: the entry count is spilled into `player_index`'s home.
   - +0x94 `xor edi,edi`: the zero comes after the count.
   - The failure tail at +0x14a..+0x160 does `mov dx,[ebp+8]; mov [eax],dx`. `[ebp+8]` is not written
     on that path, so January stores `(word)player_index`.
2. **/Od 0x8646c0 zeroes the accumulator twice.** Once at the declaration (0x8646e6, the later fix) and
   once right after the count (0x864769). January's zero sits at the second position, so the
   declaration initialiser is a later addition. Lane D (20260922) reached the same conclusion.
3. **Every defined alternative is already refuted from the bytes** (checkpoint 20260908, n1 20260915,
   Lane D 20260922): a declaration initialiser, a zero in both arms, the store inside the success
   branch, and a self-store. The only defined source that reaches +0x156 would reuse `player_index` as
   the counter. That contradicts /Od (which never writes the `player_index` slot) and is nonsensical,
   so it was never compiled.
4. **The compiler flags the path.** At /W4 the candidate raises `C4701 'number_of_available_profiles'
   may be used without having been initialized`.
5. **Precedent.** The same ai_communication.c:2527 precedent applies.
6. **Whole-object audit** (MEASURED.md section 2): 46/46, object_audit PASS against the emulated split,
   provider_link PASS in both orders (11/11), surplus 0, data 100%, header blast radius 0 across 6
   includers, 0 fake leads.

### Evidence AGAINST

1. **The park explicitly records a prior rejection.** config/parked.json says: "reproducing that
   undefined behavior is rejected even though it makes the section strict exact". The same rejection
   appears in claude_fable_opus_reconciliation_checkpoint_20260908.md ("undefined, nonsensical path is
   rejected under the house rules") and in saved_game_files_obj_opus_reconciliation_20260904.md.
2. **The indeterminate value leaves the function.** It is written through an out-parameter, so the
   caller can observe it. That makes it harder to defend than the dead_camera read, which only feeds a
   compare and store inside the same camera.
3. **Rule 17** (combine declaration and initialisation) is locally reversed. The /Od double zero attests
   the late position, but the declaration's own initialiser comes from the later build only.
4. **No `_RTC_UninitUse` attestation.** The /Od build contains the fix.

### Also in the packet: patch 02 (zero-credit storage; no ruling needed)

- `enumerate_memory_units` becomes static, with its prototype moved from saved_game_files.h into the .c
  private block.
- config/symbols.json line 7376 gets `"static": true`.
- Evidence: cachebeta publics do not list it, and it is referenced only inside this object.
- It is byte-neutral: one storage flip, and 0 moved rows or sections across 6 includers.
- It was held since wave 1 and never landed.

### Advisory: what is still needed for saved_game_files to be COMPLETE (a separate question, not the UB ruling)

Three functions still disagree with PDB storage: `_saved_game_files_take_mapfile_mutex`,
`_saved_game_files_release_mapfile_mutex` and `_saved_game_files_enumerate_default_files`.

- **The wave-1 "proven external" argument is refuted by law (h).** A static that the source uses is
  emitted even when every call is inlined; the wave-1 lab never used the helper.
- **/Od calls the mapfile-mutex helpers** from exactly the 6 functions where our source hand-expands
  them.
- **A LAB variant closes all three at no byte cost** (`lab/LAB_ONLY_...diff`):
  - the three helpers become static;
  - their calls replace the hand expansions;
  - their prototypes move into the .c private block.

  Result: 46/46, every section identical, object_audit PASS, **PDB storage 0**, link PASS, blast radius
  0.
- **Caveat:** `enumerate_default_files` has no /Od call-site attestation (2 candidate sites).

**Owner or integrator choice:** review and land that follow-up, or accept the three as external. Until
then, do not flip config.json:398 to Matching.

**If YES (and 02 lands):** the function is exact (+359 meaningful), and the object is 46/46 and
audit-exact, with completion as described above.

**If NO:** land 02 alone (zero credit). The park stays, and its evidence text is still accurate.
