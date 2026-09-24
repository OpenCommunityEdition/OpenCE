# MEASURED - owner-queue packet ub_dead_camera_saved_game

Base: cdc8ebd3 (Halo objects 379). HEAD moved to eb33baec during the session. That was an integrator
ledger commit touching docs only, so source/, config/ and tools/ are identical and every check below
holds at both commits. `git status` stayed clean throughout.

Tools: `gate_slug.py` is a copy of tools/campaign/gate.py. It differs only in writing its temp
files under this slug, honouring GATE_WORKDIR for shadow-header compiles and printing warnings on
request. `object_audit_env.py` and `pdb_storage_env.py` are copies that take the split and candidate
paths from the environment. `surplus_identity_cand.py` is surplus_identity.py pointed at a candidate
object. `secs_cmp.py` compares every non-debug section and symbol of two objects. Split emulation ran
the real `build/tools/csplit.exe` on a copied config (`csplit/`). The control run reproduces all 833
objects of build/split byte for byte.

## 1. dead_camera `_dead_camera_update` (1,248 padded / 1,235 meaningful)

### 1.1 Gate (`--all`, every January function of the object)

| Function | production (head.c = build/base) | patch 01 (landing.c) |
|---|---|---|
| `_dead_camera_new` 288 | EXACT | EXACT |
| `_dead_camera_update` 1248 | residual [sha] | **EXACT** |
| `_player_get_next_player_with_a_unit` 176 | EXACT | EXACT |
| `_player_has_allies` 112 | EXACT | EXACT |
| **Total** | 3 / 1 / 0 | **4 / 0 / 0** |

- The production residual is exactly one instruction (alndiff). At +0x176 January has
  `mov eax,[ebp+0x10]` and we have `mov eax,esi`.
- head.obj equals build/base on all 26 sections.
- landing.obj differs from head.obj only in the `_dead_camera_update` section.
- The held packet's comment and the landing comment produce identical objects (26/26 sections and all
  symbols). The TU's asserts use literal line numbers, so the comment is byte-inert.

### 1.2 Object audit

- **No symbols.json change is needed.** The `_dead_camera_constants` static landed in batch 1, and the
  current build/split already has storage 3 on it, so the emulated split is the current split.
- `object_audit landing.obj`: 6/6 January sections ok (both .rdata sections and all 4 .text). 6 January
  symbols, 0 differ. **PASS**.
- pdb_storage: 0 disagreements (6 symbols), for both production and the candidate.

### 1.3 Surplus, link and data

- **Surplus:** 8 code COMDATs (`_dot_product3d`, `_magnitude_squared3d`, `_real_local_random_range`,
  `_valid_real`, `_valid_realcmp`, `_valid_real_vector3d`, `_valid_real_normal3d`,
  `_valid_real_vector3d_axes2`) plus 11 literals and .drectve. All are already emitted by production.
  The candidate adds no new emission.
