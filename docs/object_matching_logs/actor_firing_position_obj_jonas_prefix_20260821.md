# `actor_firing_position.obj` Jonas firing-position prefix

This Jonas/Codex ledger records one bounded natural four-function experiment
for `source/ai/actor_firing_position.obj` from authoritative campaign commit
`7d2ff479dde012f94cc08857e4cc74650b85b9af`. All four functions were strict
on the first emitted candidate object and are retained. The object advances
from 0/27 to 4/27 strict functions and remains `NonMatching`.

## Scope and provenance

- Baseline `source/ai/actor_firing_position.c` blob:
  `612ee969e79e4efc21cf647363ee649e9db9861c`.
- Retained source blob:
  `4a0e7c6311c0fb9b669f7aae049d2c9ffb868c78`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `1785e9de937184c51550a6ecd18bc637ee70ba763ad17e4427fc3ee29db8f568`.
- Pre-commit retained candidate-object SHA-256:
  `9cbb6b9a6ece9f58cb3afd0ff0f6c02f3caf6a84a4d617cdc633848e8c8f3c51`.
- The clean local HCEA semantic oracle is pinned at
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`. Its four exact-name source
  blobs are `78f4ee4c9eb9f6af2ac5ca2f50ed1781f39d4b04`
  (`actor_get_firing_position_group`),
  `3ad2a6841eceaeed9c818a4807af802034bf0b9a`
  (`actor_clear_discarded_firing_positions`),
  `798c166776506dc518c4ebf1ca3223d70e4299d4`
  (`actor_discard_firing_position`), and
  `1bfe12f39b28e1f62e4d60a9c995d091665f8c04`
  (`actor_firing_position_discarded`).
- The local January-x86 reconstruction corroborator is pinned at Stian
  C-series commit `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its relevant
  `actor_firing_position.c` and `actor_looking.c` blobs are
  `184f42171b529d3d61c8b3fa10897be6a0fe0d24` and
  `f841012d620ee464349fd175801a3418337a859d`.

HCEA supplies the semantic bodies, domain names, and later typed field roles.
January COFF bytes, relocations, strings, assertion line, and x86 field loads
remain authoritative. Stian's source is supporting disassembly annotation,
not production code: none of its raw-address or byte-offset access is copied.

The retained wave changes `source/ai/actor_firing_position.c`, its sole
translation-unit-local caller declaration in `source/ai/action_guard.c`, and
this newly created ledger. No header, configuration, frozen-object source,
or pre-existing Markdown file is edited.

## Bounded compile result

Exactly the requested four functions were compiled together as one natural
typed candidate. The first compiler invocation stopped before emitting an
object because VC7 does not accept array-index member syntax inside its
`offsetof` macro. Replacing that proof with the mathematically identical sum
of the aggregate and member offsets changed no function body or source shape.
The next invocation emitted the sole candidate object; no body, declaration,
statement order, lifetime, or expression was tuned or retried.

All four functions were strict on that object:

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_get_firing_position_group` | 275 | 288 | 9 | `64860a154390022194ec5fa053d4d3962bca102232aace4ce960b4c24d02e1f8` |
| `_actor_clear_discarded_firing_positions` | 95 | 96 | 2 | `894250398575e398c29f91fd2d7610d5fd562dc336c2a945edbe054884ca5510` |
| `_actor_discard_firing_position` | 192 | 192 | 5 | `bdb732a2b36170a04873e23aa8db7ad393f1bb0a920c783f2aa155bfc9a2ee39` |
| `_actor_firing_position_discarded` | 78 | 80 | 2 | `fb87683ada5d62f8cf580c03b7ae9030add049c75260ac40915a76c973509f8e` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends. The retained prefix contributes
640 meaningful code bytes, 656 padded bytes, and 18 relocations. There was no
nonexact body to remove. The target object's remaining 23 functions and all
1,260 target non-code bytes remain outside this wave's credit.

## Authenticated typed layout

Compile-time checks bind every accessed aggregate to the January layout:

- `firing_position_definition` is 0x18 bytes, with its typed `real_point3d`
  position first;
- `scenario.ai_encounters` is at `+0x42C`;
- `encounter_definition` is 0xB0 bytes, with `squads` at `+0x80` and
  `firing_positions` at `+0x98`;
- `squad_definition` is 0xE8 bytes, with its group array at `+0x54`;
- actor encounter, squad, searching, and defending fields are at `+0x34`,
  `+0x3A`, `+0x98`, and `+0x374`;
- the four-byte discarded-position record and four-entry ring are proved at
  cursor `+0x3C6`, ring `+0x3C8`, first index `+0x3CA`, valid flag `+0x3D8`,
  temporary flag `+0x3D9`, and cached point `+0x3DC`.

The production bodies use `actor_get`, `global_scenario_get`, and
`TAG_BLOCK_GET_ELEMENT`; no address is reconstructed. The group accessor
retains January's assertion spelling and source line 1412. Existing
`actor_discard_firing_position` consumers have the same long/short/boolean
contract. The sole current group-accessor call passes normal mode `FALSE`.
HCEA and the January body establish the third argument as the short
`group_selection_mode`, so the translation-unit-local declaration in
`action_guard.c` was corrected from a byte boolean to that same short type.
This removes a cross-translation-unit incompatible-function-type boundary;
the forced caller rebuild leaves all twelve accepted `action_guard.obj`
functions byte exact.

## Policy and validation

The retained code is readable, typed, defined C with explicit returns. It
contains no assembly, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, raw integer-address access, pointer/integer
reconstruction, union pun, signed-overflow match, synthetic anchor, or
byte/code-generation forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass (568 actions).
- Direct hardened comparison: all four requested functions pass.
- Semantic audit: 470 units, 3,998 functions evaluated, 3,858 semantic exact,
  101 hidden exact, 36 ordinary-only, 3,919 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,906/11,060 exact functions,
  472,883/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes. This wave adds four functions and 640 meaningful code bytes with no
  data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

The pre-wave fail-closed manifest reports exactly these four functions as
`NEWLY_EXACT`, with `changed_nonexact: []`. The intended assertion-literal,
compiler-debug, and symbol-owner additions are surfaced for review rather
than hidden by an adjudication. A clean committed-state snapshot/check and a
forced object/report rebuild close that enlarged structural surface before
handoff. No push is performed.
