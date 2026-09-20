> Archived research proposal: NOT LANDED; zero exact gain. Production remains at wave3.

# Wave 4 communication independent review — 2026-09-20

**Research-only, unlanded, zero exact credit.** The typed-array, diagnostic, and danger-lookup changes are supported by the evidence below and preserve the inherited exact functions. The final combined function still differs from January. Root has explicitly chosen not to integrate zero-credit Wave 4 source and is restoring the temporary allegiance header. This review does not recommend production admission or whole-object completion.

Frozen source baseline: `7a6ae71e614638753d04d6d04acbec56c7b8f636`. Independent artifacts: `scratch/astra-wave4/communication/independent-review.{py,json,log}`, `header-combined-independent.{py,json,log}`, `typed-arrays-independent.patch`, `combined-independent.patch`, and `primary-independent-excerpts.asm`. The scripts only read existing sources, artifacts, and the original PE, and write review receipts. No compiler, Ninja, or test suite was invoked by this review.

## Typed arrays: supported local types, no artificial support

The raw DIA receipt `scratch/astra-debug-evidence/hcea_ai_communication_event_sym.txt` explicitly records `short[0x2][0x8][0x2] speech_enable_after_ticks` and `unsigned char[0x2][0x8][0x2] speech_disabled`. Its HCEA PDB identity is SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`. Current `boolean` is `byte`, which is `unsigned char`.

The candidate changes the corresponding existing 32-element arrays to `[team][priority][distance]`, with unchanged footprints of 64 and 32 bytes. The row-major offset is exactly the old `2 * (8 * team + priority) + distance` for all 32 cells. Priority-none remains index zero. The later access uses the same `near_player`, produced by a comparison, and retains explicit team/priority assertions. Whole-array clearing still precedes every read, including the authentic zero-before-write timer check. No new uninitialized read or different initialization scope is introduced.

The debug-only `speech_disabled_reason` remains flat because its multidimensional type was not established. Both existing `slot` locals still index that array; they are not unused declarations or register-shaping filler. The independent patch reconstruction proves no other function body changed. The HCEA record corroborates this natural type recovery; it is not claimed as an original January source record.

## Diagnostics: January graph plus fresh raw later-build verification

January `_ai_communication_event +0x329..0x379` tests `ai_debug+149`, selects the observed/unobserved and betrayal/accident strings, loads the two team names, and calls `console_printf`. The added source prints exactly that report under `ai_debug.print_allegiance` before the observed-only incident call.

January `+0x3c1..0x409` separately tests the debug flag and `!broken`, calls `game_allegiance_get_incidents` at `+0x3d7`, reads a signed word threshold, maps `NONE` to 999, sign-extends the short return, and prints the still-held allegiance report. `notify_immediately`, `betrayal`, `observed`, and `broken` retain their initialized dataflow. The owner initializes `threshold = NONE`, conditionally replaces it, and writes `*incident_threshold` whenever the pointer is nonnull. Passing a valid local address therefore initializes the output on every return path.

The potentially redundant `broken ? "broken" : "still holds"` inside a `!broken` guard is authenticated by raw later-build instructions, not added for code pressure. At `0x48dbc2`, a nonzero broken value exits the report. At `0x48dc13..0x48dc34`, the same stored byte is nevertheless tested again to select those two strings. Optimization naturally drops the unreachable choice in January. No extra emitted string or helper is required.

This review reread the original `halo_cache_symbols.exe` PE (SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`) and independently decoded `0x48da93..0x48dc59`, including the raw string pointers. The full decoded bytes and eight resolved string literals are retained in `header-combined-independent.json`. The saved Ghidra assembly was checked separately. Cross-build source-revision equivalence is not asserted.

## Danger lookup: genuine refreshed datum access

January `+0x15bb..0x160a` tests the subject handle against `NONE`, loads `_actor_data`, calls `_datum_get`, tests the returned actor's danger type at `+0x280`, and checks/uses its danger object at `+0x28c`. Reusing the earlier pointer omitted that operation. The candidate restores ordinary `actor_get(subject_actor_index)` in a nested scope and uses the returned, initialized pointer for both fields.

