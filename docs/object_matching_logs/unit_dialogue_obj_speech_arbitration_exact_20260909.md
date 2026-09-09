# `unit_dialogue.obj` speech-arbitration exact packet (2026-09-09)

## Result

Starting from donor-branch commit
`a53e8d63eace21a192b0589709b6c4e335a41bf4`, this packet reconstructs the
caller-connected speech arbitration core in `source/units/unit_dialogue.c`.
Three functions are newly strict exact for **1,396 meaningful / 1,440 padded
code bytes** and 63 relocation records:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_unit_test_speech` | 738 | 752 | 30 | `5a51601a7f1599532a46c37d822cb6ae32db64adbd101015a2536eb21dc70ad9` |
| `_unit_lose_speech` | 193 | 208 | 15 | `97542404d1e6c8b5622425b0fa426a8715783fdde1e488c08a11c3466031e261` |
| `_unit_speak` | 465 | 480 | 18 | `fb4b2e9226970d97014d9dd893d387f218ddcb8b31d66d5b439ae6c26483349e` |

The two target-owned arbitration tables are also restored in their natural
translation unit. Their combined 68-byte `.rdata` section is byte-identical
to January (including the two-byte alignment gap), but it is not included in
the meaningful-code total above.

`unit_dialogue.obj` moves from 5 to 8 strict exact functions. Its two inherited
residuals (`unit_dialogue_determine_variant` and `unit_dialogue_update`) remain
residual, and the four remaining unwritten functions remain unwritten. The
object therefore remains `NonMatching`.

## Source and semantic boundary

The retained code is ordinary typed C:

- `unit_test_speech` resolves a vocalization through the dialogue tag and
  fallback table, rejects non-death speech for dead units, and selects
  immediate, dequeue, or queued playback from the current/queued priorities
  and queue-time policy;
- private `unit_lose_speech` reports an interrupted waiting or queued item
  through the existing AI debug switch and console path;
- `unit_speak` installs or queues the selected item, accounts for displaced
  speech through its private helper, initializes every timer/state field, and
  derives the duration from the sound definition.

All three bodies use existing typed unit and tag accessors. The full dialogue
tag layout is completed locally in `unit_dialogue.c`, where its field offsets
are consumed; it is not exposed through a shared header. The lookup table is
likewise declared at translation-unit scope. The existing public declarations
in `units.h` remain unchanged. The sound-duration expression follows the
repository's existing signed-timer conversion style without changing the
shared sound-definition type.

No address-named owner, synthetic anchor, forced/manual inline, volatile
scheduling device, assembly, optimizer pragma, raw byte emission, semantic
exception, configuration change, or parked-function entry is introduced.

## Rejected zero-credit candidates

Three optional adjacent reconstructions were tested and then removed rather
than retained as misleading exact-looking code:

| Function | Target padded bytes | Rejected result | Credit |
| --- | ---: | --- | ---: |
| `_unit_notify_impulse_sound` | 240 | structural body remained normalized-SHA residual | 0 |
| `_unit_scream` | 384 | natural candidate emitted 400 bytes and remained residual | 0 |
| `_unit_describe_speech` | 272 | structural body remained normalized-SHA residual | 0 |

None is parked by this packet, and no residual implementation for them is left
in the source. `unit_make_damage_sound` was not attempted.

## Owner and consumer audit

An initially considered shared-header completion was rejected during the
consumer sweep because even a harmless-looking declaration perturbed VC7's
large-`units.c` schedule. Paired header-only compiler probes showed that adding
the lookup `extern` alone changed exactly `_unit_preprocess_node_orientations`;
the other 219 emitted Units functions and every function in dialogue
definitions, AI communication, and AI debug remained strict-equal between the
probes. On current canonical the changed Units body is exact, so the declaration
is deliberately translation-unit-local to avoid a 189-to-188 regression. The
final shared `dialogue_definitions.h`, `units.h`, and `sound_definitions.h`
payload hashes equal their branch-base blobs and are not staged. The final
packet therefore changes no shared consumer contract.

The five dialogue-header consumers were nevertheless rebuilt and audited:

| Translation unit | Final donor-branch strict census | Packet loss |
| --- | --- | ---: |
| `source/units/dialogue_definitions` | 2 exact / 0 residual / 0 unwritten | 0 |
| `source/units/unit_dialogue` | 8 exact / 2 residual / 4 unwritten | 0; +3 exact |
| `source/units/units` | 188 exact / 1 residual / 0 unwritten | 0 |
| `source/ai/ai_communication` | 37 exact / 2 residual / 9 unwritten | 0 |
| `source/ai/ai_debug` | 56 exact / 4 residual / 0 unwritten | 0 |

This donor branch predates the later canonical declaration-owner repair that
raises Units to 189/189. Because the final packet stages neither the dialogue
header nor `units.c`, it does not overlap that repair; canonical integration
must retain the 189/189 Units census.

`unit_dialogue.obj`, `dialogue_definitions.obj`, and `ai_communication.obj`
pass the forbidden-emitted-symbol guard. The donor baseline already emits an
inherited `point_from_line3d` copy from the large Units/AI-debug compilation
context; this packet neither changes those sources/headers nor adds a copy to
`unit_dialogue.obj`. The canonical batch must preserve its already documented
inherited-helper adjudication rather than count it as new packet output.

## Validation

- Focused hardened gate: all three claimed functions are strict exact.
- Whole `unit_dialogue.obj` gate: 8 exact, 2 residual, 4 unwritten; forbidden
  `point_from_line3d` emission absent.
- Rename-stable full-tree comparison: exactly 3 gains / 1,440 padded bytes,
  **zero regressions** across 8,245 functions (6,611 to 6,614 exact on this
  donor branch).
- `ninja all_source`: pass.
- Changed-source fake-match scan with `--fail-on-findings`: zero findings.
- Tool suite: **1,144 passed, 5 skipped, 26 subtests passed**.
- `git diff --check`: pass.
- No scratch, generated, build, configuration, semantic-match, admission, or
  parked-ledger artifact is committed. No push is performed.
