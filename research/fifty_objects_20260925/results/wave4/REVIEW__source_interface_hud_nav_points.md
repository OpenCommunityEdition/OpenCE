# REVIEW source/interface/hud_nav_points

approve: True

## per_object
[
 {
  "unit": "source/interface/hud_nav_points",
  "approve": true,
  "issues": "Approve P4 only atomically with P1+P2 (+P3/01RS, P5, P6, P7). Byte-inert apart from the new SELECT_ANY _check_stack_buffer, which is identical to January's hud_draw copy and emitted by the strictly exact _hud_update_nav_point_local_player. Park sha unchanged. Zero credit; the object is NOT admission-ready: besides the park it has pre-existing law (g) blockers (consumer-local prototypes for objects.h/game_engine.h/units.h functions, TU-local opaque-span struct views hud_globals_definition and hud_nav_object_datum, float z_offset/vertical_offset)."
 }
]

## checks
All checks were independent; notes and evidence are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_nav_points\ (LEDGER.md, REVIEW.md). Tree 16542e46, clean before and after.
(1) git apply --check on 16542e46: P4 alone, P1 and P2 each clean; the full set P1+P2+01RS+P4+P5+P6+P7 is clean.
(2) Rebuilt the candidate from `git show HEAD` blobs plus `patch -p1` P1/P2/P4. The text equals the worker's cand/interface/{hud_nav_points.c, hud_draw.h, hud_draw.c}.
(3) Harness control: the production TU with its include redirected to a verbatim header copy shows 0 non-debug differences.
(4) gate.py --all on the candidate: 31 EXACT / 1 residual (_custom_render_nav_point), the same rows as production.
(5) Own strict comparer (fullcmp.py: raw bytes, relocs, flags, COMDAT selection, every symbol including UNDEF), production vs candidate: the only difference is a new _check_stack_buffer (.text 48 B, SELECT_ANY, external). The worker's h2_out object is identical to mine.
(6) Park: the normalized sha of _custom_render_nav_point is ad77c226..., equal to the parked.json base. The reloc-masked diff against January is still only +0x58a..+0x591. The inlined sentinel tail (+0x5bb to the end) is byte-identical to January.
(7) The new COMDAT is section_infos_equal to January's build/split hud_draw _check_stack_buffer (48 B, 0 relocs, c7049c31).
(8) object_audit: the production and candidate reports differ only by `+.text _check_stack_buffer 48`. All 46 January symbols match. FAIL(1) is the pre-existing park.
(9) surplus_identity (slug copy that takes a candidate obj): 5 candidate-only code COMDATs, all IDENTICAL to January's selected copies.
(10) provider_link: against the production hud_draw it fails with LNK2005 (the expected control). Against HEAD+P1+P2 hud_draw.obj (my own compile: 22/0/1 rows, only change NODUP->ANY) all 21 surplus pass. A 5-object packet link (draw, nav, and the worker's hud_unit/hud_weapon/motion_sensor) in 3 orders gives 0 LNK2005/1169.
(11) pdb_storage: 0 disagreements. _check_stack_buffer and _get_return_eip are PUBLIC in cachebeta.
(12) /W3 /Zs: the warning multisets are identical.
(13) fake_match_scan: 0 leads.
(14) Data: every January data/rdata/bss section is ok, and raw-identical to production.
(15) Law (i) lab: with the parked caller's macro use removed, the COMDAT is still emitted and identical, and _hud_update_nav_point_local_player stays EXACT. The emission therefore comes from a strictly exact caller.
(16) STRIP TEST: a plain expansion with `short corrupt_index` at function top and no macro block is EXACT and whole-object identical. The block is byte-inert.
(17) /Od build: odbuild str finds neither sentinel string (sanity: `file hud_nav_points.c` finds 5 fns), so no /Od frame can contradict the block. HCEX has no check_stack_buffer.
(18) Consumer grep (recursive, all spellings, including spaced dirs): 23 TUs, matching the worker's list.
(19) Own shadow sweep of all 23 consumers with the full packet: gate rows identical everywhere. With $Lnnnn label numbers normalised, raw bytes and relocs are identical in every section. The only changes are the expected _check_stack_buffer NODUP->ANY in hud_draw and +ANY in hud_nav_points, hud_unit, hud_weapon and motion_sensor. models with P7: 0 differences.
(20) Atlas 7eacac85 read directly: `_get_return_eip i hud_draw.obj` at 0xcff20, with a 0x40 gap before _get_sprite_clip_rect.

## issues
P4 is APPROVED only as part of the atomic packet P1+P2+(P3 or 01RS)+P4+P5+P6+P7. It cannot land alone: it needs P1's macro and STACK_BUFFER_LENGTH to compile, and it needs P2's ANY hud_draw copy to link (LNK2005 against the production NODUP copy, reproduced in both orders). If the owner rejects P1, or picks the P-A focused header, P4 is void and must be regenerated.

The verdict label overstates the result. P4 is zero-credit hygiene: it removes one law (g) blocker class, two hand-expanded check_stack_buffer copies with 4 invented labels/gotos. It does NOT make hud_nav_points admission-ready apart from the park. The lane log line "only _custom_render_nav_point remains" misses pre-existing law (g) blockers that P4 does not touch:
- Consumer-local prototypes of other files' functions that have owner headers: object_get_bounding_sphere (objects.h), game_engine_render_nav_points (game_engine.h), unit_get_head_position (units.h). Also object_try_and_get_and_verify_type, game_engine_get_goal_position and `extern struct hud_globals_definition *hud_globals`.
- TU-local struct views with opaque spans: `struct hud_globals_definition { messaging; waypoint; byte unknown1BC[0x294]; }`, `struct hud_nav_object_datum { byte unused[0xB6]; word damage_flags; }` and a local `struct scenario_cutscene_flag`.
- `float` instead of `real`: hud_nav_point_datum.z_offset and several `float vertical_offset` parameters.
These should be recorded as open hud_nav_points admission blockers.

Advisory for P1, not a P4 defect: the Sept-2001 atlas flags `_get_return_eip i hud_draw.obj` (a header inline in 2001), but P1 keeps it a plain naked hud_draw.c function. This is byte-inert and earns no credit. check_stack_buffer is absent from the Sept-2001 publics but PUBLIC in January, which is a storage change between the builds, not a contradiction.

The macro name match_assert_stack_frame is descriptive (Bungie's name is lost) and follows the match_assert_valid_*(file, line, ...) convention.

The integrator should still run a full build and sweep of all 23 hud_draw.h consumers and the parks on the live tree, together with any other wave-4 header packets. I found no defect in the P4 diff itself, so no amended patch is needed.
