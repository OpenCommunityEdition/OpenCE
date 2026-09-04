# `game_sound.obj` Opus scripted-music reconciliation (2026-09-04)

## Result

This packet continues from the admitted impulse/foley checkpoint
`9b224f70c`. Three previously unwritten January owners are reconstructed in
ordinary typed C and compare strict exact as one private-call cluster.

| January owner | Semantic name | Padded / meaningful bytes | Relocations | Verdict |
| --- | --- | ---: | ---: | --- |
| `_code_001b7800` | `looping_sound_definition_is_music` | 96 / 96 | 3 | strict exact |
| `_code_001b7860` | `scripted_music_stop_all` | 208 / 195 | 12 | strict exact |
| `_scripted_looping_sound_start` | unchanged | 144 / 139 | 10 | strict exact |

The bounded gain is **three functions, 448 padded bytes, and 430 meaningful
code bytes**. `game_sound.obj` advances from 23 exact / 8 unwritten to
**26 exact / 5 unwritten**, with zero residuals. No park or whole-object
completion claim is made.

## Authority and provenance

January's split COFF remains authoritative for bytes, private ABI,
relocations, and ownership. The read-only interrupted Opus donor at
`eeedd72e5ba0fc1761519f68cad241fe606320e7` supplied the candidate source
shape. Its prebuilt object independently recorded all three normalized owners
exact. The pinned HCEA research tree corroborates the two private names,
static linkage, and program behavior only; later-version layout and calling
conventions were not imported.

The two anonymous January owners are mapped in `config/symbols.json` as
`_looping_sound_definition_is_music` and `_scripted_music_stop_all`, each with
`"static": true`. HCEA's static function records plus the frozen donor's
unique size/relocation name-gap pairs authenticate those mappings. Their
January normalized hashes before the semantic rename are respectively
`af2569afae3a8287ecb8173844956172e074cacfcd45c84c48483a768dd9e7b1`
and
`14fac40d64220ba7c1e08386be845e88dc0d781ea6a71d3ed740e0b368409c00`.

## Reconstructed behavior

`looping_sound_definition_is_music` obtains the typed looping-sound tag,
walks its typed track block, and returns true when a loop reference resolves
to a sound in `_sound_class_music`. `scripted_music_stop_all` walks live game
looping sounds and requests fixed fadeout for unattached entries whose
definitions are music. `scripted_looping_sound_start` stops an existing
scripted instance, honors `_looping_sound_stops_music_bit`, starts the new
unattached instance, records it on the definition, and marks the live datum
scripted.

The target passes the classifier's private parameter in `EAX`; the ordinary
static C definition emits that convention without annotation or assembly.
The stop-all owner also contains the compiler's natural inlined copy of the
already-exact private stop helper. The retained source still expresses a
normal function call and does not force or simulate the inline. The public
start wrapper deliberately calls `scripted_looping_sound_stop`, matching the
January relocation and the exact frozen donor rather than importing HCEA's
later direct-private call.

The formerly unresolved looping definition and track spans are now owned and
size-asserted in `sound_definitions.h`; this packet uses
`looping_sound_definition_get`, `sound_definition_get`, and
`TAG_BLOCK_GET_ELEMENT` rather than shadow layouts or raw offsets.

## Supporting ownership cleanup

The six already-authenticated `enum sound_class` members were moved unchanged
from `sound_manager.c` to their natural owner, `sound_classes.h`.
`game_sound.c` includes that owner and uses `_sound_class_music`; no numeric
comparison or duplicate local enum remains. No attempt was made to invent the
unknown members of the full enumeration.

The inherited `unattached_looping_sound_start` declaration and definition now
name their second parameter `source_object_index`, matching its direct use as
the object argument to `game_looping_sound_new` and HCEA corroboration. Its
literal bit zero is replaced by the semantic
`_game_looping_sound_unattached_bit` enum constant. These are source-semantic
repairs with identical generated code; the inherited 96-byte owner remains
strict exact.

## Isolated verification

- Joint alias gate for the two pre-rename target owners and the public start:
  **3 exact / 0 residual / 0 unwritten**.
- Full `game_sound.obj` gate: **26 exact / 0 residual / 5 unwritten**.
- `sound_manager.obj` before and after enum relocation:
  **27 exact / 6 residual / 32 unwritten**, unchanged.
- Every prior direct `sound_classes.h` consumer was checked: `sound_classes`
  remains **12/12 exact** and `sound_definitions` remains **7/7 exact**.
- `unattached_looping_sound_start` remains strict exact after the semantic
  parameter and flag cleanup.
- `tools/fake_match_scan.py`: zero review leads across the four touched
  C/header files.
- The gated `game_sound.obj` emits no `point_from_line3d` symbol or COMDAT.

This worker used only the isolated `tools/campaign/gate.py` compiler path. A
full canonical build, rename-stable sweep, tests, and header blast-radius
validation remain the admitting orchestrator's responsibility.

No raw offset, shadow tag type, false cast, fake dependency, contradictory
branch, volatile/register forcing, pragma, forced inline/noinline, undefined
behavior, representation pun, or inline assembly is introduced. Parameters
remain one per line, no-argument functions use `void` on its own line, and
every function retains an explicit terminal return.
