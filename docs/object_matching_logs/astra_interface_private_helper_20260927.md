# Private weapon-HUD resolver — strict closure, 2026-09-27

Baseline: `ecf65725781eda8c36ad5c86febaa5647d9a5df1`, canonical
`jonas/exact-pilots`. This packet gains **one Halo function, 444 meaningful /
448 padded code bytes, with zero exact losses**. No data or object-admission
credit is claimed; interface remains NonMatching despite 18/18 exact functions.

## Source correction and approval

`interface_get_weapon_hud_index` is private to interface.c. The only caller is
`interface_draw_screen`, after the definition. The complete minimal repair is:

- Make its existing definition `static`, without changing its body or types.
- Remove its public prototype from interface.h. No replacement forward
  prototype is necessary and none is added.
- Mark its existing symbols.json entry at file offset 844576 static.
- Retire its freshly exact park in the same packet.

This is the clean Variant A of the earlier fifty-objects interface packet,
not Variant B's conflicting extern/static redeclaration. The old blocker was
an exact loss in rasterizer_frame_statistics_draw. P1 (`7b51e0ba`) already
explicitly debited that function before this baseline. The owner separately
approved reopening this interface packet on 2026-09-27 **if all gates pass**;
P1 was not treated as blanket permission to lift other holds.

No unrelated profile-data, render_debug_profile or interface_splitscreen_render
linkage changes from the larger donor packet were imported. No assembly,
casts, body respellings, dummy declarations, compiler changes, or scorer changes.

## Independent provenance

Fresh DIA checks against January's cachebeta.pdb found no public symbol for
this function; the positive control interface_draw_screen was present at
RVA 0xCE4E0. January PDB SHA-256:
`8480f0c44fc7b5acba5775c02053d1a62c6794ab8d646661106985cbe46d7bc5`.

HCEX independently reports `static long interface_get_weapon_hud_index(float *)`
and `static function: true`; its parameter is `flashlight_power` (float *).
HCEX PDB SHA-256:
`f55cfe957da8079a62a26a6753f1ddbb700b067a66de0bd5db5aac182d985ff1`.
Later debug information corroborates linkage/signature, not January code.

A fresh census of 833 January and 622 rebuilt objects finds one definition,
in interface, and no undefined imports in other objects. The in-owner call
comes from interface_draw_screen at relocation +0x19, REL32, addend zero.
Source-wide references agree. After regeneration, target and candidate both
give this symbol storage class 3.

The old ledger's universal claim that storage class always controls the
floating-copy template has counterexamples. This landing relies on authentic
private linkage and this function's actual exact result, not that broad claim.

## Verification

Target and rebuilt function:

- 448 padded bytes; 444 meaningful bytes; 20 complete relocations.
- Normalized SHA-256:
  `0673f867b219cdbd399685310736ae3de8263479cb2104edd5c56f48930c6549`.
- All 18 January-owned interface functions are now strict exact.

All 19 interface.h consumers were rebuilt. There are no indirect header
consumers. Only two normalized code sections change: the new exact resolver
and already-residual rasterizer_frame_statistics_draw. The latter remains
residual; there is no new debit. Non-debug data sections/flags/extent and COMMON
are unchanged. Compiler-local $L names renumber naturally in some consumers;
no semantic owner is added or removed. The sole source-symbol storage change
is the intended private helper.

The final no-forward-prototype object was checked independently against the
initial scratch variant. W3 diagnostics do not increase; explicit W4 comparison
is 72 warnings before and after, with no C4211. All 17 inherited surplus
definitions match January's selected copies; both-order provider probes pass
(34 pair checks). These are bounded duplicate-definition probes, with expected
unresolved externals, not a whole-program link or boot claim.

Full gates:

- `ninja all_source progress build/report.json`: passes, frozen objdiff 3.3.1.
- Stable owner-section sweep: 8,252 rows, 7,639 -> 7,640 exact; +1, zero losses.
- Halo accepted ledger: 7,467 -> 7,468 / 7,574 functions;
  1,593,054 -> **1,593,498 / 1,770,166 meaningful bytes**.
- Data remains 2,588,903; complete Halo objects remain 389 / 468.
- Parks: 72 -> 71 active, zero stale/invalid.
- Admission audit: 11 -> 12 candidates, zero contradicted, one existing
  rejection, zero revocations. The new candidate is not an admission.
- Fake scan: 26 inherited leads, unchanged.
- pytest before and after: 1,161 passed, 5 skipped, 26 subtests passed.
- `git diff --check`: clean. Existing README and unrelated research preserved.

## Receipts and bounded negatives

All full receipts remain locally under
`scratch/astra_one_function_retry_20260927/interface/`: PLAN.md,
consumer_sweep.json, before/after snapshots and gate logs, final_verification.json,
provider_audit.jsonl, and diagnostic logs. The independent review is under
`scratch/astra_one_function_retry_20260927/interface_review/REPORT.md`.

An initial shadow probe accidentally retained the public prototype after an
atomic patch failure. It was inadmissible Variant B and is explicitly recorded
as such; the corrected, finally landed Variant A was recompiled and verified.

Other bounded investigations in this request gained nothing and remain scratch
evidence, not production edits: bipeds coupled helpers, multiplayer directions
with its missing error header, pause-menu getter, progress-bar, sound renderer,
and upstream pill continuity. Their existing exact-loss/held forms were not
admitted. No repeated count or decoration sweep was used to obtain this closure.

Historical sources: `research/fifty_objects_20260925/w/interface/LEDGER.md`,
its wave-1 independent review, and
`docs/object_matching_logs/interface_obj_weapon_hud_best_fuzzy_20260912.md`.
