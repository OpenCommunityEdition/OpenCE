# HUD Unit renderer integration — 2026-09-04

## Verified result

Starting published commit:
`fc8b19906d480c161dcda3dc36d79a9c6806478e`, verified on both bnunu/halo refs
`jonas/exact-pilots` and `jonas/ai-debug-integration-20260829`.

This batch retains the new exact private initializer and a substantial,
source-credible **nonexact renderer**. It does not complete HUD Unit or waive
its helper ownership boundary. The new code was independently reviewed for
types, January control flow, floating-point comparisons, calling conventions,
and complete candidate code/data ownership before the root production build.

| Measure | Published start | Verified result | Change |
| --- | ---: | ---: | ---: |
| Meaningful credited code bytes | 898,330 | 898,381 | +51 |
| Credited functions | 6,137 | 6,138 | +1 |
| Credited data bytes | 2,022,994 | 2,022,998 | +4 |
| Matching objects | 391/833 | 391/833 | 0 |
| Strict owners in stable sweep | 6,183/8,245 | 6,184/8,245 | +1, zero losses |
| Valid measured parks | 251 | 252 | +1, none stale/invalid |

The stable gain is exactly `source/interface/hud_unit::section:4`, 64 padded
bytes. Production progress reports its meaningful size as 51. The renderer's
3,493 meaningful target bytes receive **zero exact credit**; its 3,460-byte
non-padding candidate is preserved separately as fuzzy implementation work.
The ordinary helper and pooled literals receive no extra HUD Unit credit.

HUD Unit now has 21 exact target functions, one measured residual, and no
unwritten target functions. Its raw objdiff count is 20 because the existing
strict damage-indicator function needs the already-approved switch-label
exception. No new comparator exception is added by this batch.

## Genuine ownership and semantic names

The newly live private helper is `initialize_hud_state`. Its name is inferred
from its genuine caller and sentinel-initialization behavior, not represented
as a recovered January PDB name. The target map at file offset 812480 now
names `_initialize_hud_state` with `static: true`. Its 64 bytes and sole
ordered relocation compare strictly; normalized SHA-256 is
`117b8363f3f64f6c4d6d19fd8264e389aa8479f00f1a7f588213d802eb430bca`.
The real renderer caller retains it naturally, without a synthetic anchor.

HCEX local-symbol evidence identifies `overcharge_count` as a real static
local `long`. January supplies a four-byte DATA owner with value four and
two renderer references. File offset 3034216 is now mapped to the compiler's
actual local-static name
`?overcharge_count@?1??hud_render_unit_interface@@9@9`, with `static: true`.
Target/candidate DATA are strictly equal, with normalized SHA-256
`fb5e512425fc9449316ec95969ebe71e2d576dbab833d61e2a5b9330fd70ee02`.
No public facade, fake aggregate, anonymous address-derived source storage,
or contribution-size change was introduced.

The pre-existing `unit_hud_globals` BSS and all six January RDATA owners stay
exact. The renderer also uses ordinary pooled 0.2f and assertion literals
whose real selected January owners are elsewhere. The assertion's no-space
`player->local_player_index==render.local_player_index` spelling comes from
January's 54-byte string, correcting the draft's different 56-byte literal.
This is string evidence, not instruction-allocation tuning.

## Honest residual and complete-owner boundary

The rebuilt renderer is 3,472 padded bytes against January's 3,504. Both
have 102 relocations, but equal counts do not imply an exact comparison.
Production objdiff similarity is 78.98858 percent. The measured park uses
the conservative `unclassified` class and these normalized hashes:

- target: `b82d575e1154cad1dd27cee874e9af5cc4b86dc302ea41685b56181ba32d93bc`;
- base: `4071a4f0c7b1f808423bd5673e822e7f3c6a036e9a3a2db0fe7bd14b413b1817`.

