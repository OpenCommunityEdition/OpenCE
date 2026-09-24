# review3 rasterizer_xbox_profile - adversarial review ledger

Tree 954eebd2. Reviewing worker result scratch/w/rasterizer_xbox_profile/ (verdict OBJECT_COMPLETE_CANDIDATE).
Read: WORKER_BRIEF s0-8, house rules, lane ledger, rejection entry, 20260906 shared-owner ledger, worker LEDGER.md.

## V1 - mechanical re-runs (reviewer)
- git apply --check: 01..07 each alone OK (04 also applies alone; hunks do not overlap 03), full series OK, series w/o 04 OK.
- Series applied in a scratch git tree (repo .gitattributes + autocrlf=true): post-images of profile.c, internal.h, rasterizer.c,
  detail_objects.c are byte-identical (cmp) to worker candidate_files/.
- gate (cand profile.c = 03+04, patched internal.h via --cflag /I<review>/hdr): 16/16 EXACT, 0 residual, 0 unwritten.
- object_audit cand_final.obj: PASS, 67 January symbols 0 differ; surplus = .drectve + 7 literals + 3 D3D tables.
- surplus_identity: 0 candidate-only code COMDATs. provider_link cand: 10/10 PASS, SELECTED-PROVIDER LINK: PASS.
- HCEX globals dump: NO rasterizer profile TU statics (only folded stub rasterizer_profile_enable + profile enum/debug-option
  data). /Od build has no symbols. => the 4 file-static aggregates (rasterizer_profile_globals with pad02/pad06,
  rasterizer_profile_elapsed_state, rasterizer_profile_frame_state with pad12/pad16, rasterizer_profile_state with
  reserved06[6]) have NO first-party attestation of shape or names.
## V2 - consumer sweep (reviewer's own harness mini_shadow.py; cwd = shadow root, production cflags, owner-keyed
## coff_compare.section_infos_equal + flags + symbol table vs build/base)
- Affected TUs = every includer of a changed file: internal.h consumers (grep: 8 .c, no header includes it) + profile + the
  .c files edited by 02/05/06 = 10 TUs.
- Control shadow A (pristine copy of source/): 10/10 SECTION+SYMBOL IDENTICAL to build/base (harness reproduces production).
- Shadow B (patches 01..06 applied with git apply, autocrlf): 10/10 SECTION+SYMBOL IDENTICAL to build/base, warning counts
  unchanged (rasterizer_xbox 1 before and after). No other TU can change (no other includer). Header 01 blast radius = 0.
## V3 - aggregate authenticity (section-8(g) "invented aggregates"; fog precedent memory aggregate-vs-scalar-oracles)
- January code refs into .data/.bss (dataref.py on build/split): .data +0 (i64), +8 short, +0xc short, +0x10 name table;
  .bss +0,+0x80,+0x100,+0x180,+0x268,+0x350 (i64 arrays), +0x438/+0x440 (i64), +0x448 short, +0x44c short, +0x450 short,
  +0x454 dword, +0x458 short, +0x460 short. Unreferenced: .data +0xa,+0xe (bytes 00 00); .bss +0x44a,+0x44e,+0x452,+0x45a..+0x45f.
  January .data bytes 01 00.. | ff ff 00 00 ff ff 00 00 => two short NONE values each followed by a zero halfword.
- PROBE probe_loose_layout.c (VC7 /O2 /Oy-, loose file statics, declaration order, all initialised): .data frequency 0,
  active_profile_index 8, window_index 0xc, profile_names 0x10 (shorts at 4-BYTE STRIDE); .bss callback_index 0x30,
  last_callback_index 0x34, local_enable 0x38, profile_flags 0x3c, callback_errors 0x40, error_count 0x44 (4-byte stride).
  => VC7 pads every file-scope static to 4. Loose statics reproduce January's .data layout EXACTLY and every .bss offset
  except error_count (loose: +0x45c; January +0x460 => exactly ONE unexplained 4-byte slot at +0x45c).
  => pad02/pad06 (globals) and pad12/pad16 (frame_state) are NOT required by layout: they are alignment padding that an
  aggregate had to spell out, i.e. the fog-precedent signature of an invented aggregate. elapsed_state (3 i64 arrays)
  is layout-neutral too. None of the 4 aggregates has first-party attestation (no HCEX compiland statics, /Od has no names).
