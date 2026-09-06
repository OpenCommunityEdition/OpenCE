# ActionCharge: authentic math owners and private names (2026-09-06)

## Result and scope

Starting at published `71128a19fc9707f678f970e380b97ee3181db05e`, remove the
reconstructed ActionCharge-only alias/duplicate-definition packages for
`square_root` and `magnitude_squared2d`. Their existing, identical definitions
in `source/math/real_math.h` now supply the ordinary source call sites.
The header itself is unchanged. No implementation body is rewritten or tuned.

Both helpers still materialize and remain strict exact on the first natural
compile. All **20 inherited exact ActionCharge functions** survive. The
688-byte candidate `action_charge_setup` residual is also byte/relocation
identical to its pre-trial version; `action_charge_perform` remains unwritten.
ActionCharge remains **20 exact / 1 residual / 1 unwritten**, not complete.

Across the campaign all **6,271 strict functions** survive. Meaningful exact
code remains **919,549 bytes**, credited functions **6,225**, matched data
**2,061,024 bytes**, and Matching objects **391/833**. This is source-ownership
progress, **zero new function/byte credit**. The helper bodies were already
counted; changing their owner representation does not count them again.

## Why this is authentic reconstruction

The source previously renamed these two header functions before including
`actions.h`, undefined the aliases afterwards, and supplied duplicate external
C definitions. The historical recovery ledgers explain that those packages
were introduced during reconstruction to reproduce selected-object helper
ownership. They are not original source evidence. Review included the
2026-08-26 math-helper rejection and wave-2 ledgers, real-random recovery,
the 2026-08-28 public residual recovery, and 2026-08-29 helper reintegration.

HCEX's actual debug records authenticate `real_math.h` ownership and report
`WasSpecifiedAsInline=true` and `WasInlined=true` for both functions:

- `square_root`: RVA 0x169DC28, source lines 644-646;
- `magnitude_squared2d`: RVA 0x169DC50, source lines 733-739.

Use `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe -sym <name>` and
`-lines <name>` against
`research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`,
relative to the outer `i-w` workspace. Both line records name
`d:\projects\code\hcex\sources\math\real_math.h`, source MD5
`DE0F004C6EB037D8BE384822F84E002C`.

