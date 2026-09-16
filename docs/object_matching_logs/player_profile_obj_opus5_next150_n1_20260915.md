# `player_profile.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Baseline and final

Guarded gate at HEAD and after the wave: **18 exact / 4 residual / 0 unwritten**,
`_point_from_line3d` guard passes, park drift 0 on the real object. The real file
`source/saved games/player_profile.c` is **unchanged** (`git diff --stat` empty).
Nothing landed; no exact byte is claimed.
`_player_profile_get_argb_color` was out of scope (do-not-repeat) and was not touched.

Candidates live in `scratch/workers/n1_player_profile_game_state_xbox_etc/`
(`pp_v1.c` … `pp_v5.c`, with matching `.obj`).

## A new January type fact for the profile file block

January's `= {0}` on the 512-byte profile file block in `_player_profile_new` emits

```text
45  mov byte ptr [ebp-0x204], bl
52  lea edi, [ebp-0x203]      58 rep stosd (0x7f)      5c stosw      66 stosb
```

while our typed `struct player_profile_file_block` emits
`mov word …; lea edi,[ebp-0x202]; rep stosd; stosw`. The explicit head store's width is
the width of the object's first scalar, so January's block **leads with a 1-byte member**.
It cannot be our `struct player_profile`, whose first member is
`wchar_t player_name[MAXIMUM_PLAYER_PROFILE_NAME_LENGTH]` — independently proven in the
same function by the 2-byte store of `player_name[MAXIMUM_PLAYER_PROFILE_NAME_LENGTH-1]`
at `[ebp-0x1ee]` (offset 22) and by `push 0xb` into `_ustrncpy`.

The house-clean TU-local spelling that reproduces January byte-for-byte is a union whose
first member is the raw block (`pp_v4.c` / `pp_v5.c`, no cast, no raw offset, the
`sizeof == SAVED_GAME_FILE_BLOCK_SIZE` verifier retained):

```c
struct player_profile_file_contents
{
	struct player_profile profile;
	XCALCSIG_SIGNATURE checksum;
};

union player_profile_file_block
{
	byte data[SAVED_GAME_FILE_BLOCK_SIZE];
	struct player_profile_file_contents contents;
};
```

Measured effect (whole-object gate, all 18 EXACT rows preserved, fakescan clean):

| row | HEAD | with the union | target |
| --- | ---: | ---: | ---: |
| `_player_profile_create_default_profiles_on_disk` | 336 | **320** | 288 |
| `_player_profile_new` zero-fill hunk | 4 differing insns | **identical to January** | — |
| `_player_profile_write_thread_proc@4` | 384 | 384 | 352 |

Because all three bodies are parked, this change drifts three parks and needs an
orchestrator re-baseline; it is strictly closer on two of them and inert on the third.

## `_player_profile_new` — NOT LANDED, but the structural relocation gap is closed

Target 464 padded / 456 meaningful / **19** relocations / sha `ce58c5a489fbc7f5`;
HEAD 464 / **18** / sha `4df7452aff75a49e` (`[relocs 18!=19, sha]`).

### Census

* **Relocation census by target:** the single missing relocation is a **second**
  `_saved_game_file_close`. January's order is `_file_write` @0x136,
  `"failed to initialize…"` @0x142, `_error` @0x149,
  `_delete_enumerated_saved_game_file` @0x152, `_saved_game_file_close` @0x165 (failure
  path), `_saved_game_file_close` @0x181 (success path), `"failed to open…"` @0x192, …
  Ours has one close at 0x154, emitted before the failure block. Every other relocation
  matches 1:1.
* **Frame census:** target `sub esp,0x310`, ours `sub esp,0x314`. January homes only
  `player_profile_index` at `[ebp-4]` and puts `block` at `[ebp-0x204]`; ours shifts
  `block` to `[ebp-0x208]` because `boolean succeeded` needs a cell. January never
  materialises `succeeded` at all (`test al,al; je 0x141`, then `test al,al; jne 0x175`)
  because its close does not sit between the assignment and the test.

### Shapes measured (5)

1. **`pp_v1`** — `if (file_set_position(…) && file_write(…)) { close; } else { error;
   delete; close; player_profile_index = NONE; }`, `succeeded` deleted. Result
   **464 padded / 19 relocations / frame 0x310**: both structural gaps closed, the row
   drops to `[sha]`, and the body stays fully defined (the close always receives the
   valid opened index).
