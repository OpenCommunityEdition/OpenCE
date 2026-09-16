# `player_profile.obj` — opus5 next-150K lane, wave n5 (2026-09-16)

Worker `n5:player_profile`. Unit `source/saved games/player_profile.c`. Lane HEAD `3faefe7f6`.
Per-function report blocks: `scratch/workers/n5_player_profile.md`. Candidates:
`scratch/workers/n5_player_profile/`.

**Real file UNCHANGED** — `git diff --stat -- "source/saved games/player_profile.c"` is empty.
Nothing landed; no exact byte is claimed.

## Baseline and final

`python -B tools/campaign/gate.py "source/saved games/player_profile" --all --forbid-emitted-symbol _point_from_line3d`

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (HEAD) | 18 | 4 | 0 |
| final (HEAD, unchanged) | 18 | 4 | 0 |

Point-from-line3d guard passed at baseline and at the end. `scratch/parkcheck.py` on the real object:
4 parks, **drift 0** (`_player_profile_write_thread_proc@4` 384/27, `_player_profile_new` 464/18,
`_player_profile_get_argb_color` 80/1, `_player_profile_create_default_profiles_on_disk` 336/14).
`tools/fake_match_scan.py "source/saved games/player_profile.c"`: **0 review leads**.
`branch_sweep.py`: 12 unique blobs; the current tree (18/4/0) is the best; every historical blob is worse or
fails to compile. `git log --all -- "source/saved games/player_profile.c"`: 14 commits, newest `458ba910c`.

Duplicate prevention read in full: `player_profile_obj_opus5_next150_n4_20260915.md`,
`player_profile_obj_opus5_next150_n1_20260915.md`, `player_profile_obj_opus5_150k_w2_20260914.md`,
`player_profile_obj_jonas_public_quintet_20260828.md`,
`player_profile_obj_jonas_highest_completed_solo_level_first_shot_20260830.md`,
`player_profile_obj_jonas_wrapper_pair_20260821.md`,
`player_profile_s3tc_batch_reconciliation_20260907.md`,
`saved_game_family_reconciliation_20260909.md`, and the four `config/parked.json` records for this unit.

## Per-function outcomes

| Function | Padded | Outcome | Classification |
| --- | ---: | --- | --- |
| `_player_profile_new` | 464 | NOT LANDED | owner-blocked original bug; LAW Z route terminates at the same ruling |
| `_player_profile_get_argb_color` | 80 | NOT LANDED (4 shapes) | RECLASSIFIED: VC7 dead-store-elimination, 6 meaningful bytes — **not** an x87 schedule |
| `_player_profile_create_default_profiles_on_disk` | 288 | SKIPPED-EXHAUSTED | owner-blocked (uninitialised serialized padding) |
| `_player_profile_write_thread_proc@4` | 352 | SKIPPED-EXHAUSTED | owner-blocked, and still non-exact even with the ruling |

## `_player_profile_new` (assigned target) — LAW Z applied, surplus named, route blocked

### The LAW Z profile is confirmed, but it is a pin **use-count** delta, not a pin **presence** delta

`python -B scratch/hs_pin/webprofile.py "source/saved games/player_profile" _player_profile_new`, re-run this wave:

```
JANUARY  149 insns   ebx: 1 def @0x14, 13 uses
                     esi: 5 defs / 9 uses  (mov esi,[ebp+0xc] @0x0e; xor esi,esi @0xd3;
                                            inc esi @0xf3; or esi,0xffffffff @0x159; mov esi,[ebp-4] @0x175)
                     edi: 5 defs / 5 uses  (lea edi,[ebp-0x203] @0x52, rep stosd, stosw, stosb)
OURS     146 insns   ebx: 1 def @0x14, 15 uses
                     esi: 4 defs / 8 uses, no `or esi,-1`
                     edi: 4 defs / 5 uses, no `stosb`
```