The January symbol atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`
marks adjacent `normalize2d`, `magnitude3d` and `normalize3d` as inline
functions selected from ActionCharge. It omits these smaller two symbols;
do not misrepresent that omission as direct January name/inline evidence.
January's actual helper bytes and the current header bodies independently
agree; the original selected COFF representation is not proof of a hand-written
external wrapper in the original C file.

Current ordinary call sites justify retention without a fabricated anchor:
`magnitude2d` calls `square_root(magnitude_squared2d(v))`, and `magnitude3d`
calls `square_root` on the three-component squared magnitude. Reuse those
unaltered calls and the existing ordinary header definitions. No new inline
annotation, static-inline conversion, qualifier, forced emission, new source
alias, compiler switch, object patch or comparator exception is introduced.

## Full compiled-object proof

Root's complete before/first ordinary COFF fingerprint comparison finds
**38 COFF sections: 36 unchanged and exactly two ownership changes**
(34 unchanged non-debug sections plus both unchanged debug sections).
Only the COMDAT selection of `square_root` and `magnitude_squared2d` changes
from NODUPLICATES (1) to SELECT_ANY (2). Every executable/data byte, relocation,
symbol name/storage, debug section, COMMON owner and other runtime header
property is unchanged. No section is added or removed. The existing selected
`point_from_line3d` body is unchanged; there is no new copy or inline scheduling
change. The first gate retains both exact helpers and all inherited functions.

Normalized helper hashes remain:

- `square_root`, 16 padded bytes, zero relocations:
  `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7`;
- `magnitude_squared2d`, 32 padded bytes, zero relocations:
  `9d8460618a63e41e020b2ce8071486fe278a849abccad44e5ea444da9700d014`.

The genuine header is unchanged, so no other translation unit needs a source
rebuild. A full Ninja run compiles ActionCharge and regenerates the campaign
reports. In this owner-only phase, all other current base objects and every
selected target remain identical to the frozen baseline. The full strict census has zero
gains and zero losses. **650 tests plus 26 subtests** pass; parks remain
**297 active / zero stale / zero invalid**. Admission retains five existing
rejections and has zero candidates, contradictions or revocations. The source
has zero fake-match scanner leads.

## Current owner census and actual link outcomes

The complete 572-base-object census now finds 60 `square_root` definitions
and 15 `magnitude_squared2d` definitions, **all SELECT_ANY**, with identical
target-correct bytes within each family. There is no remaining NODUPLICATES
definition for either name. `dot_product2d` remains seven identical definitions:
ActionCharge's one NODUPLICATES and six SELECT_ANY copies.

Root's actual VC7 `/DLL /NOENTRY /NODEFAULTLIB` links of the frozen complete
PathObstacles and ActionCharge objects, in both input orders, now report only
`dot_product2d` as LNK2005. The previous `square_root` and
`magnitude_squared2d` duplicate errors are gone. The expected 23 unrelated
unresolved dependencies remain and no successful full engine link is claimed.
No forced-multiple-definition flag, object suppression or synthetic stub
removes the remaining conflict.

## Authenticated private names, without behavior changes

After freezing and verifying the two-owner restoration, a separate `named`
phase replaces exactly two address placeholders in declarations, definitions,
calls and the symbol comment:

| January file offset | Old placeholder | Authentic private name |
| ---: | --- | --- |
| 4304 | `code_000010d0` | `action_charge_find_target_range` |
| 9168 | `code_000023d0` | `action_charge_valid_melee_destination` |

The exact January atlas cited above supplies both names at lines 7 and 14,
each attributed to ActionCharge. HCEX Release DIA independently identifies
both as static and not specified inline, with source lines 821-854 and 860-901
in `d:\projects\code\hcex\sources\ai\action_charge.c`. The source was already
static; the two matching `config/symbols.json` entries now record the genuine
names and `static: true` rather than csplit's default external ownership.

The source argument types and all executable statements remain unchanged.
In particular, the melee destination's second argument remains
`real_point3d const *goal`: HCEX's `short goal` type metadata is contradicted
by actual January point loads and by the later decompiler's own corrected
body. January reads `[ESI]`, `[ESI+4]` and `[ESI+8]`, while its caller supplies
the prop's center of mass. Do not import the erroneous short signature.

January's helper does not access or write through `melee_target_point`, and
the caller never consumes that discarded local. The HCEA semantic body and
caller corroborate this. Keeping `(void)melee_target_point;` therefore
preserves actual behavior; do not invent an output write or use the parameter
name as proof of missing executable logic. Whole-TU optimization could remove
historical dead stores, so the audit does not claim to prove the original text
never contained an assignment.

Root's full before-name/after-name fingerprint comparison reports all **38
runtime sections unchanged**, once the two authenticated symbol identities
are mapped for comparison. No instruction, literal, data, relocation offset,
addend, debug record or ownership change exists in the base object beyond
those names. The selected target changes names and the two static-storage
records. Across all 1,405 current objects, only the ActionCharge base and
selected target hashes change; every other object is byte-identical. The
full rebuild and strict census again preserve all 6,271 exact functions.
This audit-name mapping is not a comparator exception or new byte credit.

## Remaining boundary

`dot_product2d` has the same authenticated header-inline provenance, but no
live source caller in current ActionCharge. Its real use belongs to the still
unwritten `action_charge_perform`. Do not invent a caller or a retention anchor
to make an uncalled header definition materialize. Its old duplicate-definition
package remains outside this two-owner restoration and needs the genuine
caller closure. Other inherited ActionCharge aliases/prototype
debts are not certified by this patch, and the large residual is not reopened.

This does not claim a complete engine link or whole-object admission. It
reconciles two real duplicate-owner families while preserving all compiled
behavior and accounting. Active Opus implementations and donor worktrees are
untouched. Original binary evidence and temporary link outputs stay local.

## Frozen evidence

Two independent post-implementation audits pass. The owner audit replays the
full before/first COFF comparison, verifies all 572 named-phase base objects,
and runs the actual VC7 diagnostic links in both orders. Its script hard-pins
the imported comparison tools and linker. The separate private-name audit
checks all section and relocation records, caller targets/addends, exact source
and config changes, and all 1,405 current base/selected object hashes.

- Owner replay script: `scratch/action_charge_two_owner_postimplementation_audit_20260906.py`,
  SHA-256 `0cc2e805f87cc55920a6bc3202aa284185e36374cbd7eb335e5c1ad15e86fa9d`.
- Independent owner report: `scratch/action-charge-two-owner-postimplementation-audit-final-20260906.json`,
  SHA-256 `096b850bd6c00d0e3b8692dea0fbe0ad4e4789556a868a78ab84ed9b8dc9a397`.
- Independent private-name report: `scratch/action-charge-private-name-postimplementation-independent-20260906.json`,
  SHA-256 `380291ccd1dd183ca88231dd08b2b68f5aae04639e60fdfffc767006228b11a4`.

These reports are local frozen evidence, not target binaries to distribute.
Both audits preserve the distinction between the remaining `dot_product2d`
link conflict and the two genuinely resolved duplicate-owner families.

- Baseline: `scratch/action-charge-math-owner-before-20260906.json`,
  SHA-256 `34ccf1a6d5e32b4348364235802ba488b62c48317e83c45d0317f9fdc93fa4dc`.
- First: `scratch/action-charge-math-owner-first-20260906.json`,
  SHA-256 `fd64316616dca63d976bd3c9ca367b9061a69e80ae0c28f27e61a443cdd210d5`.
- Final: `scratch/action-charge-math-owner-final-20260906.json`,
  SHA-256 `a81dcaced90b94c6726aa7004b04bb56213c2f2208efe21a8b1898771183d7c5`.
- Before C/base object: `e8096e28a0a821ae5f97b0461c61ae6f828f51037b0cb62b75f5b1d8fb0be366` /
  `45c5386b96ba99b67834dd3ffd85e797dc5fdb8385db4cd4ab1e73d006f8df6d`.
- Pre-name two-owner C/base object: `5f182feceec9ccf33d2d8eade40c88db42c697cbfe4d26f5682fce3121bd193a` /
  `8c19c85deb223ff89aeaed6ff768e192139e040496c853b1fb91e31eef5c315b`.
- Unchanged math header: `4e640d1be7b2c843dc7c9acb198afcd5d7d390eb310e5e14eedd484cf99535d4`.
- `scratch/action-charge-math-owner-{before,first}-stable-20260906.json`:
  6,271 strict functions, zero gains/losses.
- Named final: `scratch/action-charge-math-owner-named-20260906.json`,
  SHA-256 `1d4305f1d603e619889f2e94dd282ca40a364d3115ea447ddb579a7c7e257a88`.
- Named final C: `75961e822a451f5d23910def9cc4661e1fbd0bdefa9caa314f776cf0aa21e81d`;
  base object: `cecd4d02fc1dbcf8a355ef289899f7230bd3463b3299abe486cb36ee871ef371`;
  selected target: `a906f0ec24b0313a056d4a413eaf85ee32f22f02c45878261bcae4521bfa63c8`;
  symbols configuration: `57d80ec4aa5d567067f97ac9f4552bba4d85dc777201d480fdadd47f93b06ccf`.
