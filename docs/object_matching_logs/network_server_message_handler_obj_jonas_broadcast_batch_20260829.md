# `network_server_message_handler.obj` broadcast sender batch

## Result

Three additional public senders are strict semantic-COFF exact. The unit
advances from 1/22 to 4/22 exact functions and remains honestly `NonMatching`
with 18 functions still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_server_send_message_to_all_machines` | 262 / 272 | 16 | `b0d2023efabe29d554f5c1b5e2d9dab4135637b6027ddad611d44fa5c1ad5cd8` |
| `_network_game_server_send_player_joined_info_ingame` | 144 / 144 | 10 | `256082a71af0ad7316276c45ff4bd90a5db932dfe6a70b83d3c28e10b6455aae` |
| `_network_game_server_send_game_data_pregame` | 188 / 192 | 14 | `d467902488b57500c26c040a3f9ee8b40623d12dc31899a8a45c839571456544` |

Direct `tools/coff_compare.py` comparisons report `all_equal: true`. The
whole-unit gate reports four exact, zero residual, and 18 unwritten functions,
proving that the previously retained machine-send leaf remains exact. This
wave adds 594 meaningful and 608 strict padded code bytes.

## Typed behavior and ABI

The all-machines sender validates the server and encoded message, extracts the
typed 16-bit header length, and iterates the four server-side machine slots.
For each joined machine with an active connection it copies at most 0x600
bytes into a local message buffer and invokes the existing five-argument
connection writer. January's assertion expressions, line values, event text,
loop topology, delayed stack cleanup, and all 16 relocation identities arise
naturally from the retained source.

The player-joined sender copies the existing 0x20-byte `network_player` into a
message payload, creates `_message_server_add_player_ingame`, broadcasts it,
and preserves the authenticated failure paths. The pregame sender uses the
network message system's established 0x434-byte game-settings payload shape,
copies the server's typed game state into it, creates
`_message_server_game_settings_update`, and broadcasts it. Its opaque payload
member represents a serialized message boundary; no field is accessed by raw
offset and no server object is reinterpreted.

January object SHA-256 is
`1a8cd435b187a0c881c9f9c6ec7de919dc90e9005123fa66de32354d99c417fd`;
the retained rebuilt raw object SHA-256 is
`74268ce4f1922285fbd7136e1eca1216715fe8d874cf850bd6e95746cd69b4d5`.

## Validation and policy

- Complete `halobetacache_build`, `libcmt_build`, and `all_source`: pass.
- Progress: 574,892 / 2,198,102 overall and 561,978 / 1,770,166 Halo code
  bytes; 4,583 / 11,060 overall and 4,416 / 7,574 Halo functions.
- Semantic audit: 470 units, 4,732 functions evaluated, 4,620 accepted, zero
  unit errors.
- Admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Strict board: 274/619 objects, 628,919 / 1,922,669 bytes, and 4,591 / 8,246
  functions after excluding parked and assembly-implemented credit.
- Tooling tests: 205/205 pass; `git diff --check` passes.

The retained implementations are readable typed C with every parameter on its
own line and explicit terminal returns. They introduce no assembly,
attribute, pragma, intrinsic, volatile/register forcing, raw address or byte
offset, undefined behavior, compiler option, object patch, semantic
exception, or comparator exception. `matrix_math.obj`, Vehicles, Units, and
all user-protected AI and game-engine sources are untouched.
