# Review: rasterizer_xbox_profile (worker verdict OBJECT_COMPLETE_CANDIDATE)

**approve = false.** The worker's measurements all reproduce. The patches are clean and byte-inert, and all three items
in the old rejection are resolved. But the object's own source still has three problems, and the brief's section-8
standard blocks admission for them: invented aggregates, an authentic January bug, and volatile. Each one needs an
owner ruling or new first-party evidence.

Recommendation:

- Land patches **01+02, 03, 06** and the optional **04, 05**, plus my **08**, at zero credit. They are verified
  byte-inert ownership fixes.
- Replace **07** with my **07R**, which rewrites the rejection. Do not flip the object to Matching, and do not add the
  data entry yet.

Evidence and ledger: `LEDGER.md` in this folder (V1-V7).

## What I re-ran (all reproduce the worker)

| Check | Result |
|---|---|
| `git apply --check` | 01..07 each alone OK (04 also applies alone), full series OK. Post-images equal `candidate_files/` byte for byte. |
| gate (patched internal.h on /I) | 16/16 EXACT |
| object_audit | PASS: 67 January symbols, 0 differ; surplus = 7 literals + 3 D3D tables (+ .drectve) |
| surplus_identity / provider_link | 0 candidate-only code COMDATs; 10/10 PASS in both orders |
| pdb_storage | 0 disagreements |
| data entry | 132 B / 29 relocations / sha b577924a..., equal to January; report.json .data 68.686874 (known '$' defect) |
| Consumer sweep | My own harness `mini_shadow.py` compiles from the shadow root with production flags and compares every section plus the symbol table against build/base. Pristine control: 10/10 identical. Patched (01..06): **10/10 identical**. That covers every includer of every changed file: the 8 consumers of internal.h, plus profile and detail_objects. |
| /W3 warnings | identical warning multisets in all edited TUs |
| fake_match_scan | 0 leads |

Resolved rejection items:

- **SDK tables.** D3D8.h:325-367 defines them as unconditional `extern const __declspec(selectany)`. January's profile
  object imports `_D3DDevice_InsertCallback@12` and `QueryPerformance*`, so it included D3D8.h and emitted the same
  tables. This is authentic topology, and it meets the collision_debug standard.
- **Window extern.** The owner is now rasterizer_xbox_internal.h, the backend header of the January COMMON owner
  rasterizer_xbox.c. Blast radius is zero.
- **API pairs.** detail_objects now uses rasterizer_xbox.h.
- **Parameter order (04).** The /Od order (condition, profile, message) is attested at 0x8004c0.

## B1: four invented file-static aggregates (section 8(g) blocker)

The source groups the TU's private state into four structs:

- `rasterizer_profile_globals`, with members `pad02` and `pad06`;
- `rasterizer_profile_elapsed_state`;
- `rasterizer_profile_frame_state`, with `pad12` and `pad16`;
- `rasterizer_profile_state`, with `reserved06[6]`.

Neither HCEX (no profile statics) nor the /Od build (no symbols) attests them. The measured evidence:

- **Layout probe (`probe_loose_layout.c`).** VC7 /O2 places *every* file-scope static, shorts included, at 4-byte
  alignment. Loose statics give frequency@0, shorts@8/@0xc, names@0x10 in .data, and 4-byte strides for
  callback_index, last_callback_index, enable, flags and errors in .bss. So the pad members only spell out padding the
  compiler inserts anyway. That is the fog-O2 signature of an invented aggregate.
- **Lab de-aggregation L1 (`make_loose.py`, `compare_resolved.py`).** The comparison resolves relocations
  name-independently. All four structs become 14 loose statics in the same order, with volatile unchanged.
  - Every code byte is identical in all 16 functions, and .data is resolved-equal.
  - Every .bss offset equals January's except `error_count`: 0x45c against 0x460.
  - So the aggregates contribute nothing except that one slot.
- **Later /Od build.** `elapsed_times` (e0e9d0) sits before `callback_start_times` (e0eac8). That contradicts
  `elapsed_state`'s fixed member order.
