# MANIFEST: source/hs/hs_runtime :: _render_debug_trigger_volumes (1,088 B), round 2 worker r2w1

- Base: canonical f6d00a8c (worktree HEAD b7c1b7d9 = baseline commit; `git diff f6d00a8c HEAD -- source/ config/` is empty).
- Tip: no commit (worker lane). The production change is `production.patch` (LF, matches the index) or `production_crlf.patch`
  (CRLF working tree), plus `park_retire.patch` / `park_retire_crlf.patch`. All four pass `git apply --check` at f6d00a8c.
  GNU `patch -p1` simulation reproduces `cand.c` byte for byte, from the LF HEAD blob and from the CRLF working tree.
- Final candidate: `cand.c` (= `color_else.c`), `cand.obj`, and `cand_gate.txt`.
- The object stays incomplete. `_hs_evaluate_begin_random` is a retired 4-byte register tie. The 12 converter storage rows are
  handled by the separate ADMISSION claim `source/hs/hs_runtime (storage: 12 converters)`, patch
  `scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch`. The two patches are hunk-disjoint (see Composition).

## Source evidence per changed construct

| construct | first-party evidence | January-byte attestation |
| --- | --- | --- |
| points[1..3] tail written in BOTH side arms | /Od fn 0x606c80..0x607462: side arm calls 0x42e0d0 at 0x607053/0x607098/0x6070dd; else arm at 0x607218/0x60725d/0x6072a2 (od_trigger_volumes.txt). /Od does not cross-jump, so this is source text. | Strip test: `single_tail.c` hoists the three calls below the if/else. It stays residual: 342/342 insns, 53 hunks, 97 differing target insns, all frame-slot renames (single_tail_aln.txt). The duplication sets VC7's frame ranking. |
| real `point_from_line3d` calls (t = 1, 1, 1, -1; center 0.5) | fn 0x42e0d0 = `result->x = v->i*t + p->x ...; return result` (odbuild fn). Floats: 0x93dd5c=1.0, 0x9401b4=-1.0, 0x93dd50=0.5. Argument pushes give (p, v, t, result) exactly as written. | EXACT. January has no 1.0/-1.0 relocation, and VC7 folds them. Center's 3 x `__real@3f000000` relocs are unchanged. Rule 6 forbids the hand-expanded alternative (throughput w7). |
| `real_vector3d sides[2] = { 0 }` | HCEX_Release.pdb `union real_vector3d[0x2] sides`. /Od zero-inits 24 B at -0xd0 (RTC `sides` -208/24). | January's contiguous 24-byte zero-init: 5 register stores + 1 immediate (w3c/w1 t1). |
| names matrix, local_extent, world_extent, ray, result | HCEX_Release.pdb locals (DIA2Dump -sym) and /Od RTC descriptor 0x607464 | byte-inert |
| `corner[4]` (was points) | /Od RTC descriptor: `corner` -176/48 (HCEX does not home it) | byte-inert (c1 vs c2: only .debug$S differs) |
| side = edge_index % 2 before axis = edge_index / 2 | /Od 0x606ece..0x606eef | inert (axis_first.c also EXACT) |
| if/else pair of render_debug_string_at_point (yellow / white) | /Od 0x6073f7..0x607436: two calls with 0xa1c244 / 0xa1c22c | EXACT (January has 1 reloc, cross-jumped) |
| color declared in the else block with its initialiser | /Od frame order ... sides, side, axis, color; house rule 17 | inert (color_edge, od_order, etc. are all EXACT) |
| declaration order (volume_index before scenario; matrix first in the volume block) | /Od frame: matrix, volume_index, scenario, volume, local_extent, world_extent, edge_index, corner, sides, side, axis, color, center, ray, result (RTC order = declaration order) | inert (7 orders measured, all EXACT) |

Not reproduced, recorded only:
- /Od `volume = NULL` outer initialisation (`mov [ebp-0x50],0`). It is a dead store at /O2 with no January-byte attestation.
- A bare block putting center/ray/result after the edge-loop locals. It is byte-inert (bareblock.c EXACT), but under C89 it
  would be an invented block. The later /Od build's text order cannot tell such a block from a mid-block declaration.
- /Od assert line 0x214 vs January 0x213: the later revision has one extra line above this point. January's 0x213 is kept.

## Target vs candidate

| function | January (split) | production (build/base) | candidate cand.obj | gate |
| --- | --- | --- | --- | --- |
| _render_debug_trigger_volumes | 1088 padded (1080 before trailing pad) / 34 relocs / sha 5789a872498d7ef2 | 1088 / 34 / 66af597069fc087b | 1088 / 34 / 5789a872498d7ef2 | EXACT |
| _hs_evaluate_begin_random | 528 / 30 / fce47f8f | 528 / 30 / c10e869c | 528 / 30 / c10e869c (unchanged) | residual (tie, retired) |
| _point_from_line3d (surplus) | not defined or referenced by January hs_runtime.obj; January selected copy is action_charge.obj: 48 / 0 / 9b763841f8519177 | not emitted | 48 / 0 / 9b763841f8519177 | IDENTICAL to selected copy |

