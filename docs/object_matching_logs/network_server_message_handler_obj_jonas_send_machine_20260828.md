# `network_server_message_handler.obj` machine-send leaf

## Result

`_network_game_server_send_message_to_machine` is strict semantic-COFF exact.
The previously empty unit advances to one exact function and remains honestly
`NonMatching` with 21 functions still unwritten.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_network_game_server_send_message_to_machine` | 61 / 64 | 2 | `f040c90362d58c2f467d98b213a08a60eec07f1df50995eb145ffcea9b59e7bc` |

Direct `tools/coff_compare.py` comparison reports `all_equal: true`. The two
REL32 relocations occur at `+0x0F` and `+0x2E` and address
`_network_game_server_get_machine_connection` and
`_network_connection_write` respectively. The whole-unit gate reports one
exact, zero residual, and 21 unwritten functions.

## Typed behavior and register evidence

The function initializes a false result, asks the server for the selected
machine's connection, and writes the encoded message only when that connection
exists. The local `network_message` view gives the encoded packet a typed
16-bit header; its upper twelve bits hold the message size, matching the
existing encoded-message use in `source/networking/network_game_globals.c`.

The first natural expression had the exact size, instruction count,
relocations, calls, branches, and return paths, but swapped ECX and EDX while
forming the write arguments. Introducing the semantic `word message_size`
local reproduces January's exact schedule: EDX retains the message pointer,
ECX receives the zero-extended header and four-bit shift, and both are pushed
in the required order. No compiler-control device or nonsemantic source
forcing is involved.

January object SHA-256 is
`1a8cd435b187a0c881c9f9c6ec7de919dc90e9005123fa66de32354d99c417fd`;
the retained rebuilt raw object SHA-256 is
`8f7ad6e1c40ae97aac9c52156002d9aa8f2fbbfb38fce4b0197379e468fe18b4`.

## Validation and policy

- Complete `halobetacache_build`, `libcmt_build`, and `all_source`: pass.
- Progress: 574,298 / 2,198,102 overall and 561,384 / 1,770,166 Halo code
  bytes; 4,580 / 11,060 overall and 4,413 / 7,574 Halo functions.
- Semantic audit: 470 units, 4,729 functions evaluated, 4,617 accepted, zero
  unit errors.
- Admission: zero candidates, contradictions, and revocations.
- Parks: 13 active, zero stale, zero invalid.
- Strict board: 274/619 objects, 628,311 / 1,922,669 bytes, and 4,588 / 8,246
  functions after excluding parked and assembly-implemented credit.
- Tooling tests: 205/205 pass; `git diff --check` passes.

The retained implementation is readable typed C with every parameter on its
own line and an explicit terminal return. It introduces no assembly,
attribute, pragma, intrinsic, volatile/register forcing, raw address or byte
offset, undefined behavior, compiler option, object patch, semantic
exception, or comparator exception. `matrix_math.obj`, Vehicles, Units, and
all user-protected AI and game-engine sources are untouched.