- **The 0x45c slot.** L2 tries `error_count` as a static local of `rasterizer_profile_check`; it still lands at 0x45c.
  January has an unreferenced 4-byte object at +0x45c, and no natural spelling reproduces it. Today it is hidden inside
  `reserved06[6]`, which is an opaque span.

**Why I did not produce an amended patch:** a filler-free exact de-aggregation is not available. The de-aggregation also
needs new descriptive symbols.json names, which is the owner-held class (rasterizer_xbox .bss names; the 20260915
env_fog/models ruling).

## B2: authentic January bug, pending the owner's original-bug ruling

In `rasterizer_profile_check`, the NONE branch passes `(profile, message)` to `"### PROFILE: %s -- tell Bernie!"`.
`%s` therefore consumes the sign-extended NONE value, which crashes on that rare path. Both January's bytes and /Od
0x8004c0 prove it.

This is the same class as `_hs_parse_set` ("shorts passed to %s") in the pending original-bug packet
(claude_structural_owner_packet_20260923.md section 4). It is also the same class as the held king, glow, fpw-B and
dead_camera items. The brief says: "report, don't land".

The comment also lacked the methodology marker. Patch 08 adds it; the object is identical after the change.

## B3: volatile, disclosed as owner risk

The three qualifiers are load-bearing: with all three stripped the object drops to 12/16, as I independently re-ran.
First-party evidence cannot attest volatile (/Od reloads everything anyway). The rationale is also only partial:
`rasterizer_profile_frame_callback` is an asynchronous D3D callback too, yet its arrays are not volatile, and adding
volatile there breaks frame_end and frame_callback. The earlier 20260903 and 20260906 reviews accepted the qualifiers.
Brief section 2 lists "volatile/aliasing tricks" as fake matching, so admission needs the owner to confirm them.

## Minor

- The two `(void *)` casts in `csmemset` are inert decoration: stripped, the object stays 16/16. Optional cleanup.
- Out of scope: the already-Matching rasterizer_xbox_detail_objects still carries a caller-local
  `struct detail_objects_debug_options` view of `rasterizer_debug_options`, and a `bss_0045e904` name behind a
  `#define`. Both are section-8(g) and house-rule-8 debt in an object that is already admitted.

## Production changes (integrator)

1. **Apply, in order:** 01+02 (inseparable), 03, 04 (optional), 05 (optional), 06 from
   `scratch/w/rasterizer_xbox_profile/patches/`, then `scratch/w/review3_rasterizer_xbox_profile/patches/08_*.patch`.
   Everything is verified section- and symbol-identical, so there is zero credit.
   - Shared header 01 consumers, for the full sweep: xbox_texture_cache, rasterizer, rasterizer_transparent_geometry,
     rasterizer_xbox, rasterizer_xbox_active_camouflage, rasterizer_xbox_environment, rasterizer_xbox_models and
     rasterizer_xbox_transparent_geometry, plus profile through 03.
2. **Replace 07 with `07R_config_profile_rejection_update.patch`.** The full copy is `config/object_admission_rejections.json`.
   The unit stays NonMatching, and there is no semantic data entry yet.
   - 07R points its evidence at `research/.../w/review3_rasterizer_xbox_profile/REVIEW.md`, so run curate.py first.
3. **Owner questions:**
   - (a) De-aggregate into loose statics with descriptive names? The L1 shape is exact except the +0x45c slot. Then how
     should the unattested 4-byte object at +0x45c be represented?
   - (b) Admit the BUG-marked NONE-branch varargs defect?
   - (c) Confirm the three volatile qualifiers?

## Incidents (mine, both repaired)

- **Deleted worktree pointer.** A broken `&&` chain ran `rm -rf .git` in the worktree root and deleted the worktree
  pointer file. I restored it in the exact sibling format and verified `claude/fifty-objects-20260925` @ 954eebd2. The
  bare-repo admin data was untouched. Any parallel worker's git command during that window would have failed.
- **hs.c line endings.** `tools/campaign/volatile_scan.py --help` (the script has no argparse) rewrote `source/hs/hs.c`
  with LF endings. I restored it byte-exact from its own backup; `git diff` is clean.
