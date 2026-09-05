# Input, HUD and Player UI follow-up — 2026-09-04

## Starting point and current boundary

Published baseline `d556ba5304eeb4ee663b7c81998925b36f2d5cd7` is verified on
both bnunu/halo refs `jonas/exact-pilots` and
`jonas/ai-debug-integration-20260829`. It credits 897,322 meaningful code bytes,
6,134 functions, 2,022,850 data bytes and 392/833 Matching objects. The stable
baseline `scratch/input-hud-next-wave-before-20260904.json` contains 8,245
owners and 6,181 exact functions. Do not replace this baseline with a dirty
candidate measurement.

This follow-up is **source-frozen and verified for publication**. One C-file
owner per lane: Input Xbox, HUD Unit, Player UI; root owns HUD Sounds and
shared headers/configuration. Workers used scratch gates only. The final
results below supersede the historical intermediate measurements in the
individual lane ledgers. Remote publication must still be checked against
the actual commit containing this ledger.

## HUD sound source-authenticity correction

Independent review finds the old `word *volatile played_flags` qualifier
unsupported. It volatile-qualifies a nonescaping local parameter pointer,
not the referenced flag word, and has no authenticated runtime purpose.
Its schedule effects cannot support a house-rule-clean byte match. The old
body also lacks the explicit void return and contains a redundant nested
handle check. See `hud_sounds_obj_source_authenticity_review_20260904.md`.

Root removes the qualifier, restores `return;`, uses genuine SET_FLAG calls,
and performs one natural impulse-sound guard cleanup. The same five-argument
cdecl API now lives in closest existing owner `interface/hud.h`, with no
caller-local public prototype. A pre-existing non-self-contained color-pointer
declaration there now names its actual forward-declared `union real_argb_color`;
no broad math-header import was added to work around the header error.

The natural helper is 384 padded bytes versus January's 352, both 13 ordered
relocations. Target normalized SHA-256:
`1d026cea1652a4639bc1258234647070cfdb9dfda61f268e1033299edb398f37`;
candidate `scratch/hud-sounds-source-authentic-20260904.obj`:
`5ad2867ca1866ae4cda357d19e78baca3a47d275b39f6640653948f1728cb858`.
Its source SHA-256 is
`406913f2ca516c3f2b9b7da3a0dce97e259fe2b95ae9070a2309652b8c046dda`.

`hud_sounds.c` is deliberately changed from Matching to NonMatching. This is
a documented **policy-driven revocation**, not a silently accepted accidental
regression or deletion of prior research: the exact-but-noncompliant source is
preserved at `d556ba530` and origin commit `32580a870`. A fresh measured park
and final full-sweep loss accounting are now verified below. Do not
weaken the strict comparator or reclassify the natural residual as exact.

Focused HUD API blast:

```text
python scratch/hud_device_api_blast_20260904.py --baseline scratch/input-hud-next-wave-before-20260904.json --output scratch/hud-sound-api-blast-20260904 --units source/main/main source/game/game source/game/players source/interface/hud source/interface/hud_nav_points source/units/units
```

All six units compile; 387 inherited exacts and their existing parks remain
unchanged. Units' inherited surplus helper is not newly admitted. This focused
check does not replace the final frozen full build.

## Unit-HUD tag schema

The previously empty owner `source/interface/unit_hud_interface_definition.h`
now declares the genuine full tag layout, typed `tag_get` access and original
sound-state enum. There were no existing direct includers before the change;
HUD Unit consumes it and removes its duplicated private placement/color
types. No fake opaque prefix or synthetic aggregate is introduced.

Source corroboration is pinned HCEA revision
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, accessible under parent workspace
`research-cache/halocea-full-blobs-20260830/src/headers/`. The proposal is
`scratch/unit-hud-schema-proposal-20260904.h`; root corrects its enum count to
the recovered `NUMBER_OF_UNIT_HUD_STATES` spelling.

Root independently inspected the actual HCEX PDB with:

```powershell
& 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe' -type unit_hud_interface_definition 'C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb'
```

PDB members confirm warning_sounds +0x3C0, auxilary_meters +0x3CC, and the
genuine long[89]/long[12] reserved tails at +0x3D8/+0x53C. Size assertions check
the full 0x56C type and every nested element required by the new caller.
January accesses and callee ABI remain authority over later-build semantics.
The 544-byte sound caller is now strict; the private 64-byte state
initializer remains deferred until its real large rendering caller exists.

## Other lanes and final requirements

Input Xbox closes `input_get_key` and `input_key_is_down` while retaining the
natural raw-data formatter as a measured residual. Initialization stays blocked on the
unidentified DWORD at globals+0x230; do not invent a field purpose or force
thread/rumble helper emission without its genuine caller.

Player UI has reconciled its semantic private profile-index owner and ordinary
typed iteration/macros. A 42/42 code count alone is not whole-object admission:
source credibility, exact data/BSS, ownership and extra-string coalescing must
all be independently checked before any Matching promotion.

## Final frozen integration results

Full Ninja regeneration reports:

| Measure | Published baseline | Verified candidate | Change |
| --- | ---: | ---: | ---: |
| Meaningful credited code bytes | 897,322 | 898,330 | +1,008 |
| Credited functions | 6,134 | 6,137 | +3 |
| Credited data bytes | 2,022,850 | 2,022,994 | +144 |
| Configured Matching objects | 392/833 | 391/833 | -1 policy revocation |
| Stable strict exact owners | 6,181/8,245 | 6,183/8,245 | +3 gained, -1 revoked |