- /Od (later build) .bss map (od_scan.txt): callback_index e0e8c0 (frame_begin %16 word), local_profile_enable e0e8c8,
  profile_flags e0e8cc, callback_errors e0e8d0, [e0e8d4 unreferenced], start_times e0e8d8 (init loop), elapsed_times
  e0e9d0 (init loop + query), then the 3 memset callback arrays e0eac8/e0eb48/e0ebc8. So in the later first-party build
  elapsed_times PRECEDES callback_start_times: incompatible with our struct rasterizer_profile_elapsed_state
  {callback_start_times, pushbuffer_elapsed_times, elapsed_times} (member order is fixed). /Od .data a36d40/44 frequency,
  a36d48 active, a36d4c window, a36d50 names = January layout, consistent with loose statics.
- VERDICT B1 (blocker): 4 invented file-static aggregates with fabricated pad/reserved members = section-8(g)
  "invented aggregates" (precedents: fog O2 ruling, hs_globals_external, render_debug held, biped_limp_noodle fixed).
  The natural fix (de-aggregation) needs new descriptive symbols.json names = owner-held class (rasterizer_xbox .bss
  names; 20260915 env_fog/models ruling). Not self-landable.

## V4 - other source review items
- B2 (owner-gated): rasterizer_profile_check NONE branch passes (profile, message) to "### PROFILE: %s -- tell Bernie!"
  => %s consumes the sign-extended short -1 (crash on that rare path). Target-proven (both branches push 2 varargs) and
  /Od-attested (0x8004c0 pushes both). Same class as _hs_parse_set "shorts passed to %s" in the pending original-bug
  owner packet (claude_structural_owner_packet_20260923.md s4) and the held king/glow/fpw-B/dead_camera items.
  Source comment lacks the methodology marker "BUG (preserved for exact matching): ... A corrected build should ...".
- B3 (disclosed risk, owner): 3 load-bearing volatile qualifiers; strip test fails (worker F6: 12/16); no first-party
  attestation possible (/Od is store-reload anyway). Rationale "shared with async D3D callback" is only partial: the
  frame-callback arrays are ALSO written by an async D3D callback (rasterizer_profile_frame_callback via InsertCallback)
  yet are NOT volatile (adding volatile there breaks frame_end/frame_callback). Accepted by 20260903/20260906 reviews;
  brief s2 lists "volatile/aliasing tricks" as fake matching => owner must confirm for admission.
- SDK tables (rejection item 1): RESOLVED. D3D8.h:325 `#define D3DCONST extern CONST DECLSPEC_SELECTANY`, tables at
  327/342/367 unconditional; January profile split imports _D3DDevice_InsertCallback@12 + QueryPerformance* => January's
  TU included D3D8.h and emitted the same selectany tables (authentic topology). Identical + pair-link PASS both orders.
## V5 - fixability lab (reviewer; lab names only)
- compare_resolved.py: name-independent comparison (every .data/.bss relocation resolved to base+offset in both objects).
  Validated: worker candidate 16/16 resolved-equal, .data equal.
- L1 (make_loose.py): the 4 aggregates -> 14 loose file statics in the same declaration order, no pad/reserved fields,
  volatile kept on the same 3 objects. gate 16/16 (gate normalisation is name-lenient here, do not rely on it);
  resolved: 14/16, ALL code bytes identical (sha eq in all 16); .data resolved-equal (layout 0/8/0xc/0x10 = January);
  .bss every offset = January EXCEPT error_count @0x45c (January 0x460) => size 1118 vs 1122. The only differing relocs
  are the error_count address in check + frame_begin. => the aggregates buy exactly nothing except the 4-byte slot.
- L2 (L1 + error_count as `static short error_count = 0;` local of rasterizer_profile_check): lands at 0x45c too
  (gate 14/16 by name; resolved 14/16 same single-address diff). => no natural spelling fills January's 4-byte slot at
  +0x45c; in the current source it hides inside reserved06[6] (2 B alignment + 4 B unattested object). A filler-free
  exact de-aggregation is NOT available from current evidence; closing it needs either an attested unreferenced
  initialised static at +0x45c or an owner ruling. /Od has e0e8d4 unreferenced but that is explainable as 8-alignment
  padding before start_times there, so it does not attest a variable.
