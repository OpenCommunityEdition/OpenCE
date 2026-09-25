# REVIEW source/objects/object_lights

approve: True

amended_patch: None

## per_object
null

## checks
I re-ran everything independently in scratch/w/review_r2w2_object_lights/ with my own tools, my own csplit output and my own shadow tree. I made no tracked edits. The claim review_r2w2_object_lights_rv1 has been released. At the end, git status shows only the untracked research dir that was already there.

(1) git apply --check at HEAD 434f0151 (= 05255584 + the lane ledger): 01, 02 and 03_NO_ALT are each CLEAN, and all three together are CLEAN. The index lines equal the HEAD blobs (2553c63c, c863833f, f9abb17d, 77814b3d). The patches are LF. My post-images hash to 9e77ec23 / 527cc0b3 / e6ea96b2 / 3f1a813e and are byte-identical to the worker's tree/ and cfg_OL/.

(2) csplit run into my slug: the verbatim config copy equals build/split, 833/833. With 02 applied, only source/objects/object_lights.obj changes.

(3) Shadow compile (build.ninja cflags, cwd = shadow root): the candidate is 43/43 EXACT against the emulated split, 12,608 padded bytes. Production is 43/43 against build/split and 43/43 against split_OL. gate.py --source with the candidate is 43/43 and gives the same object. Comparing production and candidate objects: all 109 sections are identical apart from the .debug$S path, and only the 4 symbol storage rows change (2 -> 3).

(4) object_audit, redirected to split_OL: the candidate PASSes (71 January symbols, 0 differ). Controls: production FAIL(4) against split_OL, and the candidate FAIL(4) against build/split. So 01 and 02 must land together, followed by a csplit regen.

(5) pdb_storage: production has 4 disagreements, exactly the four helpers. cachebeta publics has none of them. 0x128A10 sits between the public _sample_diffuse_texture (0x128920) and _lights_initialize (0x128A30).

(6) Reference census over all build/split and build/base objects: each of the 4 names is defined only by object_lights, with 0 UNDEF references anywhere. A git grep finds no other user.

(7) surplus_identity: 12 COMDATs, 11 IDENTICAL. The one DIFFERENT copy is _object_get_bounding_sphere, the held Q1 item that production already has.

(8) provider_link on the candidate: 33 surplus names PASS in both orders.

(9) Header consumers: render is 13/13 and render_objects is 22/22. Both objects are identical to build/base.

(10) Strip tests:
- (a) Static helper without the two calls: 42/43, with _should_render_lights UNWRITTEN. The calls are load-bearing for a January-owned section.
- (b) Static prototype removed from the .c block: 43/43 and an identical object. The prototype is byte-inert, so it is style, not filler.

(11) /Od build (data only): 0x78dab0 reads [0xded68c]+0 and calls 0x59b3a0, and it is the only caller of 0x59b3a0. 0x78dab0's only callers are 0x78c6eb in diffuse (right after begin 0x825320) and 0x78c97b in specular (after 0x825a30). January's diffuse and specular relocations reference begin, lights_game_globals and _game_engine_allow_dynamic_lighting, i.e. the helper is inlined.
- NEW corroboration beyond the worker's evidence: in the /Od build, same-TU calls to January-public functions go through incremental-link thunks (light_disconnect_from_map, light_reconnect_to_map). Every direct same-TU call or pushed address targets a January static.
- All four helpers are direct: 0x78dab0; light_attenuation 0x787b80 (1-d*d/(r*r), called from 0x78798e); cluster_get_first_light 0x787810 and cluster_get_next_light 0x787840 (pushed as direct addresses in lights_preprocess_scene).
- HCEX.pdb (DIA2Dump) marks cluster_get_first/next_light 'static function: true'.

(12) Data:
- section_info_resolved(_lights_section) is EQUAL with both the cfg_OL and production symbol maps.
- Measurements: size 1541, 1 reloc, sha 527b43a1...f71, matching the entry exactly. Flags are c0400040 on both sides. _lights_section is static @0 and the 4 public object_light_* sit @1528..1540.
- build/report.json: .data 99.7411%, unmatched 1541, equal to the entry size.
- apply_semantic_data_matches gives +1541 (2645/2645) on build/report.json and on my own mini objdiff 3.3.1 report (sha1 3130e428; candidate 43/43, 12294/12294). Without the opt-in it fails closed.

(13) audit_object_admission: 9/0/2/0 -> 10/0/2/0, as disclosed.

(14) Scans: fake_match_scan finds 0 leads. /W3 and /W4 warning sets match production. The one-line /W4 count gap is a path artifact: the unpatched source compiled in a shadow tree shows the same gap.

(15) Parks, rejections and semantic_matches have no entries for the unit.

(16) 02 stacks textually with the in-flight symbols patches from rasterizer_xbox_debug, vehicles, weapons and weather_particle_systems.

(17) Precedent originals read: 05255584, ae12a3c1, 84414a1a, 5019c186, 3bfde7bd, the race doc ('established CTF policy'), the round-1 owner_queue section 4 / RULING Q1, and the 20260903 / 20260829 / marker_cluster ledgers.

## issues
No blocking issues. The precedents are applied within their original conditions, not stretched.

P1 (static per cachebeta publics; zero credit; the object may stay blocked):
- 05255584 made the hs_runtime converters static while that object stayed blocked. The 2026-08-31 marker_cluster policy and the units / ui_widget entries follow the same rule.
- Every stated condition holds. The four helpers are absent from publics, have no outside reference, get in-place rows, the regen touches only this unit, all 43 functions stay exact with unchanged bytes, and the audit rows match.
- The earlier 'external' claims (3bfde7bd / the 20260903 reconciliation, and the 20260829 light_attenuation ledger) read csplit's default class for a _code_ placeholder. 3bfde7bd only renamed _code_00128a10. That is not independent January evidence.
- I found extra first-party corroboration in the /Od build: its call pattern (direct call = static, thunk = public) shows all four helpers as static there too.

P2 (allow_incomplete_unit, single-section '$'-literal data entry):
- Precedents: leaf_map, editor_flying_camera, objects, and race/CTF.
- The entry is a single symbol with pinned measurements. It covers the sole unmatched data, and the verifier re-proves it on every report.
- The round-1 owner queue section 4 itself lists 01+02 (+ optional 03_NO_ALT) as the landing on the NO path, independent of Q1. So this does not pre-empt the owner. Adding no rejection entry matches the other owner-held units already in the review queue.

The should_render_lights() calls are /Od-attested. They are needed for January's static to be emitted, and they remove a hand expansion (rule 13). They are not decoration.

Non-blocking notes for the integrator:
- Land 01 and 02 together, then run the csplit regen, then 03_NO_ALT.
- If Q1 later gets a YES, swap 03_NO_ALT for 03_YES and add 04_YES.
- The added static prototype is byte-inert (strip test). It is fine under rule 9.
- Optional: add a one-line correction note to the 20260903 and 20260829 ledgers, which call these helpers external.
- Evidence: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_r2w2_object_lights\REVIEW.md and LEDGER.md.
- No amended patch is needed. Land the worker's patches as they are: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_object_lights\patches\01_object_lights_static_helpers.patch, 02_object_lights_symbols_json.patch and 03_NO_ALT_object_lights_semantic_data_entry_incomplete.patch.