The meaningful code delta separates three distinct kinds of change:

- Fresh strict implementations: `input_get_key` 97, `input_key_is_down` 192,
  and `hud_play_unit_sounds` 540, totaling **829 meaningful / 848 padded bytes**.
- Existing exact progress newly credited: Player UI's
  `set_local_player_controls_from_player_profile`, **528 meaningful/padded
  bytes**. This is accounting repair, not fresh implementation.
- Removed unsupported credit: HUD Sounds' `hud_play_sound`, **349 meaningful /
  352 padded bytes**. The source-authentic replacement is an honest residual.

The stable diff has exactly one loss,
`source/interface/hud_sounds::section:1`; it correctly returns nonzero.
All other **6,180 inherited strict exact owners remain exact**. No comparator,
baseline, target bytes or contribution boundary was modified to conceal this
deliberate policy revocation. The three gains are exactly the new Input/HUD
functions above. Player UI remains 42/42 strict without adding a stable gain.

HUD Unit's new ordinary `0.1875f` constant is four logical bytes and completes
its 144-byte reported RDATA contribution. All six target RDATA owners compare
strict; the remaining 140 padded bytes were existing strings that previously
received no credit for the incomplete contribution. Therefore **+144 data
credit is not a claim of 144 newly reconstructed data bytes**. The unresolved
four-byte initialized HUD Unit owner remains absent; HUD Unit is still partial.

Final object states are Input Xbox 20 exact / 4 residual / 3 unwritten,
HUD Unit 20 exact / 0 residual / 2 unwritten, Player UI 42 target functions
strict but source-layout admission blocked, and HUD Sounds one honest residual.
The Input raw formatter is parked at 96.69512%; HUD Sounds is parked at
82.10656%. Neither park receives exact code or completed-object credit.

## Whole-object policy and tool coverage

Player UI now has an explicit `source-layout-incomplete` rejection for the
incomplete public `struct playlist_profile`. Its ordinary typed iteration,
game-variant storage and semantic private-global naming remain admitted
source improvements. All target data and 42 exact functions stay credited;
the veto only prevents a misleading whole-object completion label. The proper
public January layout and documented real-link/coalescing boundary remain
open. No later-build layout was transplanted.

The policy validator accepts this reviewed rejection class and still fails
closed for unknown classes or missing/invalid evidence fields. New tests
prove that 100% function/data and a pre-existing complete metadata flag cannot
bypass it in admission or canonical/best/union treemap views, while valid
function credit is preserved. No testing or matching threshold was weakened.

The full tooling suite reports **288 passed, 26 subtests passed**. Its sole
warning is the existing optional pytest-cache directory permission warning.
All **251 park records are valid**, with zero stale or invalid records.
The admission audit reports zero candidates, zero contradictions, four
explicit rejections and zero additional tool-driven revocations. The
HUD Sounds demotion is already reflected in the source/config baseline of
that audit; it must not be confused with a zero-loss function sweep.

The fake-match scanner inspected all six changed C/header files and reported
zero leads. Independent human-style source/ABI/owner review is recorded in
the Input and HUD Unit independent-review ledgers; the scanner alone is not
semantic approval. The protected bitmap source and header hashes are unchanged.

## Reproduction and frozen source identity

```text
ninja
python -m tools.campaign.stable_verdicts snapshot scratch/input-hud-next-wave-after-20260904.json
python -m tools.campaign.stable_verdicts diff scratch/input-hud-next-wave-before-20260904.json scratch/input-hud-next-wave-after-20260904.json
python -m pytest -q tools
python -m tools.parked_functions --output scratch/input-hud-next-wave-parks-20260904.json
python tools/audit_object_admission.py --output scratch/input-hud-next-wave-admission-20260904.json
python tools/fake_match_scan.py source/input/input_xbox.c source/interface/hud.h source/interface/hud_sounds.c source/interface/hud_unit.c source/interface/player_ui.c source/interface/unit_hud_interface_definition.h
git diff --check
```

The stable diff's sole expected nonzero result is the named HUD Sounds policy
revocation; every additional loss would block publication.

| Frozen file | Physical SHA-256 |
| --- | --- |
| `source/input/input_xbox.c` | `b8195cdc8bb4e07791182c036e35ef96e6fd753d57f72f1f853e2e6c7f34cbd3` |
| `source/interface/hud_unit.c` | `651819986c519aa73f0bd90f03aa2a27771cf8bdaed9d859412f17b90e814312` |
| `source/interface/hud_sounds.c` | `406913f2ca516c3f2b9b7da3a0dce97e259fe2b95ae9070a2309652b8c046dda` |
| `source/interface/player_ui.c` | `f57dbcb19e13d4e9d160f226aa04ae20ea1bb2c9a5c613138bab3854f689b6c2` |
| `source/interface/hud.h` | `e78f5c4a36c9686a98eb696563e47a4218aaf609e3a282beb0b1676d69537703` |
| `source/interface/unit_hud_interface_definition.h` | `a71f603e5a3c136edafcf34a34ec730a779378e73ddaf9ea40c4cbaf1de71a96` |

Git line-ending normalization can change physical checkout hashes without a
semantic source change. Per-owner normalized COFF hashes in the individual
ledgers are the strict code evidence. Essential commands, measurements and
conclusions are committed here; proprietary scratch objects are not published.