The complete candidate census has 23 code owners versus January HUD Unit's
22. Its sole additional code owner is the authentic shared `_fast_ftol`,
32 bytes, zero relocations, normalized SHA-256
`8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.
It is unreferenced by this object after natural call-site inlining. It is
neither an invented helper nor eligible for fresh target-function credit.

Identical body bytes are not sufficient linker ownership evidence. This
candidate is select-any (selection 2), whereas January's selected Actor Combat
copy is no-duplicates (selection 1). The prior Player Rumble owner audit
documents the ordinary XDK duplicate-owner failure; no compatible canonical
base/base owner or discard pass has been demonstrated here. No whole-object
admission follows from this source reconstruction. See
`player_rumble_obj_jonas_object_accounting_audit_20260831.md` and the independent
HUD ownership review for the precise boundary and reopen conditions.
An explicit `candidate-only-comdat-owner` policy record preserves this veto
even if the renderer's function comparison later becomes exact. It does not
remove valid initializer/data credit. HUD Unit is still partial, so it is not
one of the four otherwise-complete candidates rejected by this audit run.

There is **no `_point_from_line3d` code definition and no COMMON allocation**.
Root reran the emitted-symbol guard using the actual decorated name including
its leading underscore, and independently compared the production object.
The retained renderer introduces no forced-inline directive, manual math
expansion, new assembly block, compiler flag/pragma trick, raw-offset view,
fake storage, optimizer barrier, or register/volatile steering.

This is approval of the new implementation and measured credit, not a claim
that all inherited source debt in HUD Unit is resolved. In particular, the
two pre-existing one-byte public no-op bodies still have address-derived
names and require their own semantic ownership/name evidence. Their old
credit is unchanged, and HUD Unit remains `NonMatching`.

## Source review and frozen identity

Read the complementary ledgers:

- `hud_renderer_owner_api_preparation_20260904.md`;
- `hud_unit_obj_renderer_initializer_wave_20260904.md`;
- `hud_unit_obj_renderer_independent_review_20260904.md`.

Those records distinguish January evidence from later PDB/source corroboration
and describe the genuine 18-entry arrays, one auxiliary value, five colors,
static local, meter copies and stack guards. The bounded fixes address real
unordered floating-point behavior, cutoff equality, current/reference operand
selection, the six genuine `fast_ftol_C` health calls, and correct rounding
through the existing sparingly assembled math helper. No further residual
spelling or allocation search was performed.

Frozen physical source SHA-256:
`cc98970ee2318245d01706a1fd6941b4207aab62a3ea1e2293879a8738d260eb`.

Production `build/base/source/interface/hud_unit.obj` SHA-256:
`f9432eedde20ac7c9d388e46392211287393da6b90337aa884e6571e7ab0aafe`.

Root scratch gate `scratch/hud-renderer-root-verified-20260904.obj` SHA-256:
`34bc73754ab43e2d490dce89d21c554757776fdeae78170a537583ca131a8332`.

Physical object hashes differ with debug/source-path metadata; normalized
per-owner bytes and ordered relocations are the strict comparison evidence.
Compiler objects and proprietary assets are not committed.

## Full verification

Workers froze production source before root ran the full Ninja build. Other
workers' new proposals remained in separate scratch files. Root's comparison
uses the clean published prerequisite snapshot, not a dirty baseline:

```text
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/hud-renderer-final-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/hud-renderer-owner-api-after-20260904.json scratch/hud-renderer-final-after-20260904.json
python tools/campaign/gate.py source/interface/hud_unit --all --forbid-emitted-symbol _point_from_line3d --out scratch/hud-renderer-root-verified-20260904.obj
python -m tools.parked_functions --output scratch/hud-renderer-final-parks-20260904.json
python tools/audit_object_admission.py --output scratch/hud-renderer-final-admission-20260904.json
python tools/fake_match_scan.py source/interface/hud_unit.c
python -m pytest -q tools
python configure.py progress
git diff --check
```

Results: zero inherited exact losses; 252 valid parks, zero stale or invalid;
zero admission candidates or contradictions, four existing reviewed
rejections, zero additional revocations; zero fake-match scan leads; and
**288 tests plus 26 subtests passed**. The optional pytest-cache permission
warning remains unrelated. Source/ABI/ownership review, not the scanner alone,
supports admission of the new function.

Protected bitmap source/header hashes remain respectively
`ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140`
and `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`.
No bitmap work was removed.

## Adjacent documentation

The Input Xbox `+0x230` lifecycle audit is evidence-only: all 833 January
objects provide one explicit zero write and no semantic read, and the stripped
January PDB/later incompatible type cannot identify that field. The three
coupled lifecycle functions remain unwritten rather than receiving guessed
storage or synthetic helper anchors. See
`input_xbox_obj_lifecycle_unknown_230_blocker_20260904.md`.

The campaign README now correctly distinguishes padded board counts from
meaningful canonical credit, warns that its default partial-object list is
filtered, and directs regression checks to the complete stable sweep and
insert/delete instruction analysis to the aligned diff tool. These are
documentation corrections only; no comparator, gate, or test was weakened.

The campaign target remains 625 completed objects out of 833. This batch
does not meet that goal or claim a new completed object.
