# `ai_communication.obj` bounded Jonas code wave

## Result

This wave starts from authoritative integration commit
`6cd2dcd766c7fd71f4ace7f8665c5d473f190d67`. It keeps the complete typed-data
checkpoint and its two accepted functions, attempts exactly eight small
source-backed function bodies in one candidate compile, and retains the three
that independently compare strict-exact:

| Function | Meaningful code | Padded section | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_ai_communication_get_type_name` | 35 | 48 | 2 | `7387241d7c4e1643bc9ead172ce469f0f2ad3802fc871a275e08726a2cf8cbb8` |
| `_ai_communication_get_type_by_name` | 55 | 64 | 2 | `0fe64c3167aa951120ec4509c4ec01a1397ee35bb5a7b5a50095497584de2df1` |
| `_code_00031550` | 17 | 32 | 1 | `0b5766f437b2066a2ed12589654660bc56e4f7cea326352b372ad803039e29d8` |

The admitted delta is 107 meaningful code bytes, 144 padded code bytes, five
relocations, and three functions. The type-name accessor also naturally owns
the target's eight-byte `"<error>"` literal COMDAT. The object remains partial;
no completion label, semantic exception, data exception, or parked record is
added.

## Forty-eight-function inventory

The January split object contains the following 48 code sections. `baseline`
means strict-exact before this wave, `gain` means independently strict-exact in
the bounded candidate compile, and `absent` means no retained definition.

| # | Function | Padded bytes | Final state |
|---:|---|---:|---|
| 1 | `_ai_communication_initialize` | `0x150` | absent |
| 2 | `_ai_communication_dispose` | `0x10` | baseline |
| 3 | `_ai_communication_initialize_for_new_map` | `0x110` | absent |
| 4 | `_ai_communication_dispose_from_old_map` | `0x10` | absent |
| 5 | `_ai_communication_get_type_name` | `0x30` | gain |
| 6 | `_ai_communication_get_type_by_name` | `0x40` | gain |
| 7 | `_ai_communication_packet_new` | `0x60` | absent |
| 8 | `_code_00031390` | `0x70` | absent |
| 9 | `_code_00031400` | `0x70` | absent |
| 10 | `_code_00031470` | `0x50` | absent |
| 11 | `_code_000314c0` | `0x90` | absent |
| 12 | `_code_00031550` | `0x20` | gain |
| 13 | `_code_00031570` | `0x40` | absent |
| 14 | `_code_000315b0` | `0xB0` | absent |
| 15 | `_code_00031660` | `0x40` | absent |
| 16 | `_code_000316a0` | `0x40` | absent |
| 17 | `_code_000316e0` | `0x1A0` | absent |
| 18 | `_actor_communication_team` | `0x40` | absent |
| 19 | `_code_000318c0` | `0xB0` | absent |
| 20 | `_code_00031970` | `0x50` | absent |
| 21 | `_ai_conversation_status` | `0x110` | absent |
| 22 | `_ai_conversation_line` | `0x60` | baseline |
| 23 | `_ai_conversation_advance` | `0x90` | absent |
| 24 | `_ai_conversation_finish` | `0x190` | absent |
| 25 | `_code_00031d50` | `0x130` | absent |
| 26 | `_code_00031e80` | `0x1B0` | absent |
| 27 | `_code_00032030` | `0x290` | absent |
| 28 | `_code_000322c0` | `0x30` | absent |
| 29 | `_code_000322f0` | `0xD0` | absent |
| 30 | `_actor_communication_update` | `0xF0` | absent |
| 31 | `_code_000324b0` | `0x80` | absent |
| 32 | `_code_00032530` | `0x2A0` | absent |
| 33 | `_ai_communication_get_player_rating` | `0x340` | absent |
| 34 | `_ai_conversation_stop` | `0x90` | absent |
| 35 | `_ai_conversation_actor_deleted` | `0xD0` | absent |
| 36 | `_ai_conversation_unit_died` | `0x170` | absent |
| 37 | `_code_00032de0` | `0x800` | absent |
| 38 | `_ai_communication_started` | `0x2C0` | absent |
| 39 | `_ai_communication_notify` | `0x210` | absent |
| 40 | `_code_00033ab0` | `0x390` | absent |
| 41 | `_code_00033e40` | `0xC0` | absent |
| 42 | `_code_00033f00` | `0x120` | absent |
| 43 | `_code_00034020` | `0x6D0` | absent |
| 44 | `_code_000346f0` | `0x450` | absent |
| 45 | `_ai_communication_finished` | `0x630` | absent |
| 46 | `_ai_conversation` | `0x150` | absent |
| 47 | `_ai_conversation_update` | `0x260` | absent |
| 48 | `_ai_communication_event` | `0x1F80` | absent |

Final strict state is therefore 5/48 functions: the two baseline functions and
the three gains above.

## Evidence and bounded candidate compile

The recovered `global_communication_type_names[57]` table and January local
disassembly prove both name-query bodies directly. The accessor returns
`"<error>"` outside `[0, 57)` and otherwise indexes the pointer table. The
reverse query walks all 57 entries with `csstrcmp`, retaining the matching
short index or `NONE`.

The typed `global_reply_table[46]` supplies the common callback type
`boolean(long, ai_information_packet *, long)`. HCEA identifies the nine
callback semantics, while January disassembly remains authoritative for the
Xbox implementation. `_code_00031550` is HCEA's `reply_filter_fighting`: it
discards the first two callback arguments and returns
`actor_is_fighting(reply_actor_index)` directly. A TU-local ordinary-C
prototype provides the proven return type without changing a shared header.

The one candidate compile contained exactly these eight bodies:

1. `_ai_communication_get_type_name`
2. `_ai_communication_get_type_by_name`
3. `_code_00031390` (`reply_filter_close`)
4. `_code_00031400` (`reply_filter_not_close`)
5. `_code_00031470` (`reply_filter_searching`)
6. `_code_00031550` (`reply_filter_fighting`)
7. `_code_00031660` (`reply_filter_no_certain_target`)
8. `_code_000316a0` (`reply_filter_flee_leader`)

The first, second, and sixth bodies were strict. The other five were withdrawn
completely after that comparison, with no residual tuning:

| Rejected body | Target padded | Candidate padded | First strict evidence |
|---|---:|---:|---|
| `_code_00031390` | 112 | 96 | size and relocation-address drift |
| `_code_00031400` | 112 | 96 | size and relocation-address drift |
| `_code_00031470` | 80 | 80 | normalized bytes differ; relocations begin one byte late |
| `_code_00031660` | 64 | 64 | normalized bytes differ; relocations begin three bytes early |
| `_code_000316a0` | 64 | 64 | normalized bytes differ; first call relocation is three bytes early |

No inline assembly, volatile access, force-inline directive, pragma, compiler
barrier, undefined behavior, raw address, type pun, byte forcing, synthetic
anchor, or shared-header edit is present.

## Typed-data and baseline preservation

The previously recovered sections remain strict under the hardened COFF
comparator:

| Section | Bytes | Relocations | Owners | Normalized SHA-256 |
|---|---:|---:|---:|---|
| main `.rdata` | 6,336 | 36 | 19 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | 3 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |

The 19 read-only owners are the five priority/timing arrays,
`communication_unit_prefer_silent_time`, `communication_timer_tolerances`,
`communication_play_delays`, the nine remaining timing/rating scalars,
`global_dialogue_table`, and `global_reply_table`. The three writable owners
are `global_communication_priority_names`,
`global_communication_team_names`, and `global_communication_type_names`.
Names, offsets, types, storage classes, raw bytes, and all 105 relocation
records remain equal. This preserves all 22 owners and all 6,612 checkpoint
bytes.

The two baseline functions also remain independently strict:

| Function | Meaningful code | Padded section | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_ai_communication_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ai_conversation_line` | 89 | 96 | 4 | `ceb9598bc8445eec6c88d8648239dc886b08589bfb8ee15ef7da19524fae0410` |

The pre-wave translation-unit manifest reports only the three reviewed
`NEWLY_EXACT` functions, expected section/symbol-inventory changes from adding
their definitions and literal, and `changed_nonexact: []`. Direct hardened
comparison proves both accepted baselines and the two checkpoint data sections
unchanged.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Strict semantic report: 470 units scanned, 3,954 functions evaluated,
  3,875 accepted exact, zero unit errors.
- Progress: pass; Halo reports 3,695/7,574 credited functions and libcmt
  167/476.
- Object-admission audit: zero candidates, zero revocations, with only the
  inherited `source/shell/shell_xbox` contradiction.
- Parked-function validation: 3 active, 0 stale, 0 invalid.
- Tooling suite: 179/179 tests pass.
- Frozen-object A/B from clean commit `6cd2dcd7`: all 172 defined code sections
  and all 431 non-debug non-code sections in `vehicles.obj`, `ai_debug.obj`,
  `ai_script.obj`, `actions.obj`, and `units.obj` remain strict-identical.
  Source, configuration, and existing logs for those units are untouched.
