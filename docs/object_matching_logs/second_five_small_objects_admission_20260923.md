# Second five Halo object admissions (2026-09-23)

## Result

Five Halo-owned units outside the concurrent Lane D allowlist are promoted
from `NonMatching` to `Matching`. All 78 target functions, 17,507 meaningful
code bytes and 12,626 target data bytes were already credited before this
change. **Newly matched code bytes: 0; newly matched data bytes: 0.** This is
a whole-object ownership and source-quality admission, moving the object count
from 413/833 to 418/833.

| Unit | Strict functions | Already credited code | Already credited target data |
| --- | ---: | ---: | ---: |
| `source/ai/props` | 17/17 | 2,902 | 964/964 |
| `source/ai/action_uncover` | 9/9 | 2,587 | 304/304 |
| `source/cutscene/recorded_animations` | 16/16 | 2,684 | 894/894 |
| `source/rasterizer/rasterizer_text` | 14/14 | 2,984 | 2,776/2,776 |
| `source/networking/network_server_message_handler` | 22/22 | 6,350 | 7,688/7,688 |

Fresh PID-isolated `gate.py --all --forbid-emitted-symbol
_point_from_line3d` checks all five units at the listed strict counts with
zero residual or unwritten target functions. Target-owned named data sections,
relocations, symbol offsets, and storage have been audited. Neither active
parks nor object-admission rejections apply to these five.

## Candidate-only section boundary

`props` emits the authentic 48-byte `real_math.h` `_vector_from_points3d`
header-inline helper. `action_uncover` emits the analogous shared-header
`_distance_squared3d`, `_magnitude_squared3d`, and `_vector_from_points3d`
helpers. Their emitted code and relocation identities equal January's selected
copies. The project's established shared-inline COMDAT exception applies:
genuine shared-header inline, byte-identical copy, strict-exact caller and
no regression or hand expansion. Candidate-only pooled literals in these
units also match January's selected providers; ordinary base/base VC7 links
in both input orders pass duplicate definition checks.

`recorded_animations` has an internal `_playback_codec` label at offset 16
inside the same exact 34-byte `.data` block, not surplus storage. HCEA's
compiland independently attests its source field and offsets. Its two surplus
SELECT_ANY strings match January's selected providers byte-for-byte and link
without duplicate-definition errors in both base/base orders.

`rasterizer_text` has matching target data and logical-zero BSS symbols. Its
two surplus SELECT_ANY constants are byte-identical to January's selected
providers and pass ordinary base/base duplicate-link checks in both orders.
The source cleanup replaces a raw debug-options offset with the typed field
and places all three rasterizer bitmap lifecycle prototypes in their true
definition owner's new narrow header; consumers no longer redeclare them.

`network_server_message_handler` has all 120 target-owned code and data
sections exact, no extra code, and seven surplus SELECT_ANY data sections:
three SDK arrays and four pooled strings. Each matches the corresponding
January selected provider, and base/base selected-provider links in both
orders have no duplicate-definition error. As in the earlier
`network_client_manager` admission, mixed candidate/January-split links can
fail `LNK2005` because csplit represents January's selected copy as NODUP.
That is a bounded proof of ordinary current-object coalescing, **not** a claim
of a successful whole-program mixed January/current link. No surplus section
receives January-byte credit.

## Verification and rejected alternatives

`ninja` builds all 833 units and reports 418/833 matching. The stable
function diff over 8,245 rows is gained 0, regressed 0, with 7,493 strict
exact in both snapshots. Park validation: 151 active, 0 stale, 0 invalid.
Admission audit: 41 pending candidates, 0 contradicted, 10 rejected,
0 revoked. Fake-match scanner finds 0 leads in edited source. Tool tests:
1,152 passed, 5 skipped, 26 subtests. `git diff --check` passes.

The tempting `game_engine_race` and `game_engine_ctf` objects remain
`NonMatching`: although their target bodies and data match, their surplus
`_random_range` conflicts with the current selected provider in ordinary
base/base links. `actor_stimulus`, `actor_type_flood`, `path_smoothing`,
`collision_usage`, `unit_dialogue`, and other superficially complete rows
retain distinct ownership, storage-class, or relocation blockers. The user's
held `path_obstacles` parenthesis candidate remains untouched. No Lane D
source file was changed.
