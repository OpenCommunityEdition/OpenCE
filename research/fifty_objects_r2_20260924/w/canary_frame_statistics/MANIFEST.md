# MANIFEST - declaration-count canary research (r2w2-canary), LAB ONLY

Base/tip: 05255584 (clean). No tracked file edited. Claim: source/rasterizer/rasterizer_frame_statistics (r2w2-canary).
Verdict: RESEARCH_ONLY. math/real_math is RESERVED for the Codex packet, so the packet below is delivered
measured and ready, not proposed for landing. It also needs an owner ruling on which duplicate copy to drop (see F9).

## Mechanism (new facts, all lab-measured; LEDGER.md F1-F7)
- Period is exactly 64 (not 63). draw is EXACT at +0/+64/+128 names, at the top of the TU and after the globals.
  Every other canary tested (bitmap_copy, units, alpha_bleed) repeats with period 64 too.
- What gets counted is C1's symbol records: declared names (tags, members, typedefs, externs, statics, params,
  locals), string literals (deduplicated per TU), compiler labels (one per `if`, and `&&` labels too), and one record
  per distinct FILE opened. Numeric constants, macros, extra `{}` blocks and re-including a guarded header cost 0.
- draw depends only on the net record count before its .c globals block (mod 64). Between the top of the TU and the
  verify typedefs (24 scanned positions in the SDK, cseries, real_math.h, the rasterizer headers and the .c) every
  position gives the same fingerprint. Header externs and prototypes are not relevant symbols for draw.
- Other canaries are position-sensitive. Their relevant symbols include the parameters and locals of helpers they
  inline from headers. That is why rlr (+1) breaks units even though a +1 at the top of units' TU is accepted.

## Per-construct evidence (packet A)
| construct | file | evidence | net in frame_statistics TU |
|---|---|---|---|
| real_local_random named local | real_math.h | /Od + January COMDAT a8ad3cb3 (effects), r1 breakable audit | +1 |
| valid_real_plane3d __inline (prototype at l.672 removed) | real_math.h | January matrix_math COMDAT 5a34cbcd; collisions /Od one call; Lane B ruling that it belongs in real_math.h | +1 |
| plain definition removed | matrix_math.c | the header inline becomes the provider; matrix_math stays 35/35 | n/a |
| loop-site helper call, `position` at function scope | collisions.c | /Od 0x7A5833, 0x7A52DC (function scope) | n/a |
| stack_walk_disregard_symbol_names duplicate removed | cseries_windows.h | verbatim duplicate of cseries.h:240. Both copies are campaign-authored | -2 |
| bitmap_delete duplicate removed | bitmaps.h | verbatim duplicate of bitmaps_internal.h:24. Both copies are campaign-authored | 0 (not in this TU) |
| optional: nonuniform_cubic_spline -> glow.c body | real_math.h | glow _nonuniform_cubic_spline 192 B strict EXACT | 0 |

## Target vs candidate (whole board, 447 units, rerun_packet.py)
- PA / PAS / PAE: GAINED physics/collisions::_collision_move_point 4752 B (4752/226, sha 8b2f2900.. == January).
  LOST 0.
- PAI (the ownership-principled bitmap_delete choice) additionally LOSES units::_unit_preprocess_node_orientations
  (1920 B).
- Non-exact drift, all expected:
  - New surplus COMDATs, each byte-identical to January's selected copy: collisions and render_cameras
    `_valid_real_plane3d` (64/1, 5a34cbcd), and effects `_real_local_random_range` (32/2, 7ed2e50e; PAE only).
  - breakable and weather `_real_local_random` surplus moves to the January form a8ad3cb3.
  - The PARKED __rasterizer_model_draw moves 89b0d7ea -> cbfa8585 at the same 5168/348. Its parked.json measurement
    must be re-baselined.
- Data: every data section in every unit is identical as a multiset; only COMDAT indices shift.
- Symbols and storage: the object_audit results for collisions (PASS), matrix_math, breakable and effects are identical
  to their base audits. Their failures predate the packet.
- Provider links (both orders): the packet pairs PASS. Pairing collisions with the BASE matrix_math, or breakable with
  the BASE effects, FAILs with LNK2005, so the matrix_math.c and effects.c halves are mandatory.
- CL /Zs /W3 census: identical to base (177 lines, no new C4013).

## House-rule review
- No fillers or dummies in the packet; dummies were used only as the oracle.
- Genuine owner-header declarations (Lane B ruling for valid_real_plane3d), no hand expansion, /Od-attested call and
  scope.
- Duplicate removal is accepted cleanup (100K-lane precedent, r1 reviewer reopen criterion (a)).
- OPEN ISSUE: the choice of which duplicate copy to drop is decided by the count. The measured alternatives are in F9.

## Credit
Nothing is credited now; this is a lab packet. Potential credit after Codex lands and the owner rules:
+4752 B strict (_collision_move_point), plus the breakable B1 surplus becoming January-identical.

## Held / reopen criteria
- The packet must be re-solved after the Codex real_math.h packet lands. Run `python rerun_packet.py --refresh` in this
  slug. If Codex's net change is not 0 in a canary TU, rerun grid.py or window.py for the lost canaries
  (canaries.txt, consumer_sweep_list.txt).
- Owner ruling needed on the duplicate copy (bitmaps.h vs bitmaps_internal.h; cseries_windows.h vs cseries.h), or
  first-party evidence of January's header placement.
- The fully-genuine route is to recover January's missing real_math.h content. The assert anchors show about 114
  more January lines between Jan 880 and Jan 1508, which needs about +17 records (mod 64). Not recoverable from
  current evidence.
