> Packet18 final disposition: all three source/header probes restored, zero admission. Qualification packets precede their measured results; production remains wave17.

# Packet18: original private-function/header census

**RELEASED. `object_update` qualified for one bounded root-owned test; the completed test was byte-inert and was not admitted.** The remaining intersections are a census, not approved linkage changes or gain predictions. Baseline is `ab648e4e5ef776fdf6eacddb4de3b391a85dd931`. This evidence lane prepared no source shape, edited no production/header/tool file, and invoked no C compiler; root's separate one-shape test is recorded below.

`census.py` parses only the original **Static symbols** blocks in the August and September beta MAPs, retaining exact owner, flags, address, line and input hashes. It intersects ordinary C names with top-level external header declarations, masking comments, strings and preprocessor directives and excluding static/inline definitions and macros. The resulting 133 intersections span 25 headers; all have current external definitions, none is missing. Of these, 103 appear private in both MAPs, 102 of those have no recorded cross-owner C reference, and none carries an earlier inline `i` flag. These counts do not independently establish January private linkage.

Read-only `ninja -t deps` supplies actual recorded compiler consumers; every included dependency entry was VALID. No build or regeneration was run. Full consumer/residual lists, exact source references and source hashes are in `census.json`. Exposure totals overlap and are not additive or predicted matching gains.

| Review order | Header candidate | Consumers | Residual bytes in those consumers | Disposition |
|---|---|---:|---:|---|
| 1 | `objects.h`: `object_update` | 150 | 238,859 | Qualified below; sole proposed test. |
| 2 | `players.h`: `player_teleport_internal`, `player_handle_powerup_equipment` | 73 | 143,691 | Both earlier private, own-TU current references; not individually qualified here. Historical split-linkage rationale needs primary adjudication, not automatic removal. |
| 3 | `bipeds.h`: `biped_snap_facing`, `biped_update_airborne`, `biped_update_moving` | 23 | 114,530 | Both earlier private; later source and residual history exist, but no additional proposal in this lane. |
| 4 | `ai_communication.h`: `ai_conversation_new` | 21 | 95,260 | Both earlier private, one current own-TU caller; not qualified. |
| 5 | `props.h`: `prop_new_blank` | 28 | 94,228 | Both earlier private but no current source caller detected; do not remove its already-exact selected owner merely to clean a declaration. |

The roster above ranks shared-header reach for potential future primary review. It is not a queue of compiler tests, and no generic declaration-order or private-linkage sweep follows.

## Qualified object_update evidence

`object-update-qualification.json` and `qualify_object_update.py` preserve the full bounded proof, plus three raw disassemblies. Both original records carry exactly `f`, not `fi`, and name `objects.obj` beneath Static symbols: August line 18840, September line 20057. Both functions span 432 bytes to the next same-owner function, `object_compute_node_matrices_recursive`.

The current January target and base are already strict exact at **432 padded bytes / 23 relocations**. September's actual XBE bytes have the same instruction layout. Masking only those 23 relocation operand positions leaves one byte difference: January's `push 2508` versus September's `push 2503` at function offset `+0x70`, the assertion source line. All **19 direct and recursive calls** independently resolve to the expected names in each original MAP, including two recursive calls to `object_update`. The named header-data reference is checked through the original MAP segment/offset, and both assertion strings are read from the actual XBE. The private `object_globals` address lacks an earlier MAP data name; its identical instruction/use role is correspondence, not a recovered private-data declaration. No diagnostic normalization grants strict cross-build matching credit.

Fresh January PDB reads find no `object_update` name bytes in global-symbol stream 834 or the whole PDB; positive controls `objects_update`, `object_new` and `object_type_update` occur. The January atlas labels `_object_update` **fuzzy**. This absence alone does not prove private linkage. Combined with two original private owner records and the nearly identical complete September/January operation graph, it supports a natural private-source inference. The split target's external storage class is synthetic and does not refute that inference. Earlier logs using that storage class as original linkage proof are insufficient for this inferred-name case.

An unrestricted whole-source token search independently confirms only six occurrences: the one `objects.h` prototype, the definition in `objects.c:3575`, and calls at 3630, 3637, 4205 and 4225. There is no cross-owner call, function-pointer token or macro-body reference. The first two calls are child/sibling recursion; the last two belong to the object's own update loops. The existing Boolean return/body/type already match January.

The sole coherent proposal is to **remove the external `boolean object_update(long)` declaration from `objects.h` and prefix the existing C definition with `static`**, keeping its body, type and position. Root owns preparation, the 150-consumer test, and complete inherited exact/owner/data/COMMON/point checks. The own-TU exact status is a required control, not prospective credit. A header can influence other compilation contexts, but no gain is predicted from reach alone. If the test yields no admissible complete-function gain, retain no production change. No alternative spelling, placement, dummy dependency, forced emission or target-metadata rewrite is proposed.

The August `P`-filename patch caveat remains; September supplies the strongest body correspondence. Neither original MAP nor this correspondence authenticates a full same-source revision, all original lexical text, or an unrestricted compiler law. The lane is released with **zero compiler calls and zero admitted code bytes**.

## Completed root-owned test

Root's separate [probe receipt](../objects-context/probe.json) records exactly one coherent source/header shape and 150 compiler invocations. Independent receipt reads confirm all 150 consumers compiled, zero gains, zero losses, unchanged function inventories and function bytes/relocations, and equal runtime noncode, COMMON and point ownership throughout. The exact `object_update` owner remained emitted. The receipt confirms restoration of the header and every frozen production source/object hash. Its disposition is **MEASURED_NOT_ADMITTED**, with zero production changes and zero credit. The original qualification JSON and raw primary receipts remain unchanged: the authentic source inference qualified a test but did not deliver an admissible coverage improvement. No other census candidate is advanced by this lane.