Both sides pin. Z1 (availability) and Z3 (span) therefore do not bite; only Z2 (demand) applies, and it points
at a **use-count** surplus of exactly two.

### Z2 census — the surplus is one local, named before editing

The two surplus ebx uses, plus one surplus instruction, are:

```
O 13f  mov byte ptr [ebp-1], 1     ; succeeded = TRUE  (short-circuit join)
O 145  mov byte ptr [ebp-1], bl    ; succeeded = FALSE (ebx use 14)
O 15e  cmp al, bl                  ; if (!succeeded)   (ebx use 15)
```

That is the whole materialisation of `boolean succeeded` at `[ebp-1]`. The same local owns the frame gap:
`sub esp,0x314` vs January `sub esp,0x310`, which pushes `player_profile_index` from `[ebp-4]` to `[ebp-8]`.

There is **no other** surplus zero-valued statement. A full `alndiff --include-equal` shows the nine
default-profile field stores in the `0x76..0xc4` block are identical one-for-one once the 4-byte frame shift is
applied — both sides write field offsets 24 (`word 0xffff`), 42 (`byte 3`), 43, 45, 47, 44, 38 (`word`), 40,
41, 26 (`word`) and 22 (`word`) in that order. (Note for future readers: `alndiff` reports a few of these rows
as "equal" purely because the displacements coincide across the 4-byte frame shift; always re-derive the
**field offset** before believing an equal row in this function.)

### Why the surplus cannot be deleted without the standing owner ruling

`succeeded` exists only because our `saved_game_file_close` sits between the assignment and the test
(STORE/CALL ORDER law). Deleting it requires the close to leave the middle, and January's bytes pin where it
then goes:

```
T 156  add esp, 0xc
T 159  or  esi, 0xffffffff          ; player_profile_index = NONE
T 15c  lea eax, [ebp-0x310]
T 162  push esi                     ; <-- the NONE-valued result is the close's index argument
T 163  push eax
T 164  call _saved_game_file_close
T 16c  pop edi
T 16d  mov eax, esi                 ; ... and the SAME register is returned
```

so January calls `saved_game_file_close(&file, NONE)` on the write-failure path.