2. **`pp_v2`** — the same with `if (!a || !b) {…} else {…}`: byte-identical to `pp_v1`
   (measured inert control; VC7 canonicalises the polarity).
3. **`pp_v3`** — diagnostic `byte block[SAVED_GAME_FILE_BLOCK_SIZE] = {0};` plus a typed
   pointer: the whole 0x45-0x66 zero-fill hunk becomes byte-identical to January.
4. **`pp_v4`** — the house-clean union spelling of (3) combined with `pp_v1`:
   464 / 19 / sha `1123ab1c4d4af348`, zero fill identical, no cast, no raw offset.
5. **`pp_v5`** — the union alone, to isolate its effect on the two sibling parks.

### Remaining blocker

January's failure path is

```text
148  call _error                       ; "failed to initialize newly created player profile"
14d  mov edx, dword ptr [ebp-4]        ; the still-valid index
151  call _delete_enumerated_saved_game_file
159  or esi, 0xffffffff                ; player_profile_index = NONE
162  push esi                          ; ... passed to the close
164  call _saved_game_file_close
```

so January calls `saved_game_file_close(&file, NONE)`, which trips four `match_assert`s
(`saved_game_files.c` 603/606/607/608/609) and `system_exit`s. That ordering is the only
remaining difference class. It is a defined fatal-assert bug rather than UB, but it is
precisely the construct rejected by `saved_game_family_reconciliation_20260909.md`; under
laws_w3 R13 that rejection binds this wave, so **no reopen is proposed**.

### Reopen criterion (refined)

Reopen on an owner ruling admitting January's `saved_game_file_close(&file, NONE)`
ordering under the original-bug policy (with the
`/* BUG (preserved for exact matching): */` comment form). The union declaration and the
two-close structure are already measured and defect-free, so the ruling alone closes the
row; `pp_v4.c` is the prepared candidate.

## `_player_profile_write_thread_proc@4` — SKIPPED-EXHAUSTED (owner-blocked, and not exact even with the bug)

Target 352 / 349 / 27 / sha `9a173aa9929df8a1`; ours 384 / 27 / sha `839937d5e05bba88`.

* **Frame:** identical (`sub esp,0x30c`, `block` `[ebp-0x30c]`, `file` `[ebp-0x10c]`).
* **Relocations:** 27/27, identical.
* **Instructions:** target 116, ours 128. The twelve extra are exactly the `= {0}` fill
  plus the displaced `push edi` / `mov edi,[esi]` pair.
* January writes the whole block (`push 0x200` at 0xb4 before `_file_write`) from storage
  whose 444 padding bytes were never initialised.

Removing the initializer is the repair the batch owner explicitly forbade, is an
uninitialised read and an information disclosure under brief section 7, and — per
`player_profile_obj_opus5_150k_w2_20260914.md` and
`saved_game_family_reconciliation_20260909.md` — still leaves one differing code byte at
`+0x10b`. Defined ways to drop the fill were enumerated and refuted: a `static` buffer or
an explicit `csmemset` each add a relocation (27 → 28); a shorter `file_write` changes the
`push 0x200` immediate and the on-disk format.

**Reopen criterion:** needs BOTH an owner ruling on serializing uninitialised padding AND
a separate explanation for the `+0x10b` byte. Rank it below
`_player_profile_create_default_profiles_on_disk`, which the same ruling closes outright.

## `_player_profile_create_default_profiles_on_disk` — SKIPPED-EXHAUSTED (owner-blocked)

Target 288 / 275 / 14 / sha `0f24b860ac0a7fc7`; ours 336 / 14 / sha `3ce0cc6f2de1f5b8`.

* **Frame:** target `sub esp,0x40c`, ours `sub esp,0x410`. Target slots `block`
  `[ebp-0x40c]`, `path[256]` `[ebp-0x20c]`, `file` `[ebp-0x10c]`. The extra cell is a
  consequence of the `= {0}` fill: it claims EDI for `rep stosd`, so the loop counter `i`
  moves from EDI (January, `xor edi,edi` at 0xc) to EBX.
* **Relocations:** 14/14, identical.
* **Instructions:** target 101, ours 114; the thirteen extra are the zero fill.

The donor form (uninitialised block, and `file_set_position` folded into the
`file_create && file_open` condition so an opened file is never closed on seek failure)
is strict exact at 275/288 and was rejected on both counts. The union declaration above
takes this row from 336 to **320** padded with the padding still fully initialised.

**Reopen criterion:** unchanged — an owner ruling on serializing uninitialised padding.
The probe shows that ruling alone closes the row.
