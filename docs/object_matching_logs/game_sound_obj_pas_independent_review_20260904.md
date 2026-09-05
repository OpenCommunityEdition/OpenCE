# Game Sound PAS current-context independent review (2026-09-04)

## Verdict

The six-file PAS replay on canonical commit `18238d906` is approved for
source-level admission as **one strict 112-padded-byte function plus three
honest fuzzy functions**. This is not a whole-`game_sound.obj` Matching
claim: `compute_sound_obstruction` remains unwritten, and the three residuals
receive no exact-byte credit.

This review was read-only with respect to the packet. January split COFF was
the byte and ABI authority. The preserved packet at `9d60117aa`, the HCEA
schema/PDB, and the frozen Opus source were used only as corroboration.

## ABI and source review

- `game_sound_update(real dt)` is declared in `game_sound.h`, and the obsolete
  caller-local declaration is removed from `game.c`. January's caller passes
  the frame delta even though the January callee does not read it.
- `scenario_get_sound_environment` really returns a pointer to the current
  interpolated `sound_environment_definition`. The prior exact body exposed
  this by casting that pointer through `long`; the corrected pointer-to-pointer
  declaration removes the representation-through-integer API without changing
  its ABI or interpolation logic.
- `sound_manager_set_sound_environment` takes that definition pointer and
  copies the definition. `sound_refresh_looping` has the independently
  corroborated six-argument ABI: two `long`s, `sound_source *`, `short`,
  `boolean`, and `real`.
- The two translation-unit-private `sound_source` definitions have the same
  0x40-byte layout and member order. The source uses typed tag, datum, object,
  location, matrix, and bit-vector APIs; refresh modes and sound flags are
  named constants.
- The final frame-index write after `game_looping_sound_delete` is present in
  January itself. The datum lives in fixed data-array storage; this is not a
  donor-derived fake lifetime or a freed-allocation dereference.

The three retained residual bodies are coherent stateful reconstructions.
Their remaining differences are not assigned a guessed compiler cause, and
no declaration-order search, forced inline, volatile/register device, dummy
reference, assembly, or nonsensical coincidental branch is used.

## Complete owner census

January Game Sound contains 31 code owners. The candidate contains exactly
30, corresponding to every implemented target owner; only
`compute_sound_obstruction` is absent. There is no candidate-only code owner
and no `_point_from_line3d` symbol or COMDAT.

The candidate COMMON inventory is:

| Owner | Size | Disposition |
| --- | ---: | --- |
| `_game_sound_globals` | 4 | inherited pointer owner |
| `_game_looping_sound_data` | 4 | inherited pointer owner |
| `_combined_pas` | 64 | authentic PAS bit vector |

`combined_pas` is naturally typed as
`unsigned long[BIT_VECTOR_SIZE_IN_LONGS(MAXIMUM_CLUSTERS_PER_STRUCTURE)]`, or
16 longs / 64 bytes. DIA independently reports `unsigned long[0x10]`; January
assigns the same 64-byte contribution to `source/linker_common.c` at file
offset 5165760. It is not an invented Structure BSP aggregate. No new data or
whole-object credit is claimed.

## Measurements checked

| Function | Target padded / relocs | Candidate padded / relocs | Result |
| --- | ---: | ---: | --- |
| `location_potentially_audible` | 112 / 6 | 112 / 6 | strict exact |
| `compute_combined_pas` | 240 / 10 | 240 / 10 | honest residual |
| `update_potentially_audible_looping_sound` | 624 / 25 | 640 / 25 | honest residual |
| `game_sound_update` | 464 / 31 | 464 / 31 | honest residual |

The new exact owner's normalized SHA-256 is
`2a9fb7234670d74e0373cb40f8370dbbcf7f369bc3310a24dd552f5adafe0885`.
The reviewed physical Game Sound source SHA-256 is
`852c5ce7d635ed9b99b2d516de40e5a9ff7df43d1d6de67776c2588cb55f9c2f`.

The orchestrator's full generated build and rename-stable sweep report one
strict gain, zero regressions, all 189 Units functions preserved, all 255
parks valid, and all tooling tests passing. Those full-tree results are
recorded in
`game_sound_obj_pas_current_context_integration_20260904.md`; this independent
review confirms the source, ABI, ownership, and accounting premises rather
than duplicating their execution.

## Non-blocking naming debt

The private `game_sound_globals` field spellings `update_index` and
`music_looping_sound_index` are coherent and not address-derived. HCEA's later
schema and the recovered behavior suggest `frame_index` and
`background_loop_index`, respectively, but that is corroboration rather than
proof of January's original identifier spellings. It is therefore recorded as
future semantic-name research, not used to block or retune this packet.