Fresh later PE bytes at `0x48ff85..0x49000c` independently show the handle guard, actor-data argument, datum-get call at `0x48ff9c`, a distinct local pointer store at `0x48ffa4`, and the danger/object accesses. Later field offsets differ (`0x244`, `0x24c`), so the candidate correctly uses January's existing typed owner rather than copying raw offsets. This is an authenticated operation graph, not a fake call, added arithmetic, manual register assignment, or assembly substitute.

## Allegiance owner declaration and twelve-consumer control

The proposed declaration is exactly the existing definition in `source/game/game_allegiance.c:264`: `short game_allegiance_get_incidents(short, short, short *)`. Direct hardened comparison independently confirms that owner remains strict exact: 128 padded bytes, one relocation, normalized SHA-256 `5d01487de94f9d049e7fe1f8c26a51d502a5bf9ab5a893a2c49530add3dd427b`. January's caller provides the separate signed-word ABI evidence above.

The proposal adds only this declaration to the genuine allegiance owner header. Communication includes that header and removes three compatible local foreign prototypes. The existing `main/console.h` supplies the genuine console declaration. There is no storage definition, new helper, changed layout, or header dependency introduced by the new short-only signature.

All eleven pre-existing direct C consumers and the proposed communication consumer were audited against root's frozen pre-header artifacts. Each frozen source is verified against commit `7a6ae71e`, including source hashes. The twelve consumers are actor_stimulus, actors, ai, ai_script, encounters, aim_assist, game, game_allegiance, game_statistics, motion_sensor, damage, and ai_communication. No transitive header consumer was found in the frozen include inventory.

**All 462 strict controls, 557 function owners, and 1,343 non-debug sections are preserved.** Every complete non-debug section retains bytes, ordered relocations, flags and auxiliary records. COMMON and each consumer's point-helper presence/absence are unchanged. Some compiler-local `$L` numbers change after adding the prototype; their section offsets, storage classes and types remain identical, and the complete code comparison remains exact. These label renames are explicitly recorded rather than presented as raw symbol-table identity. Debug-only output-path differences are retained separately.

## Four measured source shapes, no exact gain

| Shape | Event padded size / relocations | Exact controls | Credit |
| --- | ---: | ---: | ---: |
| Frozen baseline | 7,776 / 319 | 44 | 0 |
| Typed arrays | 7,776 / 319 | 44 | 0 |
| Diagnostics | 7,952 / 333 | 44 | 0 |
| Danger refresh | 7,808 / 321 | 44 | 0 |
| Combined | 7,984 / 335 | 44 | 0 |
| January target | 8,064 / 336 | — | — |

Combined normalized SHA-256: `4fc46052653a15224d4e1a48876245c6fe9bd5b131246762e0e647d8c2ff665b`; target: `7a16e8d71ff2a5b3d99c76a6059a65dac3bc1750abde8ce07b4210fe3224ad2c`.

Each shape changes only the event function; all other 52 emitted function owners retain complete bytes/relocations. All 226 inherited non-debug non-code sections remain identical. COMMON stays empty, `_point_from_line3d` stays absent, and both baseline and each candidate have zero fake-scan leads. Source review found no new UB, fabricated dependency, or artificial live value.

Diagnostics and combined add exactly **seven January-exact string owners**, totaling 134 raw payload bytes: betrayal (9), accident (9), observed (9), unobserved (11), the incident format (41), still holds (12), and the allegiance format (43). Each complete added section compares equal to its actual January symbol, including bytes and relocation identity. No target-owned matched-data credit is claimed for this still-NonMatching object.

The remaining extent/relocation and instruction differences, including the unresolved actor-team inline context, remain blockers. No constant-spelling probe, flag sweep, source permutation sweep, or further shape was attempted by this reviewer. These artifacts preserve usable evidence for a future exact mechanism; they remain unlanded by explicit campaign direction.