**Independently re-verified this wave** (new evidence, not inherited): `source/saved games/saved_game_files.c`
lines 578-611 decode that argument immediately —
`SAVED_GAME_FILE_INDEX_TYPE / _MEMORY_UNIT / _FILE_INDEX(profile_index)` — and fire `match_assert` at lines
603, 606, 607, 608 and 609; `NONE` fails 603 (`memory_unit==_memory_unit_hard_drive`) first. The park criterion
recorded in `config/parked.json` ("Close using the valid opened index ... never pass NONE to the asserted close
API") is therefore a **true statement about the January API**, and LAW Z does not refute it. Per WORKER_BRIEF
section 2 the reopen precondition is not met, so **no park reopen is proposed**.

### Leverage (b) resolved: the missing relocation IS recoverable defect-free — re-measured at this HEAD

The orchestrator's second signal (January references `_saved_game_file_close` once more than we do) is real and
is the tail duplication of a single source call. The defect-free body that recovers it already exists:
`scratch/workers/n1_player_profile_game_state_xbox_etc/pp_v4.c` — the TU-local
`union player_profile_file_block { byte data[SAVED_GAME_FILE_BLOCK_SIZE]; struct player_profile_file_contents contents; }`
plus the close duplicated into both arms, always receiving the **valid** opened index. Re-gated today against
the current HEAD (`--out scratch/workers/n5_player_profile/pp_v4.obj`):

| row | HEAD | `pp_v4` | target |
| --- | ---: | ---: | ---: |
| `_player_profile_new` | 464 / 18 relocs / frame 0x314 / `[relocs 18!=19, sha]` | **464 / 19 relocs / frame 0x310 / `[sha]`** | 464 / 19 |
| `_player_profile_create_default_profiles_on_disk` | 336 | **320** | 288 |
| `_player_profile_write_thread_proc@4` | 384 | 384 (inert) | 352 |
| all other rows | 18 EXACT | 18 EXACT | — |

So both structural gaps (the relocation and the frame) close with **no defect at all**; the `= {0}` fill stays
and the serialized padding stays fully initialised.

### Distance from the defect-free body to exact (new measurement)

`alndiff --ours-object scratch/workers/n5_player_profile/pp_v4.obj` leaves exactly one difference class, the
failure-arm order:

```
January     error; delete; add esp,0xc; or esi,-1; lea eax; push esi; push eax; call close   (index = NONE)
defect-free error; delete; add esp,0xc; <reload [ebp-4]>; lea; push; push; call close; or eax,-1
```

Any defect-free spelling must reload the valid index for the close and only then set `NONE`, which is one extra
instruction and a different order. The `or esi,-1` / `push esi` pair is reachable **only** by passing NONE.
Both branch polarities were already measured inert by n1 (`pp_v2`: VC7 canonicalises `!a||!b` into `a&&b`), so
arm order is not a separate lever. The two-variable reading (a separate `result` while the close keeps the
valid index) is refuted by January pushing the very register it returns.

### Shapes

**Zero source shapes were spent on the real file.** The only exact body is the owner-rejected one (already
measured strict EXACT at 456 meaningful / 464 padded and rejected on 2026-09-09); laws_w3 **R13** binds a prior
rejection, and WORKER_BRIEF section 5 forbids reproducing an original bug without an owner ruling. The single
action taken was a zero-risk re-measurement of the existing defect-free candidate.

## `_player_profile_get_argb_color` — residual RECLASSIFIED (structural, not x87, not a register tie)

Target 80 padded / **68 meaningful** / 1 relocation; the parked body is 80 padded / **74 meaningful** / 1
relocation. laws_w2 section D lists this row as do-not-repeat after 3 w2 shapes and `config/parked.json`
describes the residual as "the x87 instruction schedule remains non-exact".

**Both statements are wrong, and that is the wave's main correction.** There is no x87 instruction in either
object, and the delta is not a schedule: it is two surplus dead stores into the local.

```
JANUARY   16 mov ecx,[eax]   18 mov edx,[eax+4]   1b mov esi,[ebp+8]   1e mov eax,[eax+8]
          21 mov dword [ebp-0x10],0x3f800000      28 mov ebx,[ebp-0x10]  2b mov edi,esi
          2d mov [edi],ebx   2f mov [edi+4],ecx   32 add esp,8   35 mov [edi+8],edx   38 mov [edi+0xc],eax
OURS      16 mov ecx,[eax]   18 mov esi,[ebp+8]
          1b mov [ebp-0xc],ecx      <-- surplus dead store
          1e mov edx,[eax+4]
          21 mov [ebp-8],edx        <-- surplus dead store
          24 mov eax,[eax+8]        (its store IS eliminated)   27 mov dword [ebp-0x10],0x3f800000  ...
```

6 bytes = 74 - 68, exactly the two stores. VC7 dead-store-eliminates **all three** local rgb stores of
`argb_color.rgb = *player_profile_get_rgb_color(&argb_color.rgb, color_index);` in January, forwarding ecx/edx/eax
into the 16-byte `*result = argb_color` copy; our build eliminates only the last one.

Everything else about the current source is positively confirmed by the bytes and needs no change:
`sub esp,0x10` = one 16-byte `real_argb_color` local at `[ebp-0x10]`; `lea ecx,[ebp-0xc]` = `&argb_color.rgb`
(alpha at offset 0, rgb at offset 4, `source/math/real_math.h:369-381`); the alpha store sits **after** the
call, so the alpha assignment follows the rgb assignment in source order; the store-then-reload of
`0x3f800000` through `[ebp-0x10]` proves the alpha is read back out of a named local, i.e. the tail really is
`*result = argb_color;`. Ghidra `scratch/ghidra/out/player_profile.obj.decomp.c:679-699` agrees
(`local_10`, all three components read through the returned pointer).

### Shapes tried (4, scratch only — the body is parked)

| shape | file | result |
| --- | --- | --- |
| control (= real file) | `scratch/workers/n5_player_profile/v0.c` | 80 / 1 / `[sha]`, 2 dead stores |
| named `real_rgb_color *rgb_color` result, alpha assigned before the copy | `v1.c` | 80 / 1 / `[sha]`, **worse** — the alpha store+reload migrates ahead of the copy (0x19/0x22) |
| named result pointer, rgb copy first | `v2.c` | byte-identical to the control; the pointer-local spelling is inert here |
| `argb_color.alpha = 1.f;` before the call | `v3.c` | the alpha store is emitted **before** the call (`O 11 mov dword [ebp-0x10],0x3f800000; O 18 call`), because the local's address escapes via `&argb_color.rgb` so the store cannot cross the call — definitively refutes the alpha-first reading, re-derived from bytes rather than from a w2 verdict |

All four preserved the 18 EXACT rows; nothing was applied to the real file.

## `_player_profile_create_default_profiles_on_disk` and `_player_profile_write_thread_proc@4`

Both remain owner-blocked by `saved_game_family_reconciliation_20260909.md` (serialising uninitialised padding).
LAW Z was applied as a detector to both and produces **no shape**, because in each case the entire
callee-saved-register delta is a consequence of the owner-required `= {0}` initializer, not an independent
source defect:

- `create_default_profiles_on_disk`: January `edi` is the loop counter (`xor edi,edi` @0x0c, `inc edi` @0x102)
  and `ebx` holds one byte (`mov bl,al`); ours must move the counter to `ebx` (`xor ebx,ebx`, `inc ebx`)
  **because** the fill claims `edi` (`lea edi,[ebp-0x40e]`, `rep stosd`, `stosw`). Relocations already match
  14/14. The n1 union declaration takes the row 336 -> **320** with the padding still fully initialised.
- `write_thread_proc@4`: `ebx` (2 defs / 1 use) and `esi` (2 defs / 4 uses) are identical to January; the only
  delta is `edi`, 1 def in January vs 4 in ours, three of which are the fill. Relocations already match 27/27
  and the frame is identical; the twelve extra instructions are the fill plus the displaced
  `push edi` / `mov edi,[esi]` pair.

## Proposals

No park reopen, no header change, no config change, no symbols.json change from this unit.

One documentation correction is recommended for the orchestrator: the `config/parked.json` evidence string for
`_player_profile_get_argb_color` describes the residual as an x87 schedule; the measured residual is two surplus
dead stores from incomplete dead-store elimination and involves no x87. (Recorded here only; workers do not edit
config.)

## Reopen criteria

1. `_player_profile_new` — an owner ruling admitting January's `saved_game_file_close(&file, NONE)` ordering
   under the original-bug policy with the `/* BUG (preserved for exact matching): */` comment form. It is a
   defined fatal-assert path (`saved_game_files.c` 603/606/607/608/609), not undefined behaviour. Prepared
   candidate: `scratch/workers/n1_player_profile_game_state_xbox_etc/pp_v4.c` with the close moved after the
   `if` block. The same commit must re-baseline all three affected parks.
2. `_player_profile_create_default_profiles_on_disk` — an owner ruling on serialising uninitialised padding;
   that ruling alone closes the row at 275/288. Rank above (3).
3. `_player_profile_write_thread_proc@4` — the same ruling **plus** a separate explanation for the remaining
   `+0x10b` code byte.
4. `_player_profile_get_argb_color` — a measured VC7 law that makes a 12-byte member assignment from a call
   result emit "load all three, then store all three" instead of interleaved load/store pairs; the batched form
   is what lets dead-store elimination remove all three stores.
