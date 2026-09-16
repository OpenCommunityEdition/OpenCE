# `game_sound.obj`: opus5 next-150K lane, wave n4 (relocation-census pass, 2026-09-15)

## 1. Scope and provenance

- Translation unit `source/sound/game_sound.c`; target `build/split/source/sound/game_sound.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, XDK 3911, repository flags; no flag changed or passed).
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch
  `opus/next-150k-house-clean-20260915`.
- Assigned target: `_update_potentially_audible_looping_sound` (PARKED, scratch-only), with the
  orchestrator's relocation-by-target leverage "January owns `_game_looping_sound_delete` ×1 that ours does
  not; ours owns `_game_sound_globals` ×1 that January does not".
- **Nothing was written to `game_sound.c`, to any header, or to `config/*.json`.** No ninja, configure,
  pytest or git write command was run. `git diff --stat -- source/sound/game_sound.c` is empty.
- Worker notes `scratch/workers/n4_game_sound_sound_manager.md`; scratch
  `scratch/workers/n4_game_sound_sound_manager/`.
- Ledgers read: `game_sound_obj_opus5_next150_n1_20260915`, `game_sound_obj_opus5_150k_w2_20260914`,
  `game_sound_obj_obstruction_current_owner_boundary_20260905`,
  `game_sound_obj_pas_current_context_integration_20260904`, `game_sound_obj_pas_independent_review_20260904`,
  `devices_game_sound_small_fuzzy_harvest_20260908`, `game_sound_obj_jonas_public_leaf_pair_20260828`,
  plus `scratch/workers/n1_game_sound_ima_adpcm.md`, `scratch/workers/bink_playback_game_sound_etc.md`,
  `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, `config/parked.json`.
- Evidence: January split-object bytes, relocations and section order (authoritative); the w2 scratch bodies
  `pas_v2.c` and `gs_v6.c`, both re-gated at this HEAD.

## 2. Baseline and final state (identical)

`python -B tools/campaign/gate.py source/sound/game_sound --all --forbid-emitted-symbol _point_from_line3d`
reports `== exact 28  residual 3  unwritten 0  (of 31 listed)` at both ends; the guard passed;
`scratch/parkcheck.py` on the real object reports `parks 2 drift 0`; `tools/fake_match_scan.py` reports
0 leads. `branch_sweep.py` sees 13 unique blobs and the current tree is the best of them (next best 27/4/0).

## 3. Result

Nothing landed (the assigned row is a parked body). The wave confirmed the orchestrator's leverage as a
**genuine missing statement in the parked body**, verified that the w2 scratch body `pas_v2.c` repairs it and
reaches size- and relocation-identity exactness, and files the park re-baseline. It also found, by running
`relocdiff --count-by-target` over the other two non-exact rows, a previously unrecorded relocation-level
defect in `_compute_sound_obstruction` (a wrong assert expression literal) and re-measured the exact scratch
body for that row, which remains blocked on one pending owner ruling.

## 4. `_update_potentially_audible_looping_sound` — park premise satisfied, re-baseline proposed

Target **624 padded / 619 meaningful / 25 relocs / `8fe3a4a813d3a2a8`**;
parked real body **640 / 25 / `6763e595e2871c3e`**;
candidate `scratch/workers/n4_game_sound_sound_manager/pas_v2.c` → **624 / 25 / `f3008036d008e7f4`**.

### 4.1 The census delta is a missing source statement

`relocdiff --count-by-target` on the real (parked) object versus January:

| target | JAN | PARKED BODY | `pas_v2` |
| --- | ---: | ---: | ---: |
| `_game_looping_sound_delete` | 2 | **1** | 2 |
| `_game_sound_globals` | 5 | **6** | 5 |
| `_sound_refresh_looping` | 2 | 2 | 2 |
| `_display_assert` / `_system_exit` | 2 / 2 | 2 / 2 | 2 / 2 |
| `_datum_get`, `_tag_get`, `_game_looping_sound_data`, `_object_get_node_matrix`, `_object_get_function_value`, `_object_get_velocities`, `_matrix4x3_transform_point`, `_matrix4x3_transform_normal` | 1 each | 1 each | 1 each |
| TOTAL | 25 | 25 | 25 |

January calls `game_looping_sound_delete` **twice** — at `+0x1d5` (inside the audible arm, after
`sound_refresh_looping` returned TRUE and the unattached flag is set) and at `+0x252` (the not-audible
unattached arm) — and references `_game_sound_globals` exactly **five** times.

The parked body funnels both unattached-delete paths into one shared bottom
`game_looping_sound_delete(looping_sound_index);` and pays for it with six early `return;` statements, each
repeating `sound->last_audible_frame_index = game_sound_globals->update_index;`. That is one delete call too
few and one globals reference too many: a **control-flow defect**, which is exactly what the park entry names
as its reopen condition ("Reopen only for authoritative source/type/control-flow or compiler-context
evidence"). It is the SINGLE-EXIT LAW (laws_w2 A3): one bottom write of the frame field, early returns
rewritten as else arms. The w2 ledger derived the same shape from January's jump-threaded
`cmp ax,2; je tail; test serviced; jne main; cmp ax,2; je tail; store` fingerprint; the relocation census is
independent confirmation and is new.

### 4.2 `pas_v2.c` re-measured at this HEAD

- whole-TU gate `== exact 28 residual 3 unwritten 0`, row-for-row identical to baseline except this row;
  `_point_from_line3d` guard passed.
- size **624 == target**; relocations **25 == target**; `relocdiff --count-by-target` equal on every target
  and addend. The single remaining count row is the select-any `??_C@_08CBBFJFMD@location?$AA@` assert
  literal, which January references undefined because csplit assigned that COMDAT to another object;
  relocdiff resolves the pair equal and the same string is already candidate-only at HEAD.
- `scratch/parkcheck.py`: `parks 2 drift 1` — drift only on this park; `_game_sound_update` unchanged.
- owner census versus `build/base/source/sound/game_sound.obj`: **identical** (nothing added, nothing
  removed; 18 candidate-only owners versus January on both sides — the accepted systemic select-any class,
  laws_w3 A30).
- `tools/fake_match_scan.py`: 0 leads, same as HEAD.
- source diff versus the real file: **one function, two hunks**, no header or config change.

### 4.3 What remains at 624/25

Four meaningful bytes, the zero-register pin decoded in the n1 ledger: January hoists `xor ecx,ecx` into the
dominator before the `sound->object_index != NONE` test, re-zeroes it on the then-arm edge, and spends it on
the `_sound_spatialization_mode_none` store, the `state == _game_looping_sound_active` compare, the `0.f`
push and the `state = _game_looping_sound_active` store. Not re-attacked: n1 already spent the brief's
two-shape tie budget (`pas_v2`, then `g1` = `0` instead of `0.f`, byte-identical).

### 4.4 Reopen criterion after the re-baseline

Unchanged from n1: a second measured instance of the zero-register-pinning lever family, i.e. a source handle
that makes VC7 promote the constant 0 to a register across an if/else join in a function with no zeroed
aggregate. Start from `pas_v2.c`. Do not retry `0` versus `0.f`, ternary/if-else rewriting, declaration
reordering, or a named zero local.

## 5. `_compute_sound_obstruction` — new relocation fact, still owner-blocked

Target **512 padded / 498 meaningful / 33 relocs / `2040bfc2f77a485e`**; real file **496 / 33 /
`13363fc7c551b842`** (ours is 16 bytes short).

**New this wave (recorded in w2 as prose, never as a relocation fact):** the assert expression literal is a
mismatched relocation target, so strict exactness is unreachable without fixing it.

| side | mangled literal | length code |
| --- | --- | --- |
| January | `??_C@_0EB@JEFKCLHK@source?9?$DOspatialization_mode?$DN?$DN_so@` | `0EB` = 65 |
| ours | `??_C@_0ED@OGIIIPOH@source?9?$DOspatialization_mode?5?$DN?$DN?5_@` | `0ED` = 67 |

The two-character difference is exactly the two `?5` (space) characters around `==`, so January's source
spells the predicate `source->spatialization_mode==_sound_spatialization_mode_absolute` with no spaces
(docs/assertions.md: exact expression text). The owner census confirms it: the spaced literal is a
candidate-only owner in `build/base/source/sound/game_sound.obj` today.

The w2 body `scratch/workers/n4_game_sound_sound_manager/gs_v6.c` was re-gated at this HEAD:
`== exact 29 residual 2 unwritten 0`, with `_compute_sound_obstruction` **strict EXACT**
(512 / 33 / `2040bfc2f77a485e`) and no other row changed; guard passed. Its owner delta versus `build/base`
is the corrected assert literal plus exactly one new owner, `_vector_from_points3d` — the recorded blocker
(laws_w3 R14).

Argument the w2 packet did not make: the body currently in the production tree hand-expands that helper
(`vector.i = source->location.position.x - camera->position.x;` and two siblings), which is itself the
laws_w3 **R15 hand-expanded-helper class**. Admitting the systemic select-any COMDAT therefore removes a
house-rule violation from the tree at the same time as it closes the row.

## 6. `_game_sound_update` — skipped as exhausted

Target 464 / 31 / `d0c2dc17…`; parked body 464 / 31 / `df019d7c…` — size and relocation count already exact.
`relocdiff --count-by-target` at HEAD: 15 targets, every count equal, TOTAL 31 == 31. No missing or extra
call, no width or ABI fact, only 17 differing address rows. laws_w2 section D lists the row with two shapes
spent and a tie verdict; w2 classified it a confirmed register-colour tie; the park text forbids declaration
shuffling and equivalent-spelling searches. No law in laws_w2/laws_w3 fires on an all-equal census at an
exact size, so no shape was spent.

## 7. Proposals

**P1 — owner ruling (re-filed, freshly measured): admit the `_vector_from_points3d` surplus select-any
COMDAT.**
- Ask: put `_vector_from_points3d` into the accepted systemic select-any header-inline COMDAT class
  (laws_w2 A4 census, laws_w3 A30), alongside `_dot_product3d`, `_set_real_point3d`, `_sine`, `_normalize3d`,
  `_scale_vector3d`, `_distance3d` and the real_math surplus. This is a pending ruling, not one of the FINAL
  declined items in brief section 1.
- Evidence: `build/base` already emits the COMDAT with selection ANY in 75 objects; only csplit's January
  `action_alert.obj` copy has selection 1, which is an artifact of splitting one linked image (a select-any
  COMDAT survives in exactly one object). The helper is a non-static `__inline` in `source/math/real_math.h`,
  so the emission is forced by the header inline linkage law.
- Measured at this HEAD: `gs_v6.c` gives `== exact 29 residual 2 unwritten 0`, one row changing.
- Consumers: none beyond `game_sound.obj` for this landing. (`_rasterizer_sun_glow_draw` needs the same
  ruling per laws_w3 C3 but additionally fails R15/R16 and is not part of this proposal.)
- Unlocks: **512 padded / 498 meaningful strict-exact bytes**, and deletes an R15 hand expansion from the
  production tree.
- If declined, please say so explicitly so the row can be reclassified from residual to owner-declined.

**P2 — park re-baseline: `_update_potentially_audible_looping_sound`.**
- Candidate `scratch/workers/n4_game_sound_sound_manager/pas_v2.c` (the current real file plus exactly this
  one body).
- `config/parked.json` base `640 / 25 / 6763e595e2871c3e…` → **`624 / 25 / f3008036d008e7f4…`**, and land the
  body. Criterion met by the section 4.1 control-flow census; zero strict credit; verification in section 4.2.

## 8. Disposition

`game_sound.obj` stays at 28/31 strict exact; this wave claims no bytes and changes no file. It leaves one
actionable zero-credit park re-baseline that takes the largest parked row from 640/25 to 624/25 with exact
size and exact relocation identity and repairs a real missing call, and one owner ruling that is worth
512 padded bytes the moment it is granted.
