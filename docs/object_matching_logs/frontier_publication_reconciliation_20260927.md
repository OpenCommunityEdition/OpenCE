# Remaining-frontier publication reconciliation, 2026-09-27

Canonical baseline: `e3aea795e8ec1804ffbe4242e163b1ee1c4922b9`.
Donor reviewed: `9736deed9c45cb3d7ab527852941a6b051e1c4f3`, read-only.
Both authorized GitHub `jonas/exact-pilots` branches were at
`cea8606242047d35c68c08cf3d3b17ae0ff202c5` before this reconciliation.
The inherited README edit and untracked research directories are not part of it.

## Deliberate debug-crash command

Reconciles donor `d18a7cee65697afbd9acba3f877002e089c7ca50` under the
owner's explicit BW-P1 original-deliberate-crash ruling. Only its ten source
lines are imported; no new trigger, compiler control, assembly, or header
migration is introduced. The existing `hs.c` consumer-local declaration has
the same `void(char const *)` ABI. Its owner-header debt remains for the
separately reviewed complete declaration correction, not an ad-hoc prerequisite.

January at 0x4f14d0 emits `c7 05 00000000 d89a6700 c3`: store the address
of `chucky was here!  NULL belongs to me!!!!!` through address zero, then
return. This is the existing script command described as `crashes (for debugging).`.
The source retains the approved BUG disclosure and deliberately ignores `str`.
This site-specific approval does not generalize to accidental null writes.

Independent stock-compiler control/candidate builds on this canonical base:

- main: 93 exact / 1 residual / 1 unwritten -> 94 exact / 1 residual / 0 unwritten.
- `_main_crash`: 16 padded / 11 meaningful bytes, one relocation;
  normalized SHA-256 `abe944925d4f3b974a0bd6e1ec5523804233ecf1d23afb41bf05be96ee3ddc59`.
- Added only this function and its 42-byte literal COMDAT; all inherited
  non-debug sections, data, storage and COMMON remain unchanged.
- Stock warnings 0 -> 0; independent /W3 check 25 -> 25.
- Unchanged objdiff 3.3.1 confirms main's credited data gain of 1,796 bytes.

Canonical full build and 8,252-row stable sweep: one gain, zero losses;
7,642 strict-exact owner rows. Halo credited code becomes 1,598,253 / 1,770,166
(7,470 / 7,574 credited functions); data 2,590,699 / 3,923,451.
Objects remain 390 / 468. Parks 72 active, zero stale/invalid; admission and
26 inherited fake-scan leads unchanged. Tools: 1,161 passed, 5 skipped,
26 subtests. `git diff --check` passes. No park is retired for this formerly
unwritten function.

Local, unpublished receipts are in `scratch/astra_publish_20260927/`, including
the independent `main_review/` control/candidate objects and comparisons.
Private reference/compiler assets are not publication artifacts.

## Q11: independently verified HS data, not code

The approved PA+PC and PB tool changes are separate no-credit commits,
reconciled from `c32e53c2` and `dccd2e95`. Only the HS entry from `e3c053cd`
is appended here; all preceding canonical entries and the shell generated-name
binding remain unchanged. There is no actions entry or scorer upgrade.

Fresh independent review verified 910 full data sections, 2,207 relocations,
and 20 surplus literals against unique January and current providers in 17
units, with the 833-object target census sealed. Member coverage, section
symbol identity, COMDAT selection, complete payloads and resolved targets
are checked; missing-member, missing-surplus and wrong-provider controls fail.
The live report is rebound to a fresh run of SHA256-pinned objdiff 3.3.1
(`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`).
PB alone leaves the complete report unchanged. The HS-only entry supplies
53,122 raw data bytes plus 1,658 modeled alignment-padding bytes: **54,780
additional data credit**, no code/function/object credit. HS remains incomplete.

Production full gates after each stage preserve all 8,252 function verdicts,
72 valid parks and the admission results. Final tool suite: 1,318 passed,
5 skipped, 100 subtests. Halo data becomes 2,645,479 / 3,923,451. The separately
reviewed verifier cases exercised all 205 tests, including an isolated-run
scorer-path skip subsequently closed against the actual pinned binary.
Receipts: `scratch/astra_publish_20260927/q11_review/` and `q11.*` logs.
This review is specific to the approved HS entry, not blanket permission for
future extent-model entries or surplus definitions.

## Scope boundary

B3 and INC-3 are not imported by this packet. B3's two new incompatible-pointer
warnings remain under the owner's requested investigation. Other donor source,
storage, data-verifier and admission packets require their own current-base
reconciliation; donor-relative credits are not automatically additive.

## Final approved production reconciliation

The remaining approved production packets were tested first in an isolated
worktree, then applied to canonical. Donor research/history was not merged:
7,429 research paths, private reference objects, SDK/compiler assets and held
candidate bodies remain outside the publication set. Existing canonical physics
corrections and the genuine virtual-keyboard helper call/private storage survive.
The unrelated README edit and seven untracked research directories are preserved.

This is not an all-zero-loss merge. The owner approved the following specific
header-correction debits, independently reproduced on the landing base:

| Function | Meaningful / padded bytes | Approved packet |
|---|---:|---|
| `_bitmap_2d_alpha_bleed` | 548 / 560 | T: attested color unions |
| `_player_profile_3wide_list_update` | 1,222 / 1,232 | Revised E01c |
| `_rasterizer_dynamic_geometry_initialize` | 473 / 480 | Revised E01c |
| `_collision_move_point` | 4,744 / 4,752 | Interface MP |
| Total debit | **6,987 / 7,024** | No additional exact loss authorized |

