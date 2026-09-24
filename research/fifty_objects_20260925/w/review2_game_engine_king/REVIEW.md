# REVIEW: source/game/game_engine_king (claimed OBJECT_COMPLETE_CANDIDATE)

Reviewer slug: scratch/w/review2_game_engine_king/ (full probe log in LEDGER.md). Base: 931ed8dc. No tracked files were edited.

## Verdict

**approve = false for the whole-object claim.** Every technical check reproduces independently. The object still cannot be
self-admitted because its completeness depends on a load-bearing reproduction of an authentic January
**uninitialised read** in `_find_next_hill`. The brief (sec. 2 and sec. 7) makes that an owner-gated class. There is no owner
ruling on record, the existing comment does not follow the original-bug-policy form, and the worker result does not
mention the issue.

The byte-inert, PDB-proven **storage correction** is verified and can land now as a zero-credit ownership correction.
The batch-1 precedent for this is profile, transport_endpoint_winsock, actor_firing_position and game_engine.

## What I re-ran (own harness, not the worker's scripts)

| Check | Result |
|---|---|
| `git apply --check`: each of the 6 worker patches, combined set, and data_gap king.patch | all OK (HEAD 931ed8dc, no tracked edits) |
| Patched copies vs worker candidates (cmp) | king_source = c2, ALT = c4; symbols/config/semantic_data_matches/symbol_ownership = worker full copies |
| rgate.py (in-place compile, shadow /I first, #error probe proves the patched header is used) | rec(c2) 29/29, alt(c4) 29/29, production 29/29 vs build/split |
| csplit with patched symbols.json/config.json into my slug | 833 objects; only game_engine_king.obj differs from build/split |
| rec / alt / production vs emulated split | 29/29 each |
| object_audit (SPLIT_ROOT copy) vs emulated split | rec PASS, alt PASS (39 January symbols, 0 differ); production FAIL(23) control |
| cachebeta_publics | only _king_engine, _render_dynamic_quad, _render_dynamic_quad_initialize are public; no public at 0x43E948 -> 22 callbacks + _king_globals file-static CONFIRMED |
| pdb_storage vs emulated split | rec 0, alt 0, production 23 |
| Surplus identity (explicit candidate obj, code + rdata) | 22 surplus sections, all IDENTICAL to a January definer |
| provider_link rec.obj / alt.obj | SELECTED-PROVIDER LINK: PASS (22 rows) |
| section_info_resolved(_king_engine) | target==base for build/emulated split x prod/rec/alt/base under both symbols.json; 136/23/4009f5b6...65a7 = entry |
| objdiff 3.3.1 (sha1 3130e428) mini report | storage change is scorer-inert; unmatched data 136 = exactly _king_engine ('$' literal defect) |
| Emulated progress chain (worker chain re-pointed at my split/obj/configs) | Halo complete 363->364, +4,865 code, +876 data, +136 matched data, snapshot OK, revoked [], parks 93/0/0, admission 15/0/7/0 |
| rec.obj vs production obj, every section | identical except .debug$S; only the 23 storage rows change 2->3 |
| UNDEF scan (build/split 833, build/base 621) | 0 references to any of the 23 names |
| Header consumers (git grep) | game_engine_king.h is included only by game_engine_king.c |
| fake_match_scan rec/alt .c+.h | 0 leads; the .c diff is exactly 23 `static` insertions |
| symbols.json copy | 23 lines edited in place, +368 B = 23 x `, "static": true`, same CRLF line count |

Strip test on the change itself: production, without `static`, is also 29/29, so `static` buys no bytes. It is required
only by the cachebeta.pdb storage evidence, which makes it legitimate.

The recorded negative in opus5_250k_house_clean_checkpoint_20260915 sec. 5 says a static king_globals loses find_next_hill
and initialize_for_new_map. It does NOT reproduce at HEAD. Relocations resolve identically to
('defined-noncode','.bss','_king_globals',428/432) on every side.

## Blocking finding: `_find_next_hill` reproduces an uninitialised read

- **January bytes:** at +0x52, `mov eax,[ebp-4]; ... ret`. [ebp-4] is the `push ecx` slot, which is never written, so the
  no-candidate path returns garbage.
- **/Od 0x5bd630:** an RTC flag at [ebp-0x15], then `_RTC_UninitUse("next_hill")` (string 0x5bd6f5). The bug is
  double-attested, and the original local name is `next_hill`.
- **Strip test:** all five defined forms (`= NONE`, `= 0`, `= hill_id`, `return NONE;`, `return hill_id;`) are 28/29 with
  `_find_next_hill` residual. The uninitialised local is load-bearing.
- **Policy:** brief sec. 2 says "Reproducing an authentic January bug needs target+layout proof, a BUG comment and owner
  approval - report, don't land". Sec. 7 lists "reproducing authentic January uninitialised reads" as owner-gated. The lane
  precedent is dead_camera, which was held as FUNCTIONS_EXACT_OBJECT_BLOCKED on the same class, with its status flip only
  after a ruling.
- **Board census:** C4700 appears in only two places, this unit and NonMatching rasterizer_xbox_environment_fog. King would
  be the first Matching object with a load-bearing C4700.
- **Current comment:** it is not in the docs/matching_methodology.md original-bug-policy form (`BUG (preserved for exact
  matching): ... A corrected build should ...`). No owner ruling for find_next_hill exists in docs; the 20260902 Fable
  text is a worker rationale.

## Recommended integration

1. **Land now (zero credit):**
   - scratch/w/king/patches/king_source.patch (c2)
   - scratch/w/king/patches/symbols_king.patch (full copy: scratch/w/king/config/symbols.json)
   - optionally scratch/w/king/patches/symbol_ownership_king_OPTIONAL.patch

   The snapshot gate has no completeness requirement. Keep config.json NonMatching. Do NOT add the semantic_data_matches
   entry: apply_semantic_data_matches raises for an incomplete unit that lacks allow_incomplete_unit.
2. **Owner packet:** scratch/w/review2_game_engine_king/patches/OWNER_GATED_king_source_bugcomment.patch. This is c2 plus a
   policy-form BUG comment and the /Od name `next_hill`. It is 29/29 against both splits, object_audit PASS, and passes
   `git apply --check`. Use it INSTEAD of king_source.patch. If the owner admits the preserved uninitialised result, apply:
   - this patch with symbols_king.patch;
   - config_status_king.patch;
   - semantic_data_matches_king.patch (no allow_incomplete_unit);
   - the optional ownership snapshot.

   The expected result is the worker's +1 object, +4,865 complete code, +876 complete data and +136 matched data.
3. **Non-blocking, pre-existing issues outside this diff:**
   - game_engine_list.c has a consumer-local `extern struct game_engine king_engine;`;
   - game_engine.h variant fields `unknown40` / `unknown4C.byte0` are placeholders;
   - the random_range COMDAT comes from real_math.h, so the data-entry reason text ("10 real_math.h helper COMDATs") is
     accurate.
