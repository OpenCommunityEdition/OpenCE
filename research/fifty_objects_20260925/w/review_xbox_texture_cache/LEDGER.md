# REVIEW ledger: xbox_texture_cache OBJECT_COMPLETE_CANDIDATE (reviewer, fifty-objects lane)
- R-0: cand.c sha256 55588ed4... matches worker-recorded hash. Diff vs production (e9e62b78, clean tree) = 3 hunks in
  texture_cache_steal_memory only (named `byte *stolen_address`, used in first XPhysicalProtect and return).
- R-1: independent gate --all --forbid-emitted-symbol _point_from_line3d -> 25 exact / 0 residual / 0 unwritten (gate_all.txt).
- R-2: production gate: 24 exact / 1 residual (steal_memory [sha]) -> the edit moves only steal_memory; 24 others unchanged EXACT.
- R-3: object_audit (my cand.obj): all January-owned .text/.rdata/.bss OK; FAIL(3) = storage 2/3 on
  IDirect3DBaseTexture8_IsBusy@4, IDirect3DDevice8_IsBusy@4, IDirect3DDevice8_KickPushBuffer@4. Register@8 already static in split.
  pdb_storage: none of the 3 are cachebeta publics (publics DO list decorated stdcall names, e.g. _D3DResource_IsBusy@4,
  so absence is meaningful) => January truth = static = ours; split metadata (symbols.json) is the wrong side.
  Precedent: rasterizer_xbox_debug (Matching) audits FAIL(8) same class; BUT 20260923 rasterizer_xbox_decals admission
  fixed the analogous split-external/PDB-static gap via symbols.json "static": true before promoting. Metadata follow-up, not a source defect.
- R-4: report.json: data 6344/6344 100%; code 3248/3457, only steal_memory (209, 96.95652%) below 100.
- R-5: surplus_identity: 2 code COMDATs IDENTICAL; provider_link (both orders) SELECTED-PROVIDER LINK PASS (19 surplus rows).
- R-6 STRIP/ORACLE variants of the named local (gate --fn steal_memory):
  Xa named local declared+initialised but UNUSED (call/return keep base+GUARD) -> residual [sha] (not a name-count effect);
  Xb named local used only in the return -> EXACT; Xc used only in the READWRITE call -> EXACT.
  => exactness comes from the value originating in an early named definition (reload-like web), not from an extra
  declared name; the worker's form (used in both places) is the most natural spelling of that shape.
- R-7: patches: `git apply --check` exit 0 in worktree; `git apply` in a throwaway autocrlf=true repo copy and GNU
  `patch --binary -p1` with production_crlf.patch BOTH reproduce cand.c byte-for-byte (cmp). Throwaway repo deleted.
- R-8: park entry {source/cache/xbox_texture_cache, _texture_cache_steal_memory, unclassified} exists, target sha 7ffbda42 =
  candidate; must be retired in the same integration (parked body bytes move).
- R-9: prior negatives re-read: w2 sm1 = named writable local USED FOR THE END GUARD (different shape); sm5 = named local
  assigned AFTER resize (residual) -> consistent with the early-definition/rematerialisation mechanism; R1 is not a repeat.
  w2's "rules out a named early writable local" was an unmeasured inference, now refuted by R1-R4 exact.
- R-10 WHOLE-OBJECT SOURCE AUDIT (pre-existing debt, not introduced by this patch):
  * caller-local prototype `long bitmap_get_pixel_data_size(struct bitmap_data *)` (genuine owner bitmaps/bitmaps_internal.h:43);
  * public January functions texture_cache_debug_render / bitmap_format_to_d3d_format / _linear_format (all cachebeta
    publics) prototyped only in the .c, absent from owner texture_cache.h (render_debug.c:564 calls debug_render with no prototype);
  * TU-local duplicate `struct bitmap_data` (genuine owner bitmaps/bitmap_group.h:29) + TU-local bitmap type/format/flag enums
    (20260904 ledger named this "Bitmap schema/prototype debt" a whole-object veto);
  * `extern D3DDevice global_d3d_device;` consumer-local AND type-inconsistent with the definition
    rasterizer_xbox.c:856 `D3DDevice *global_d3d_device` / owner header rasterizer_xbox.h:164;
  * reserved-byte prefix view xbox_bitmap_group_prefix + raw tag_get('bitm', ...) (rule 4), "provisional name" debug array.
  Precedent: 20260923 admission HELD lruv_cache for "a public prototype local to its .c file"; game_state_xbox admission
  explicitly checked "no caller-local prototypes".
- R-11 LAB (follow-up feasibility, not a landing): F1 = cand.c with the caller-local bitmap_get_pixel_data_size prototype
  replaced by #include "bitmaps/bitmaps_internal.h": gate --all 25/25 EXACT, object_audit surplus + symbol DIFFs unchanged.
- VERDICT: function-level landing (production.patch + park retirement) VERIFIED/ADMISSIBLE; OBJECT_COMPLETE (Matching) NOT approved.
