# `main.obj` — opus5 next-150K lane, wave n4 (2026-09-15)

Worker `n4:player_profile_main_etc`. Unit `source/main/main.c`. Baseline `ae10935da`.
**Real file UNCHANGED** (`git diff --stat -- source/main/main.c` empty). Per-function report blocks:
`scratch/workers/n4_player_profile_main_etc.md`. Diagnostic candidate:
`scratch/workers/n4_player_profile_main_etc/gr_d1.c` (+ `.obj`).

## Baseline and final

`gate.py source/main/main --all --forbid-emitted-symbol _point_from_line3d`:
**baseline == final == `exact 88  residual 6  unwritten 1`**. Point guard passed.
`scratch/parkcheck.py`: 5 parks, drift 0 (`_main_game_render` 464/29, `_main_get_window_count` 64/5,
`_compute_subframe_counts` 112/4, `_main_frame_rate_debug` 528/51, `_main_update_time` 1440/118).
`tools/fake_match_scan.py source/main/main.c`: 0 leads, unchanged from HEAD.
`branch_sweep.py source/main/main`: 54 unique blobs, current (88/6/1) is the best; best historical is 83/11/1.
Nothing landed; no exact byte is claimed.

Duplicate prevention: read `main_obj_opus5_250k_w3d_20260915`, `main_obj_opus5_next150_n3a_20260915`,
`main_obj_opus5_150k_w1_20260914`, `main_obj_opus5_100k_waveD_20260914`,
`main_obj_natural_state_and_render_packet_20260909`, `main_obj_lifecycle_and_fuzzy_reconstruction_20260909`,
`main_obj_jonas_crash_policy_closeout_20260826`, `main_obj_jonas_window_count_rejection_20260828`, plus
`scratch/workers/w3d_main.md` and `scratch/workers/n3_main.md`.

## Assigned targets

| Function | Padded | Orchestrator leverage | Verdict this wave |
| --- | ---: | --- | --- |
| `_main_game_render` | 448 | ours owns `_observer_get_camera` ×1 that January does not | leverage is a KNOWN artifact; NOT LANDED |
| `_main_crash` | 16 | body absent from our object | OWNER-BLOCKED (UB), unchanged since 2026-08-26 |

## `_main_game_render` — the extra relocation is tail duplication, not an extra statement

The orchestrator's relocation-by-target census (`_observer_get_camera` ours 2 / January 1) is **not a missing
or extra source statement**. Our body calls `observer_get_camera` once; VC7 tail-duplicates the shared
observer tail into the film-playback arm, which emits a second call. A full `alndiff --include-equal`
(target 143 instructions, ours 149) shows exactly three physical differences and nothing else:

1. ours inserts a 5-instruction copy of the tail at `0xdc`
   (`mov eax,[ebp-0xc]; push eax; mov word [esi],ax; call _observer_get_camera; add esp,4`) — this is the
   extra relocation and the whole +16-byte size gap;
2. ours emits `movsx ecx,ax` after `_local_player_get_next` where January stores `eax` straight to the dword
   slot;
3. January has a sixth tail instruction, `mov dword ptr [ebp-0x14],eax` (the `observer` home store), that ours
   lacks because ours register-forwards EAX across the join.

### (2) is already solved, and the declaration hypothesis is refuted

I initially read (2) as a January cross-TU declaration mismatch — main.c seeing a 32-bit-returning
`local_player_get_next` — the same class as n3a's `_main_get_window_count` finding. It is not, and I record the
refutation so the next wave does not re-derive it:

- `_local_player_get_next` is **EXACT** in `players.obj` (64 bytes / 1 relocation) with a `short` return and
  16-bit compares (`cmp cx,si`, `cmp cx,ax`, `cmp ax,0xffff`).
- `_director_update` is **EXACT** in `director.obj` and emits
  `push -1; call _local_player_get_next; add esp,4; cmp si,ax` for
  `local_player_index == local_player_get_next(NONE)` — a 16-bit compare, which proves every TU sees the
  `short` declaration in `players.h`.