All four exact variants remain in ancestor `e59d5ca9`, before the combined
production correction. Fresh baseline/trial objects and instruction-level
comparisons are retained locally. Three already-residual functions also change
only allocation/encoding details: ballistic line-of-fire, frame-statistics draw,
and model draw. The required park rebaselines preserve the recorded history.

The admitted Halo objects are dynavobgeom, object_lights, interface, hs_runtime,
game_engine and bitmap_drawing. Draw-primitives and bitmap_utilities are honestly
revoked. Net objects: **390 -> 394 of 468**. The six admissions have independently
passing January section, data, storage and resolved-relocation audits. Across
their helper/provider checks plus ioinit, 136 surplus definitions are identical;
one is the specifically approved object_lights file-path-only exception.

The narrow object_lights selection check uses all 621 current objects in the
reconstructed January module order. It selects action_vehicle's January-identical
provider; the absolute-path alternative is unreferenced. Reversing the relevant
provider order selects the other path, so the control detects the difference.
This probe uses forced handling of inherited duplicate definitions and terminal
stubs for unrelated unresolved names. It is bounded selection evidence, **not**
a successful ordinary whole-program link or a boot test. The 130 ordered pair
link receipts likewise establish duplicate compatibility, not program completeness.

## Storage, source policy and alignment

The reduced 148 storage packets comprise 126 Halo and 22 incidental vendor
packets. A fresh whole-build census checks 640 overlapping owner/name rows,
including all 598 newly static config rows. They resolve to the intended static
definitions, with no undefined reference from another built TU. The only COMMON
owner changes are the approved object-list pointers (seven owners -> object_lists)
and temporary render color (ai_debug -> actions). No speculative owner, padding,
initializer or fabricated retaining call is added.

The key-agreement, AIFF and RIFF de-aggregations are separate zero-credit commits.
Their 20 file statics have freshly checked later-build PDB names/types and January
offsets. Key-agreement and AIFF restore alignment 4 instead of 8; RIFF removes
the inherited alignment directive and two aliases while preserving resolved
bytes and relocations. The fourth RIFF name is later-build-attested; January's
literal remains `riff chunk`. No header changes are bundled with these packets.

The inherited TIFF self-alias macros exposed by the vendor static renames were
removed rather than retained as invented macros; the entire object remains
identical. Vendor `__ioterm` becomes strict exact (35 bytes) through the reviewed
one-past relocation alias/name correction, and libcmt ioinit is admitted. It was
already credited by the semantic scorer: **zero new credited vendor code**, and
never Halo credit. All other vendor storage edits also earn zero Halo credit.

Approved BUG disclosures remain site-specific. Existing byte credit is not
blanket source-policy approval. The hs_runtime long ABI retains its explicit
partial-write disclosures, including upper bytes copied or CRC'd rather than
claiming that indeterminate data never escapes. No new assembly or pragma is
introduced. B3, INC-3, fast_ftol and every other unapproved packet stay held.

## Final evidence and accounting

Fresh stock-build warning census: **199 -> 74**. The only added normalized warning
is the disclosed hs_library_external C4090 at the const permutation-name argument.
The actual callee only reads the string through `_stricmp`; no suppressing cast
is added. The 126 removed warnings are the reviewed hs_runtime/hs ABI mismatches.
This is stock-flag evidence, not a claim of a new whole-board /W3 run.

The shell temporary rebinding is checked sequentially. Comparing directly across
the combined packet initially fails the unchanged external-symbol set because
two separately approved statics changed storage. A fresh control containing only
those storage fixes preserves all sections and relocations; the original checker
then passes all 31 checks against the final form. `$T18302` -> `$T18502` changes
only the binding name. Both targets remain `_main+81` and `_main+99`. The failing
combined receipt is preserved, not discarded or hidden by weakening the checker.

Final canonical build passes. The 8,252-row strict sweep has **7,639 exact**:
two gains (main_crash and vendor ioterm), exactly the four listed approved losses.
Tools: **1,318 passed, 5 skipped, 100 subtests**. Parks: **72 active, 0 stale,
0 invalid**. Admission: 6 candidates, 0 contradicted/rejected/revoked audit rows.
The 26 inherited fake-match leads are unchanged; `git diff --check` passes.

Final independent binding finds all **621 configured compiled objects** equal to
the audited trial outside debug records, including raw auxiliary selection fields,
symbols, COMMON and undefineds. All **833 split objects are raw-file-identical**.
An extra pre-existing canonical `build/base/libs/libcmt/chkstk.obj` is preserved:
its unit is MISSING with no base_path, base recipe or response-file input. It is
not part of this source build or the bounded link proof, and earns no credit.

| Halo measure | Final |
|---|---:|
| Meaningful credited code | **1,591,266 / 1,770,166** |
| Credited functions | **7,466 / 7,574** |
| Complete objects | **394 / 468** |
| Credited data | **2,648,123 / 3,923,451** |

Relative to the pre-publication source, new Halo code is +11 meaningful bytes,
approved debits are -6,987, and the net is **-6,976**. Data increases **59,220**:
main 1,796 + HS 54,780 + bitmap_drawing 2,644. Of the HS amount, 1,658 is modeled
padding as disclosed above. Object admissions, restored attribution and vendor
changes are not misreported as new source-code matches. No scorer upgrade occurs.

Local receipts: `scratch/astra_publish_20260927/final.*`, `inventory/FRESH_AUDIT.md`,
`main_review/c_align/`, `q11_review/`, and the isolated trial's `scratch/reconcile/`.
These include complete provider, storage and section comparisons. Public commits
contain source/config/tools and this curated report, not private executable or
compiler-derived binary evidence. Publication is fast-forward-only to the two
authorized `jonas/exact-pilots` branches; neither repository's main branch changes.
