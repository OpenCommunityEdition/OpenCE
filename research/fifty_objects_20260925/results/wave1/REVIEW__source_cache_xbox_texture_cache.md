# review source/cache/xbox_texture_cache
approve=False

## checks
I re-ran every check independently in C:\halo-worktrees\claude-fifty-objects-20260925 (base e9e62b78, tracked tree clean). Notes and evidence are in scratch/w/review_xbox_texture_cache/ (REVIEW.md, LEDGER.md, gate_all.txt, gate_prod.txt, object_audit.txt, pdb_storage.txt, surplus_identity.txt, provider_link.txt, and the X*/F1* lab files).
(1) gate.py --source cand.c --all --forbid-emitted-symbol _point_from_line3d: 25 exact, 0 residual, 0 unwritten. cand.c sha256 is 55588ed4..., as the worker recorded. On production the gate gives 24 exact and 1 residual (steal_memory [sha]), so the edit moves only steal_memory.
(2) object_audit: every January-owned .text, .rdata and .bss section is OK. The audit still fails with 3 storage-class differences (split 2, ours 3) on IDirect3DBaseTexture8_IsBusy@4, IDirect3DDevice8_IsBusy@4 and IDirect3DDevice8_KickPushBuffer@4. pdb_storage shows none of the three is a cachebeta public, and the publics list does include decorated stdcall names, so their absence counts as evidence. The true storage is file-static, which is what we emit; the split metadata is the side that is wrong. rasterizer_xbox_debug is already Matching with 8 differences of the same kind.
(3) build/report.json: data 6344/6344. Code 3248/3457, and steal_memory (209 bytes) is the only function below 100%.
(4) surplus_identity: both COMDATs are IDENTICAL. provider_link in both orders: SELECTED-PROVIDER LINK PASS on all 19 surplus rows.
(5) Strip and oracle variants. If the named local is declared and initialised but not used, the function stays residual, so this is not a name-count effect. Using the local only in the return is EXACT, and using it only in the call is EXACT. This agrees with the worker's allocator decode: sm5 (named local assigned after the resize) stays residual, and R1-R4 are exact. R1 does not repeat w2 sm1, which built the end guard from the local.
(6) House rules on the diff: declaration and initialisation are combined; `byte *` matches base_address; there is no cast, parenthesis, pragma, dummy or helper change; the single return is kept. It passes.
(7) Patches: `git apply --check` exits 0. `git apply` in a throwaway copy with autocrlf=true, and `patch --binary -p1` with production_crlf.patch, both reproduce cand.c byte for byte.
(8) The park entry exists and its target sha is 7ffbda42, the same as the candidate.
(9) I audited the whole TU source for admission and found pre-existing debt (see issues).
(10) Lab F1: including bitmaps/bitmaps_internal.h in place of the caller-local prototype keeps 25/25 EXACT with the same surplus.

## issues
Split verdict. The steal_memory change is fine; marking the whole object complete is not.

FUNCTION-LEVEL LANDING APPROVED: apply scratch/w/xbox_texture_cache/production.patch (or production_crlf.patch) and retire the config/parked.json entry {source/cache/xbox_texture_cache, _texture_cache_steal_memory, unclassified} in the same integration. This is an ordinary exact closure of 209 meaningful bytes, then a full ninja and a stable sweep. The named local is genuine and not fake: the unused-local variant stays residual, and the exactness holds in every declaration order.

OBJECT_COMPLETE / Matching status NOT APPROVED. None of these blockers comes from this patch; they already exist in the TU:
(a) object_audit fails with 3 storage-class differences because symbols.json keeps the three XDK wrappers external. Truth from the PDB is static. The fix is `"static": true` on those entries plus a split regeneration, the same fix used in the 20260923 rasterizer_xbox_decals admission. The worker proposed no symbols.json change.
(b) There is a caller-local prototype of bitmap_get_pixel_data_size, although bitmaps/bitmaps_internal.h:43 owns it (rule 9). Lab F1 shows the include fix keeps all 25 functions exact.
(c) The public January functions texture_cache_debug_render, bitmap_format_to_d3d_format and bitmap_format_to_d3d_linear_format are prototyped only in the .c and not in texture_cache.h. On 20260923 lruv_cache was held for exactly this kind of debt. Fixing it is a header edit and needs a full sweep.
(d) The TU defines its own duplicate struct bitmap_data (the owner is bitmaps/bitmap_group.h:29) and its own bitmap type, format and flag enums. The 20260904 ledger explicitly named this Bitmap schema/prototype debt as a whole-object veto.
(e) `extern D3DDevice global_d3d_device;` is a consumer-local declaration whose type disagrees with the definition `D3DDevice *global_d3d_device` at rasterizer_xbox.c:856 and the declaration at rasterizer_xbox.h:164.
(f) The TU reads bitmap tags through a reserved-byte prefix view (xbox_bitmap_group_prefix) with a raw tag_get('bitm'), against rule 4. The worker also proposed no change to config.json status.
Do not promote the object until (a) through (f) are resolved or the owner rules on them.