- w3d had already given the correct explanation: `last_local_player_index` is a **`short` local whose upper
  half is never read**, which VC7 gives a full dword cell with dword stores and no `movsx`
  (`lab/l1.c`, `lab/l2.c`; Stian `main.c:4224 __int16 next_player`, HCEA `int16_t last_assigned_player`
  agree). `gr_v4.c` closes that hunk.
- Diagnostic probe run this wave (scratch only, never landable):
  `scratch/workers/n4_player_profile_main_etc/gr_d1.c` replaces the call with
  `((long (*)(short))local_player_get_next)((short)last_local_player_index)`. VC7 folds it to a direct call and
  the `movsx` disappears — **and the tail duplication survives**, while two *new* register differences appear
  at `0x136` and `0x174`. So (2) and (3) are independent, and (2) is not the lever.

### (3) is the whole residual and has no admissible source lever

w3d measured the threshold directly (`lab/l3.c`–`l5.c`): a 5-instruction / 17-byte join block is duplicated
into a fall-through predecessor; a 6-instruction / 20-byte one is not. January's sixth instruction is the
`observer` home store, i.e. January simply does not register-forward `observer` across the join. Already
refuted as causes: observer live-out of the loop (January pushes literal `0` for the console window and for
`render_frame`), loop-scope declaration, arm polarity, NULL assigned in the arms, `console_window` placement
before `set_window_camera_values`, and the HCEA flat else-if chain (480/30, worse). n3a additionally re-read
HCEA `blam/main/main_game_render.c` and Stian `main/main.c:4224` and confirmed neither reads the camera after
the loop, so any construct that keeps `observer` alive across the join would be a synthetic lifetime anchor
(prohibited).

**No park reopen is proposed.** The park's recorded premise is already accurate after w3d; this wave only
adds the two refutations above.

## `_main_crash`

Unchanged and owner-blocked. The 16 bytes are `mov dword ptr [0], <address of the literal>; ret` with one
`IMAGE_REL_I386_DIR32` at byte 6 to `"chucky was here!  NULL belongs to me!!!!!"` — an intentional
null-pointer write. `main_obj_jonas_crash_policy_closeout_20260826.md` forbids revisiting it through a null
cast, raw address, pointer/integer reconstruction, `volatile`, assembly, pragma, intrinsic or object-byte
patch, and brief section 7 prohibits reproducing the UB. Historical blob `a7ec701e` corroborates the reading
(a write through a null `char **`) but supplies no defined mechanism. No new provenance appeared.

## Not spent (recorded exhausted, and forbidden by this wave's owner directive)

`_main_load_last_solo_map` (192, the only non-parked residual) is the recorded ESI/EDI callee-saved
permutation, exhausted in waveD and w1. `_compute_subframe_counts` (112), `_main_frame_rate_debug` (544),
`_main_get_window_count` (64) and `_main_update_time` (1440) are all parked with n3a/w3d shape budgets spent
and explicit reopen criteria; none of them is a `[size]`/`[relocs]` row with a newly identified missing call,
wrong ABI/type or frame-slot defect, so all five fall under the "generic sha-only / register-allocation work
is forbidden" directive.

## Proposals

1. **No park reopen**, no header change, no config change from this unit.
2. Still outstanding from w3d (not re-litigated here): the zero-credit park re-baselines of
   `_main_update_time` to `scratch/workers/w3d_main/ut_v10.c` (1440/117) and `_main_frame_rate_debug` to
   `scratch/workers/w3d_main/frd_v2.c` (544/51).

## Reopen criteria

- `_main_game_render`: a natural source form that adds a sixth instruction to the shared observer tail — i.e.
  makes `observer` memory-resident across the join without a synthetic lifetime anchor. The width hunk
  (`gr_v4.c`) and the duplication threshold are both already measured; do **not** re-derive the
  `local_player_get_next` declaration hypothesis, which `players.obj` and `director.obj` refute.
- `_main_crash`: an explicit house-rule change, or new provenance supplying a defined, non-null fault
  mechanism that also reproduces the 11 meaningful bytes and the single relocation.