- **Surplus identity:** 8/8 IDENTICAL to January's selected copies.
- **provider_link (both orders): PASS on 19/19.** The wave-1 pre-existing FAIL (`_valid_real_vector3d_axes2`
  against bored_camera's NODUP hand copy) is gone, because Layer 1 removed that copy.
- **Data:** build/report.json dead_camera .rdata is 48/48 (100%).
- **fake_match_scan:** 0 leads.
- **Warnings:**
  - /W3 is clean.
  - /W4 gives exactly one new warning: `C4701 local variable 'next_unit_index' may be used without having
    been initialized`, at the `if (next_unit_index != camera->unit_index ...)` test.
  - The production build passes no /W flag.
- **Parks, rejections, semantic entries:** none. config.json:743 shows dead_camera.c as NonMatching.

### 1.4 Completion if YES

**dead_camera becomes COMPLETE:** 4/4 strict, 6/6 sections, 6/6 symbols, PDB storage 0, surplus
identical, provider link PASS, data 100%. objdiff code moves from 557/1,792 to 1,792/1,792 (+1,235
meaningful).

Header blast radius: none (the change is .c-only).

## 2. saved_game_files `_saved_game_files_enumerate_available_to_local_player_index` (368 padded / 359 meaningful)

### 2.1 Gate (`--all`, 46 January functions)

| Variant | Result |
|---|---|
| production (head, == build/base 162/162 sections) | 45 / 1 / 0 (enumerate_available [sha]) |
| patch 02 only (storage) | 45 / 1 / 0; object == head except `_enumerate_memory_units` storage 2 -> 3 |
| patch 03 only (bug) | 46 / 0 / 0 |
| **patches 02 + 03 (full)** | **46 / 0 / 0** (all 46 rows EXACT; saved_game_files/gate_full.txt) |

- The shadow header was verified with an `#error` probe for saved_game_files and three includers.
- full differs from 03-only only in the storage flip.

### 2.2 Split emulation and object audit

- **csplit, edited config** (symbols.json line 7376 edited in place, CRLF kept): only
  `source/saved games/saved_game_files.obj` changes. In it, 150/150 sections are identical and exactly
  one symbol flips (`_enumerate_memory_units` 2 -> 3).
- **object_audit, full against the emulated split: PASS** (152 January symbols, 0 differ).
  - Against the current split it shows the expected single storage DIFF (2/3) until symbols.json lands.
  - The 03-only candidate against the current split: PASS.

### 2.3 Surplus, link, data and blast radius

- **surplus_identity:** 0 candidate-only code COMDATs.
- **provider_link (both orders): PASS on 11/11** (8 pooled literals and 3 stock D3D SDK tables).
- **Data:**
  - report.json: .bss 540, .data 8 and .rdata 5,608, all at 100%.
  - Every data section of the candidate is identical to production.
- **Header blast radius of patch 02** (saved_game_files.h loses the `enumerate_memory_units` prototype).
  The includers are game, player_ui, ui_widget, virtual_keyboard, player_profile and playlist_profile.
  Measured on all six:
  - gate rows are identical;
  - every non-debug section and symbol is identical;
  - a production-header compile equals build/base.
- **fake_match_scan:** 0 leads.
- **/W3:** 33 warnings, the same set as production. No C4211, C4013 or C4700.
- **/W4:** exactly one new warning, `C4701 'number_of_available_profiles' may be used without having been
  initialized`, at the final `*number_of_profiles = ...` store. Production already has 2 unrelated
  C4701s (`checksum_data_size`).

### 2.4 PDB storage: 3 disagreements remain, and the wave-1 dismissal is refuted

`pdb_storage` gives these counts:

- production: 4 disagreements;
- full candidate against the emulated split: **3 disagreements**. They are
  `_saved_game_files_take_mapfile_mutex`, `_saved_game_files_release_mapfile_mutex` and
  `_saved_game_files_enumerate_default_files`. Each is external in the split and in ours, is absent from
  cachebeta publics, and has no reference anywhere in January.

Wave 1 argued that these three were "proven external" because a static function made in the lab came out
UNWRITTEN. That argument is the one the wave-3 hs review refuted (law (h)): a static that the source
**uses** is emitted even when every call is inlined, and 148 Halo functions are unreferenced and
non-public. The wave-1 lab made the helper static without using it, so it was never going to be emitted.

The first-party evidence points the other way:

- **/Od (odbuild callers):**
  - The mapfile-mutex take and release helpers (0x864f00 and 0x864ec0) are called from 6 functions each.
  - Our TU hand-expands `take_mutex` / `release_mutex(saved_game_files_globals.mapfile_mutex ...)` in
    exactly 6 functions: `set_nth`, `enumerate_available`, `find_profile_index_for_directory_path`,
    `add_new`, `remove_nth` and `get_nth`.
- **`enumerate_default_files`:** no /Od attestation was found (the PC build differs here). Its
  playlist+player sum is hand-expanded twice in `enumerate_memory_units`.

**LAB, not part of the packet** (`lab/LAB_ONLY_saved_game_files_helper_storage_on_top_of_03.diff`): make
the three helpers static, call them at the hand-expanded sites, and move their prototypes into the .c
private block. Results:

- gate 46/46;
- every section identical to the full candidate, with all three helpers emitted at storage 3 (law (h)
  confirmed on this object);
- csplit with 3 more in-place `"static": true` lines changes only this object;
- object_audit PASS against that split;
- **pdb_storage 0**;
- provider_link PASS; header blast radius 0 on all 6 includers; fake scan 0.

This is a real source change (it removes a section-8(g) hand-expansion class). It needs its own review, and
it is independent of the UB ruling.

### 2.5 Completion if YES

- **Strictly exact:** 46/46. object_audit PASS against the emulated split; data 100%; surplus and link PASS.
- **Not yet clean-COMPLETE under brief section 4, step 3:** 3 PDB-storage disagreements remain unless one of
  two things happens:
  - the lab follow-up is reviewed and lands (the lab reaches 0); or
  - the owner explicitly accepts the three as external.
- **Recommendation:** do not flip config.json:398 on the ruling alone.
- **Credit on YES:** objdiff code moves from 11,709/12,068 to 12,068/12,068 (+359 meaningful). The
  park for enumerate_available must be retired.

## 3. Summary

| Item | Patches apply at cdc8ebd3 | Strict gate | object_audit (emulated split) | provider_link | PDB storage | Object complete if YES |
|---|---|---|---|---|---|---|
| dead_camera | yes (01; both EOL forms) | 4/4 | PASS | PASS 19/19 | 0 | **yes** |
| saved_game_files | yes (02, 03; both EOL forms) | 46/46 | PASS | PASS 11/11 | 3 (lab follow-up: 0) | **conditional** (needs the storage follow-up or owner acceptance) |

No full ninja build or stable 8,245-function sweep was run: workers are forbidden to run ninja. The
integrator must run both after applying.
