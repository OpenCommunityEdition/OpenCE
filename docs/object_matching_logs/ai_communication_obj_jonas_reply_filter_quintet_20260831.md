# `ai_communication.obj` reply-filter quintet (2026-08-31)

## Result

This packet selectively integrates five small, HCEA-named communication reply
filters from Claude campaign commit `e02971dfd`.  The remainder of the large
donor translation unit is not copied.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_reply_filter_close` | `98 / 112` | `4` | `1eb1eedd6ecd0fe7860dd08fe5d95e8bd48c193a06a2d708102b81a1064ba782` |
| `_reply_filter_not_close` | `98 / 112` | `4` | `2d02b92d3d22dd7a8c3584dd73303195a05d452bf21e0a8d752e539ba6246dd9` |
| `_reply_filter_searching` | `67 / 80` | `2` | `01f766f9d7e7c2ca973c303c0a9ab68d037b89e3b81a07d8c6b81ec0094f52ad` |
| `_reply_filter_no_certain_target` | `50 / 64` | `2` | `ae9d018f55dbb36297a44917fe931bf474f728ca17bb9cade336e3f322c216d8` |
| `_reply_filter_flee_leader` | `53 / 64` | `3` | `68e935c8a7f936a1c81c5a5128c27db4c625bbeae8eda5291633f4ce34019829` |
| **Packet gain** | **`366 / 432`** | **`15`** | |

`ai_communication.obj` advances from **28 exact / 0 residual / 20
unwritten** to **33 exact / 0 residual / 15 unwritten**.

## Source and naming

The five functions retain direct typed predicates:

- close/not-close acquire the reply actor's prop relative to the original unit
  and test the named five-unit distance boundary plus line-of-sight states;
- searching accepts the canonical uncover-position and search actions;
- no-certain-target uses owner-local `_actor_mode_combat` and
  `_actor_combat_status_certain` constants rather than raw 3/4 values; and
- flee-leader requires a fighting actor of `_actor_elite` type.

HCEA `reply_filter_*.c` owners independently authenticate all five names,
signatures, fields, and behavior.  January's object supplies the authoritative
PC function shapes and data-table relocations.

All five functions are source-private.  Their target aliases are marked
`static: true`, and target/base COFF both emit storage class 3 and function type
32.  The file-local declarations, definitions, three existing calls, all 31
reply-table references, and symbol inventory comments use the semantic names;
none of the five old code-plus-address names remains.

The complete 6,336-byte constant table section remains strict exact with 36
relocations and normalized SHA-256
`7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc`.
No semantic-match exception is needed.

## Deliberate exclusions

`actor_communication_team` is separately gated by a cross-TU caller and owner
header.  The large event/conversation drivers and remaining address-named
helpers are not imported.  No raw datum offsets, representation punning,
artificial lifetime, assembly, forced inline/noinline, barrier, dead branch,
semantic exception, or completion label is retained.

## Validation

- whole-unit gate: `33 exact / 0 residual / 15 unwritten`;
- direct strict gates: all five functions exact with all 15 relocation
  identities preserved;
- all 28 inherited exact functions remain exact;
- full constant reply/dialogue table owner remains exact;
- full configure/Ninja cache build, libcmt build, progress, and semantic
  progress: pass;
- rename-aware whole-tree verdict diff: five AI gains and the two disjoint
  biped gains, zero unexpected transitions;
- semantic audit: 470 units, 5,044 functions evaluated, 4,936 semantic exact,
  4,946 accepted exact, and zero unit errors;
- campaign total after both small packets: 629,344 / 2,198,102 meaningful code
  bytes and 4,903 / 11,060 accepted functions;
- Halo total: 616,430 meaningful code bytes and 4,736 functions;
- exact data remains 1,866,670 bytes;
- protected `units.obj`: 189/189;
- tooling tests: 222/222 pass;
- parked audit: 25 active, zero stale, zero invalid;
- object-admission state unchanged; fake-match scan reports zero review leads;
  `config/symbols.json` parses and `git diff --check` passes.

No push is performed by this packet.
