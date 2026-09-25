# source/hs/hs_runtime::_render_debug_trigger_volumes (1088 B)

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_render_debug_trigger_volumes"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\cand.c",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\cand.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\production_crlf.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\park_retire.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\park_retire_crlf.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hs_runtime__render_debug_trigger_volumes\\compose.obj"
]

## production_changes
All paths are under C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime__render_debug_trigger_volumes\ (':' is illegal in NTFS paths, so the slug is sanitized).

(1) production.patch (LF, matches the index blob) or production_crlf.patch (CRLF working tree): source/hs/hs_runtime.c. It replaces only the render_debug_trigger_volumes body, 2 hunks at lines 1004-1137. Both variants pass `git apply --check` at f6d00a8c. GNU patch simulation reproduces cand.c byte for byte from both the LF HEAD blob and the CRLF working tree, and the patched CRLF file gates EXACT.

(2) park_retire.patch (or park_retire_crlf.patch): config/parked.json. It removes the entry {unit source/hs/hs_runtime, function _render_debug_trigger_volumes, class unclassified} by deleting lines 1338-1348 in place (no re-serialisation). It passes `git apply --check` and the result is valid JSON.

No header, symbols.json or config status change. hs_runtime.c stays NonMatching.

New candidate-only emission: `_point_from_line3d`, an external SELECT_ANY .text COMDAT of 48 B. It is admitted under owner ruling 5 (2026-09-21, the all-inlined header-inline class). All five conditions hold:
- genuine real_math.h __inline;
- byte-identical to January's selected copy in action_charge.obj (sha 9b763841f8519177);
- strict-exact caller;
- zero regressions in the unit sweep;
- no hand expansion.

Integrator steps:
- run a full ninja and the stable sweep;
- after the reserved Codex real_math packet lands, re-run surplus_cand.py and provider_link.py for hs_runtime.

The parallel storage ADMISSION patch (scratch/w/hs_runtime_storage/hs_runtime_converter_storage.patch) touches different hunks. Applying the two in either order gives byte-identical files.

## evidence
Baseline gate (unmodified file): 64 exact / 2 residual. The residuals are begin_random and trigger_volumes, both [sha].

Final cand.c gate: 65 exact / 1 residual / 0 unwritten. _render_debug_trigger_volumes is EXACT: 1088 B, 34 relocations, normalized sha 5789a872498d7ef2, equal to January. begin_random's bytes are unchanged.

It was exact on the first compile of the task route (c1). Further measurements:
- Renaming points to corner (the /Od RTC name) is byte-inert.
- Seven declaration orders all stay EXACT, including the full /Od frame order and color declared in the else block. Declaration placement is therefore inert.
- Computing axis before side is also EXACT (inert).
- Strip test: hoisting the three point_from_line3d tail calls out of the two arms (single_tail.c) is NOT exact. alndiff shows 342/342 instructions, 53 hunks and 97 differing frame-slot instructions. The duplicated tail is therefore load-bearing in January's bytes, and the /Od build shows it as source text: double attestation.

/Od facts (the aligned-frame function at 0x606c80..0x607462, od_trigger_volumes.txt):
- fn 0x42e0d0 decodes to point_from_line3d(p, v, t, result).
- It is called 8 times: 1.0 / 1.0 / 1.0 / -1.0 in each arm (constants at 0x93dd5c and 0x9401b4) and 0.5 for center (0x93dd50).
- The RTC descriptor at 0x607464 names matrix, local_extent, world_extent, corner, sides, color, center, ray, result.
- HCEX_Release.pdb lists matrix, local_extent, world_extent, sides[2], result, ray.

Admission battery on cand.obj:
- object_audit: the function is ok. The only new surplus is +.text _point_from_line3d (48 B). FAIL(13) is all pre-existing: begin_random plus 12 converter storage rows.
- surplus_cand.py: _point_from_line3d is NEW and IDENTICAL to source/ai/action_charge.obj.
- provider_link --baseline: PASS in both orders.
- nodup_census: 24 definers, 0 NODUP.
- definers.py: existing definers span config index 37 to 470 and hs_runtime is 283, so the selected copy cannot change in either link direction.
- cachebeta publics include _point_from_line3d (external, which agrees).
- fnsweep.py over 127 January-owned sections: gain 1, regression 0, changed-vs-base 1. My baseline compile equals the build/base production object.
- /W3 /WX: no warnings.

Composed with the storage patch: gate 65/1, object_audit FAIL(1) (begin_random only, all 129 January symbols match), provider_link PASS.

Full ledger: LEDGER.md. Manifest: MANIFEST.md.

## blockers
The function is closed; the object stays incomplete for two reasons:
(a) _hs_evaluate_begin_random (528 B) is a 4-ModRM-byte register tie. It was retired by w1, w3c and the round-2 triage and was not re-probed.
(b) The 12 converter storage rows are handled by the separate claim 'source/hs/hs_runtime (storage: 12 converters)'. That patch composes cleanly with mine.

The COMDAT byte-identity must be re-checked after the reserved Codex math/real_math packet lands. This route does not edit real_math.h.

## reopen_criteria
Object completion needs begin_random, which reopens only on a same-compiler donor or a January local record for the loop-invariant sign-extension register choice. Re-verify the _point_from_line3d COMDAT identity after the Codex real_math packet lands.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime__render_debug_trigger_volumes\MANIFEST.md

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hs_runtime__render_debug_trigger_volumes\LEDGER.md
