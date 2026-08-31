# `unit_dialogue.obj` speech-priority pair (2026-08-31)

## Result

Two previously unwritten public leaves and their complete initialized-data
owner are strict exact on the first natural typed implementation:

| Owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_get_speech_priority_name` | 48 | 2 | `ff9d68d8043332f9f31ddebaf676336f6eafa234542b4d41ed078aa8f66181e4` |
| `_unit_get_speech_priority_by_name` | 64 | 2 | `f7141b903996da21973dc1026efe60f921efbe0e2e725839e2f63b398705815e` |
| `_global_speech_priority_names` | 44 | 11 | `85759b3811ff7dc47b03792ac85317be51431a3f9e01dcafce317ed736a391b0` |

`unit_dialogue.obj` advances from 1/14 to 3/14 strict functions. The target
and candidate data owners have the same eleven ordered string relocations,
including the January spellings `"script"` and `"exclaim"`.

## Provenance and policy

The January COFF fixes the signed `short` bounds checks, the eleven-entry
table order, the `"<error>"` fallback, and the `csstrcmp` loop. The clean HCEA
tree at commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently authenticates
`enum unit_speech_priority`, its values 0 through 10, and
`NUMBER_OF_UNIT_SPEECH_PRIORITIES == 11` in
`src/headers/unit_speech_priority.h`.

The enum and corrected signed prototype live in the closest associated owner,
`source/units/units.h`; the data owner and bodies remain in
`source/units/unit_dialogue.c`. The now-redundant private
`_unit_speech_idle = 1` definition was removed from
`source/ai/ai_communication.c`; that unit remains at its prior 36 exact
functions. The implementation uses the named enum, ordinary array indexing,
and `csstrcmp`. It contains no raw address, offset overlay, representation
pun, inline assembly, forced inlining, volatile schedule control, compiler
barrier, or synthetic byte-matching construct.

The protected `units.obj` gate remains 189/189 after the shared-header edit.
The combined full-tree function sweep is the final admission gate.