## INCIDENTS (reviewer-caused, both repaired; disclose to integrator)
1. My first shadow-tree command chained `cp .gitattributes ... && cd shB && git init ...; rm -rf .git`. The cp failed
   (repo has no .gitattributes), so the cd never ran and `rm -rf .git` deleted the WORKTREE POINTER FILE
   C:\halo-worktrees\claude-fifty-objects-20260925\.git. Nothing else was lost: the admin dir
   halo-campaign.git/worktrees/claude-fifty-objects-20260925 (HEAD ref: claude/fifty-objects-20260925, index, logs,
   refs) and all objects live in the bare repo. RESTORED with the exact sibling format (one LF-terminated line
   `gitdir: C:/Users/isabe/Documents/Codex/2026-09-20/files-pasted-by-the-user-halo/work/halo-campaign.git/worktrees/claude-fifty-objects-20260925`);
   verified `git rev-parse` -> claude/fifty-objects-20260925 @ 954eebd2. Between the deletion and the restore, any git
   command run by a parallel worker in this worktree (e.g. git apply --check) would have failed "not a git repository".
2. `python tools/campaign/volatile_scan.py --help` (the tool has no argparse) ran its hard-coded hs.c scan: it backed up
   source/hs/hs.c to scratch/hs_scan_backup.c, ran one gate baseline, found 0 sites and "restored" hs.c with LF line
   endings (content identical). RESTORED byte-exact from its own pre-run backup (CRLF; git hash 35571179 = HEAD blob);
   `git diff --quiet` clean, 0 tracked modifications. It also overwrote the ignored scratch/hs_scan_backup.c and
   scratch/volatile_scan2.json.
## V6 - remaining checks
- pdb_storage (production = candidate storage): 67 split symbols, 0 disagreements with cachebeta publics.
- data entry: data_entry.py equal=true, 132 B / 29 relocs (all resolve to image addresses) / b577924a..., flags c0400040,
  storage 3/3; cand_final.obj and shadow-B obj resolved-equal to January too. report.json .data 68.686874 (known '$').
- /W3 (CL /Zs /W3, pristine vs patched shadow): profile 12/12, rasterizer 12/12, detail_objects 12/12,
  transparent_geometry 12/12, models 13/13, environment 12/12 - identical warning multisets.
- fake_match_scan on patched profile.c, rasterizer.c, detail_objects.c: 0 leads each.
- Strip test (void *) casts in the two csmemset calls: stripped = 16/16 exact => inert decoration (advisory cleanup).
- Strip test volatile (independent re-run): 12/16 (callback 192!=272, frame_begin 320!=336, initialize 144!=112,
  query 256!=288) = worker F6 reproduced.
- /Od check param order (patch 04): od_scan 0x8004c0 [ebp+8] movzx byte (condition), [ebp+0xc] movsx word (profile),
  [ebp+0x10] dword asserted non-NULL (message); /Od frame_begin pushes message, -1, condition => (condition, profile,
  message) attested. No new block scopes anywhere in 01..06.
- Pre-existing debt spotted in already-Matching detail_objects (out of scope): caller-local
  `struct detail_objects_debug_options` view of rasterizer_debug_options and `bss_0045e904` + #define alias.
## V7 - amended deliverables (reviewer)
- patches/08_profile_none_branch_bug_comment.patch: NONE-branch comment -> methodology "BUG (preserved for exact
  matching) ... A corrected build should pass only message here." Object SECTION+SYMBOL IDENTICAL (mini_shadow);
  applies alone and after 01..07.
- patches/07R_config_profile_rejection_update.patch (+ config/object_admission_rejections.json full copy): REPLACES 07.
  Keeps the unit NonMatching; rewrites the rejection (class source-layout-incomplete, symbol _rasterizer_profile_globals)
  to record the resolved SDK/window/API items and the live blockers B1-B3; validated by
  tools.object_admission_policy.rejection_index; git apply --check OK.
## VERDICT: approve=false (B1 invented aggregates; B2 authentic varargs bug awaiting original-bug ruling; B3 volatile).
Land 01+02+03 (+04, 05 optional) + 06 + 08 + 07R at zero credit; do NOT land 07.
- (housekeeping) shA/shB shadow copies, objA/objB/objB2 and the applytree temp repo deleted after measurement (regenerable: cp -r source + git apply 01..06; mini_shadow.py). git status: 0 tracked modifications; worktree pointer present; HEAD 954eebd2.
