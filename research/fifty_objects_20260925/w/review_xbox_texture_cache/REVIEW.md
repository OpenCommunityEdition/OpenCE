# Adversarial review: source/cache/xbox_texture_cache (claimed OBJECT_COMPLETE_CANDIDATE)

Reviewer worktree: C:\halo-worktrees\claude-fifty-objects-20260925 (base e9e62b78, tracked tree clean).
Evidence files: this directory (gate_all.txt, gate_prod.txt, object_audit.txt, pdb_storage.txt, surplus_identity.txt,
provider_link.txt, X*.c/.obj strip variants, F1_* lab, LEDGER.md).

## Verdict
* **Function-level landing: APPROVED.** `_texture_cache_steal_memory` becomes strictly EXACT with a natural named local;
  applying production.patch and retiring the park is safe (TU-local, no header/config/symbols change).
* **Whole-object COMPLETE / Matching promotion: NOT APPROVED.** Bytes/data/surplus pass, but object_audit is FAIL(3)
  on split storage metadata and the TU carries pre-existing house-rule debt that recent admissions treat as blocking.

## Independent re-runs
| check | result |
|---|---|
| gate --all --forbid-emitted-symbol _point_from_line3d (cand.c sha256 55588ed4...) | 25 exact / 0 residual / 0 unwritten |
| gate --all on production | 24 exact / 1 residual (steal_memory [sha]) -> only steal_memory moves |
| object_audit | all January-owned .text/.rdata/.bss OK; FAIL(3) storage 2/3 on IDirect3DBaseTexture8_IsBusy@4, IDirect3DDevice8_IsBusy@4, IDirect3DDevice8_KickPushBuffer@4 |
| pdb_storage | none of the 3 in cachebeta publics (publics do list decorated stdcall names) => truth static = ours; split metadata wrong |
| build/report.json | data 6344/6344; code 3248/3457, steal_memory (209 B, 96.96%) the only gap |
| surplus_identity | _add_vectors3d/_dot_product3d IDENTICAL |
| provider_link (both orders) | SELECTED-PROVIDER LINK PASS (19 surplus rows) |
| patches | git apply (autocrlf=true copy) and patch --binary (CRLF) both reproduce cand.c byte-for-byte |

## Strip test / fake-matching review of the diff
The diff adds `byte *stolen_address = base_address + XBOX_TEXTURE_CACHE_STEAL_GUARD_SIZE;` (decl+init combined, rule 17),
used by the READWRITE XPhysicalProtect and the single return. No cast, paren, pragma, dummy, volatile or helper change.
* Xa: named local declared+initialised but unused -> **residual** (so this is not a name-count/declaration-count artifact;
  worker's O1/O2 oracles agree).
* Xb (used only in return) and Xc (used only in the call) -> EXACT. Exactness comes from the value originating in an
  early named definition (reload-like web), consistent with the worker's allocator decode and sm5 (late named local = residual).
* R1-R4 + void* variant exact: not a knife-edge spelling. Not a repeat of w2 sm1 (that derived the end guard from the local).
The named local is ordinary Bungie-plausible C carrying a real twice-used value. Passes rules 1-21 for the changed lines.

## Blocking items for whole-object admission (pre-existing, outside the patch)
1. Split storage metadata for the 3 XDK wrappers: add `"static": true` to their config/symbols.json entries (PDB-backed,
   same fix the 20260923 rasterizer_xbox_decals admission applied) and regenerate the split; then object_audit should pass.
2. Caller-local prototype `long bitmap_get_pixel_data_size(...)` although bitmaps/bitmaps_internal.h:43 owns it.
   Lab F1 (include the owner header, drop the local prototype) keeps 25/25 EXACT with unchanged surplus.
3. Public texture_cache_debug_render / bitmap_format_to_d3d_format / bitmap_format_to_d3d_linear_format prototyped only in
   the .c; owner texture_cache.h lacks them (lruv_cache was held on 20260923 for exactly this class). Header edit => full sweep.
4. TU-local duplicate struct bitmap_data (owner bitmap_group.h:29) and bitmap enums - the 20260904 ledger's named
   "Bitmap schema/prototype debt" whole-object veto.
5. `extern D3DDevice global_d3d_device;` is consumer-local and type-inconsistent with `D3DDevice *global_d3d_device`
   (rasterizer_xbox.c:856, rasterizer_xbox.h:164).
6. xbox_bitmap_group_prefix reserved-byte view + raw tag_get('bitm') (rule 4); config.json status flip was not proposed.