- Unit gate: base 64/2/0 -> candidate **65/1/0** (cand_gate.txt). fnsweep.py over all 127 January-owned sections: gain 1,
  regression 0, changed-vs-base 1 (only this function). My base.obj equals build/base production (changed 0).
- Data sections: object_audit reports every January-owned .data/.rdata/.bss section `ok`. No data change: the rdata
  literals are the same set, and the new COMDAT is code only.

## Symbols, storage, surplus, links

- object_audit cand.obj: FAIL (13) = begin_random section (pre-existing) + 12 converter storage rows (pre-existing).
  The composed candidate with the storage patch gives FAIL (1) = begin_random only (compose_audit.txt).
- New candidate-only symbol: `_point_from_line3d`, external (2), .text COMDAT SELECT_ANY. It is a cachebeta PDB public
  (scratch/tools/cachebeta_publics.txt line 6312), so storage agrees. No other symbol changes except `$L` labels.
- surplus_cand.py (surplus_identity logic on the candidate object): 5 candidate-only code COMDATs, 0 not identical.
  `_point_from_line3d` NEW vs source/ai/action_charge.obj IDENTICAL.
- provider_link cand.obj --baseline=base.obj: `_point_from_line3d` provider source/ai/action_charge PASS in both orders.
  The full run PASSes every COMDAT and literal surplus. Its only FAILs are the 12 pre-existing converter rows, and on the
  composed object the full run is PASS.
- nodup_census `_point_from_line3d`: 24 definers, 0 NODUP.
- definers.py: base definers span config index 37 (units) to 470 (action_charge, January's selected copy). hs_runtime is
  index 283, strictly inside that range, so the linker's selected copy cannot change in either link direction.
- Owner ruling 5 (2026-09-21) conditions:
  - genuine shared-header `__inline`: real_math.h `point_from_line3d`, no header edit;
  - byte-identical COMDAT: yes;
  - strict-exact caller: yes;
  - ownership/build sweep with zero regressions: unit sweep 0 regressions; no header or config change beyond the park, so
    no other object recompiles differently. The integrator must still run the full ninja + stable sweep;
  - no hand expansion: yes.

## Composition with the storage ADMISSION patch

- `production.patch` touches lines 1004-1137. The storage patch touches 466-495 and 1538-1660. Applying the two in either
  order gives byte-identical files; the second patch applies at offset -8.
- The composed file gates 65/1/0. object_audit FAIL (1) is only begin_random, all 129 January symbols match, and the full
  provider_link run PASSes.

## House-rule review

- void on its own line; explicit `return;`; `real` types; project typed macros (TAG_BLOCK_GET_ELEMENT,
  BIT_VECTOR_TEST_FLAG); enum constants in the switch.
- No decoration, casts, volatile, pragmas, dummy locals or fillers.
- The duplicated arm tail is first-party /Od text and is load-bearing in January's bytes (strip test above). It is not an
  invented duplicate branch.
- Rule 6: the helper is called, never hand-expanded. The COMDAT is admitted under ruling 5 with all conditions met.
- /W3 /WX compile: no warnings (base_w3.txt, cand_w3.txt).

## Tests run

- gate.py --all: c1, c2, od_order, od_order_vfirst, od_inner_only, od_outer_only, color_edge, color_else(=cand), bareblock,
  single_tail, axis_first, the patched CRLF file, and compose.
- object_audit, surplus_cand.py, provider_link (all and --baseline), nodup_census, pdb_storage (production),
  relocdiff (c1), alndiff (single_tail), fnsweep.py, objeq.py, git apply --check (4 patches), GNU patch simulation.

## Credit

- Credited: `_render_debug_trigger_volumes`, 1 function, 1,088 padded bytes (about 1,080 meaningful), strict exact.
  Park retirement: `{source/hs/hs_runtime, _render_debug_trigger_volumes, unclassified}` (park_retire.patch).
- Zero credit: the `_point_from_line3d` COMDAT (surplus), and the object itself (begin_random tie; storage handled
  elsewhere). No config status flip: hs_runtime.c stays NonMatching.

## Held items and integrator steps

- math/real_math is RESERVED for the Codex packet (cross_product2d). This route does not edit real_math.h. After the Codex
  packet lands, re-run `surplus_cand.py` + `provider_link.py` for hs_runtime. `_point_from_line3d` must still be IDENTICAL
  to action_charge's selected copy.
- `_hs_evaluate_begin_random`: retired tie (4 ModRM bytes; w1/w3c/triage). Reopen only on a same-compiler donor or a
  January local record for the loop-invariant sign-extension register choice.
