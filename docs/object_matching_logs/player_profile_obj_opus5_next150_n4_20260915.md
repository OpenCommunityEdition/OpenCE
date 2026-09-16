# `player_profile.obj` — opus5 next-150K lane, wave n4 (2026-09-15)

Worker `n4:player_profile_main_etc`. Unit `source/saved games/player_profile.c`. Baseline `ae10935da`.
**Real file UNCHANGED** (`git diff --stat -- "source/saved games/player_profile.c"` empty). Per-function report
blocks: `scratch/workers/n4_player_profile_main_etc.md`.

## Baseline and final

`gate.py "source/saved games/player_profile" --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 18  residual 4  unwritten 0`**. Point guard passed.
`scratch/parkcheck.py`: 4 parks, drift 0 (`_player_profile_write_thread_proc@4` 384/27, `_player_profile_new`
464/18, `_player_profile_get_argb_color` 80/1, `_player_profile_create_default_profiles_on_disk` 336/14).
`tools/fake_match_scan.py`: 0 leads. `branch_sweep.py`: 22 unique blobs, current (18/4/0) is the best.
Nothing landed; no exact byte is claimed.

Duplicate prevention: read `player_profile_obj_opus5_next150_n1_20260915`,
`player_profile_obj_opus5_150k_w2_20260914`, `saved_game_family_reconciliation_20260909`,
`player_profile_obj_jonas_public_quintet_20260828`,
`player_profile_obj_jonas_highest_completed_solo_level_first_shot_20260830`,
`player_profile_obj_jonas_wrapper_pair_20260821`, `player_profile_s3tc_batch_reconciliation_20260907`.

## Assigned target

| Function | Padded | Orchestrator leverage | Verdict |
| --- | ---: | --- | --- |
| `_player_profile_new` | 464 | January owns `_saved_game_file_close` ×1 that ours does not | genuine, but it is the KNOWN owner-rejected construct; NOT LANDED |

## `_player_profile_new` — the missing relocation is one statement, tail-duplicated, and it is owner-rejected

Target 464 padded / 456 meaningful / **19** relocations / sha `ce58c5a489fbc7f5`; parked body 464 / **18** /
sha `4df7452aff75a49e` (`[relocs 18!=19, sha]`). `tinfo` confirms the only census delta is a **second**
`_saved_game_file_close` (January `@0165` and `@0181`); every other relocation matches 1:1.

The delta is real, but it is **not two source calls**. January's tail, read from the bytes:

```text
141  push "failed to initialize newly created player profile"; push 2; call _error
14d  mov edx, dword ptr [ebp-4]      ; player_profile_index, still valid
150  push edx ; call _delete_enumerated_saved_game_file ; add esp,0xc
159  or  esi, 0xffffffff             ; the RETURNED index becomes NONE
15c  lea eax, [ebp-0x310]            ; &file
162  push esi ; push eax ; call _saved_game_file_close ; add esp,8
16c  pop edi ; mov eax,esi ; ... ret
175  mov esi, dword ptr [ebp-4]      ; success arm: returned index is the valid one
178  lea eax, [ebp-0x310]
17e  push esi ; push eax ; call _saved_game_file_close ; add esp,8
188  pop edi ; mov eax,esi ; ... ret
```

Both arms are the same three statements — *set the result; close with the result; return the result* — so the
two closes are **one source call tail-duplicated into both arms** of the write test. The unique source that
emits it is the close moved after the failure block:

```c
if (!(file_set_position(&file, 0) && file_write(&file, sizeof(block), &block)))
{
	error(_error_silent, "failed to initialize newly created player profile");
	delete_enumerated_saved_game_file(player_profile_index);
	player_profile_index = NONE;
}
saved_game_file_close(&file, player_profile_index);
```

On seek/write failure that is `saved_game_file_close(&file, NONE)`, which decodes and asserts the type,
memory-unit and file-index fields of an invalid index (`saved_game_files.c` 603/606/607/608/609) and
`system_exit`s.

**That body has already been measured strict EXACT (456 meaningful / 464 padded) and REJECTED by the owner**,
`saved_game_family_reconciliation_20260909.md` section "New player profile": *"The existing parked evidence
explicitly requires closing with the valid opened index before deletion and before changing the returned index
to `NONE`."* Under laws_w3 **R13** that rejection binds this wave, so no reopen is proposed and no shapes were
spent.

The defect-free variants are already prepared and measured by wave n1:
`scratch/workers/n1_player_profile_game_state_xbox_etc/pp_v1.c` and `pp_v4.c` put the close inside both arms
with the failure arm ordered `error; delete; close; player_profile_index = NONE`, reaching **464 padded /
19 relocations / frame `0x310`** — both structural gaps closed, `[sha]` only, the sole remaining difference
being that January's `or esi,-1` precedes the close and ours follows it.

A two-variable spelling (`result = NONE;` while the close still receives the valid `player_profile_index`)
cannot match either: January pushes the **same register it returns** (`push esi` … `mov eax,esi` in both
arms), so the close argument is provably the NONE-valued result variable, not the saved index.

## Not spent (recorded exhausted / owner-blocked)

- `_player_profile_create_default_profiles_on_disk` (288) and `_player_profile_write_thread_proc@4` (352):
  both parked; both blocked on the same pending owner ruling about serializing uninitialised padding
  (`saved_game_family_reconciliation_20260909.md`). n1 additionally showed the TU-local union file-block
  declaration takes `create_default_profiles_on_disk` from 336 to **320** with the padding still fully
  initialised, and that `write_thread_proc` needs a *separate* explanation for the `+0x10b` byte even with the
  ruling.
- `_player_profile_get_argb_color` (80, `[sha]`): laws_w2 section D do-not-repeat, 3 shapes spent.

## Proposals

No park reopen, no header change, no config change from this unit.

## Reopen criterion (carried, refined)

An owner ruling admitting January's `saved_game_file_close(&file, NONE)` ordering under the original-bug
policy, with the `/* BUG (preserved for exact matching): */` comment form.
`scratch/workers/n1_player_profile_game_state_xbox_etc/pp_v4.c` is the prepared candidate; it also carries the
house-clean union file-block declaration, which drifts `_player_profile_create_default_profiles_on_disk`
strictly closer (336 -> 320) and is inert on `_player_profile_write_thread_proc@4`, so all three parks need
re-baselining in the same commit if the ruling is granted.
