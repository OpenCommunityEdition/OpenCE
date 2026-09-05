# HUD Draw: two exact leaves integrated — 2026-09-04

## Frozen result

Starting publication is `2f57d090572587232683b1a13fc0beab3b59855e` on both
bnunu/halo integration refs. Root adopts only the two independently gated,
source-authentic helpers from the separate HUD Draw scratch proposal:

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `real_alpha_intensity_to_pixel32` | 172 / 176 | 13 | `82b3d86d1b33555fbecbbe6faec45a094ba2dca6e3b36c110daebe38251842a6` |
| `get_flash_duration` | 32 / 32 | 1 | `dcf81ee7e83bf429645379d2ecd9b0992d7d77fa53b25a791a6c04de16d0e973` |

Both compare strictly against January in the production object. Existing
`check_stack_buffer` and `hud_globals_get_scale` remain strict. HUD Draw
advances from 2 exact / 21 unwritten to **4 exact / 19 unwritten**, with no
new residual park and no whole-object completion claim.

Full production totals change from 898,381 to **898,585 meaningful credited
code bytes**, and 6,138 to **6,140 credited functions**. Data remains
2,022,998 bytes. Matching objects remain **391/833**. The stable sweep grows
from 6,184 to **6,186 exact owners out of 8,245**, preserving every inherited
exact and adding precisely target sections 11 and 13 (208 padded bytes).

## Source and the legitimate flash-duration reopen

The alpha/intensity helper preserves January's exact assertions, assigns all
four genuine `real_argb_color` components, and returns the existing public
ARGB packing API's result. It does not implement the missing ARGB converter
by a guessed prefix, packing facade, or numerical shortcut.

The public declarations already belong to `interface/hud_draw.h`. The flash
helper uses the complete existing `hud_color_definition` from its actual
owner header `interface/unit_hud_interface_definition.h`; no public type or
prototype was duplicated in this C file. Only that necessary include and
the two bodies were added. Both functions use project types, one parameter
per line, semantic names, and explicit value returns.

The historical `hud_draw_obj_jonas_flash_duration_rejection_20260829.md` tested
an ANSI cast, which calls `__ftol2`. New evidence distinguishes the real
rounding semantics: January multiplies by 30, narrows the product to a
four-byte real slot, reloads it, and uses current-rounding-mode `fistp`.
The now-authenticated existing `cseries.h::fast_ftol` expresses exactly that
contract and yields a first-shot strict match. No compiler option, target
byte, intrinsic, forced-inline directive, helper expansion, or new assembly
block was introduced. This supersedes the old cast-only conclusion using
specific semantic evidence, not another equivalent-spelling search.

## Rejected proposals are not credited

The scratch RGB converter was rejected after a valid-input counterexample:
under January's 53-bit x87 nearest mode, binary32 channel
`0.0019607844296842813` times 255 exceeds 0.5 before narrowing. January RGB
converts that product directly to integer 1; the proposed helper call first
narrows to binary32 0.5 and then converts to ties-even integer 0. This is an
observable arithmetic difference, not merely register allocation. The body
never reached production and is not a fuzzy park.

The proposed `(long)value` body for `fast_ftol_C` is also withheld. January
has a 56-byte hand-optimized FIST/FISUB and bit-pattern correction sequence;
the proposed compiler cast uses a runtime helper and has no authenticated
finite/in-range precondition establishing equivalent exceptional behavior.
No undefined conversion or unproven semantic approximation is added just to
reduce the unwritten count. Preserve those research artifacts and conclusions
for a separate evidence-backed conversion investigation.

The RGB finding triggered an independent all-twelve-site check of the newly
published HUD Unit renderer. Both reviewers confirm that January and the
published candidate explicitly narrow all shield/auxiliary products before
FISTP, so this particular defect does not occur there. See
`hud_unit_obj_renderer_precision_followup_20260904.md`.

## Complete-owner boundary

The production candidate has five code owners: the four exact target-owned
functions plus an authentic, unreferenced `_fast_ftol` copy, 32 bytes,
selection 2/select-any, normalized SHA-256
`8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.
That additional copy is not owned by January HUD Draw and receives no code
credit. January's selected Actor Combat copy is selection 1/no-duplicates;
the previously documented ordinary XDK link rejects that coexistence even
though the bodies match. No compatible canonical base/base owner has yet
been demonstrated. An explicit policy veto preserves this boundary against
future accidental whole-object promotion.

There is no `_point_from_line3d` code definition or COMMON allocation. The
new intensity assertion matches its genuine target RDATA owner; ordinary
alpha/path/zero/one/thirty pooled constants are not promoted into extra HUD
data credit. The full 384-byte target data/BSS contribution remains incomplete.

## Reproduction and frozen identity

Root owns the production application and full build; worker proposals never
write `build/`. The clean starting snapshot is the preceding published HUD
renderer snapshot, not a dirty measurement:

```text
python tools/campaign/gate.py source/interface/hud_draw --all --forbid-emitted-symbol _point_from_line3d --out scratch/hud-draw-root-two-leaves-20260904.obj
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/hud-draw-two-leaves-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/hud-renderer-final-after-20260904.json scratch/hud-draw-two-leaves-after-20260904.json
python -m tools.parked_functions --output scratch/hud-draw-two-leaves-parks-20260904.json
python tools/audit_object_admission.py --output scratch/hud-draw-two-leaves-admission-20260904.json
python tools/fake_match_scan.py source/interface/hud_draw.c
python -m pytest -q tools
python configure.py progress
git diff --check
```

Physical source SHA-256:
`88ec95f46f83dd181ad12ebf79a68596afac5d3591d56bc87f9ab19d19f00c3a`.
Production object SHA-256:
`b28b108a93d664bd2849fc3cbb846795566ef122d15d23990e1431ddb47d30f6`.
Root scratch gate object SHA-256:
`617fda2270766f592f601b8065312ed7f97e58b61e56474b9ce483a3b0844ee9`.
Debug/source paths and line endings explain physical artifact differences;
strict byte/ordered-relocation comparisons are the authoritative function
evidence. Proprietary and scratch binaries are not committed.

The full sweep reports two gains and zero losses. All **252 parks remain
valid**, with zero stale or invalid records. Admission reports zero candidates,
zero contradictions, four otherwise-complete reviewed rejections and zero
revocations; the still-partial HUD Draw/Unit vetoes remain recorded separately.
The source scan reports zero leads, and **288 tests plus 26 subtests pass**.
The only warning is the existing optional pytest-cache permission warning.
No protected bitmap file or unrelated source was changed.

The worker's detailed source, rejected arithmetic examples, ordered-relocation
comparison and complete literal-owner evidence are in
`hud_draw_obj_small_helpers_proposal_20260904.md`. The retained production
source has the same normalized text as its final two-leaf proposal.
