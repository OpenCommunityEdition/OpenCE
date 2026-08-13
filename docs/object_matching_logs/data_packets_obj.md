# data_packets.obj exact-match log

## Scope and provenance

- Translation unit: `source/memory/data_packets.c`
- January target: `build/split/source/memory/data_packets.obj`
- Candidate: `build/base/source/memory/data_packets.obj`
- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: repository defaults (`/O2 /Oy- /DDEBUG /Dxbox`)
- Authority: January object bytes, COFF relocations, assert strings, and the
  repository's reconstructed packet layouts. Cross-build material is not
  accepted without January corroboration.

## Validated baseline (2026-08-11)

The untouched `14f46f87` baseline is 3/6 strict-exact. The three public
wrappers are exact; the three recursive field walkers remain active.

| Function | Size T/B | Relocs T/B | Reloc identity | Target hash | Base hash | Result |
|---|---:|---:|---|---|---|---|
| `_data_packet_verify` | `272/272` | `23/23` | exact | `b587488214be...` | `b587488214be...` | exact |
| `_data_packet_encode` | `256/256` | `16/16` | exact | `d7f1602034ca...` | `d7f1602034ca...` | exact |
| `_data_packet_decode` | `256/256` | `16/16` | exact (proved alias) | `f67dd668993d...` | `f67dd668993d...` | exact |
| `_code_0010a5f0` | `464/464` | `27/27` | differs | `7ecfeee48d0b...` | `373321ae01f1...` | active |
| `_code_0010a7c0` | `768/768` | `44/39` | differs | `8a525e33293d...` | `dec56de1c3ba...` | active |
| `_code_0010aac0` | `672/672` | `28/28` | differs | `a565486c6862...` | `4210e8448955...` | active |

The ordinary report showed the object's data as complete. The final hardened
owned-section audit confirmed all 14 January-owned `.rdata` COMDATs exact.

## Accepted controls

- The existing public wrapper implementations are retained because they pass
  strict size, normalized-byte, and relocation-destination comparison.
- Recursive encoder/decoder calls remain in the same translation unit; their
  private calling conventions and inlining decisions must be evaluated as an
  atomic cluster.
- January and the independently recovered HCEA topology prove that the
  verifier does not reset `field_size` for version-ineligible fields. This is
  preserved as `BUG (original)`; initializing it before the loop is the safe
  corrected-build alternative but is not the shipped behavior.

## Experiment matrix

Every retained and rejected source family is recorded below. All measurements
use the full six-function strict sweep, not size-only comparison.

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| B00 | untouched `14f46f87` baseline | see table | see table | see table | disassembly audit pending | 3/3 public wrappers exact | baseline |
| E01 | initialize `field_size` once, then `total_size`, instead of resetting it inside the loop | `464/464` | `27/27` | `5a14c0cf7955...` | `+0x09`: candidate allocates the live zero to EDI before the end-sentinel test; target performs the test first | 3/3 public wrappers exact; other walkers unchanged | improved semantic topology, rejected code shape |
| E02 | chained `field_size = total_size = 0` before the loop | `464/464` | `27/27` | `5a14c0cf7955...` | identical to E01 | 3/3 public wrappers exact | compiler-equivalent; rejected |
| E03 | declaration initializers for both verifier size locals | `464/464` | `27/27` | `4c795546f089...` | `+0x09`: still materializes zero in EDI before the sentinel test; only the following store/load schedule changes | 3/3 public wrappers exact | rejected |
| E04 | scoped `word maximum_length = field->count` before `data_decode_string` | `672/672` | `28/28` | `16491304fb03...` | `+0x10d`: emits one `movzx`; target uses `xor reg,reg` plus a partial `mov word` | other five functions unchanged | improved signedness, rejected code shape |
| E05 | correct `data_decode_string` parameter to `word` in the shared declaration/definition | `672/672` | `28/28` | `a565486c6862...` | none | decoder walker and all 15 previously exact `data_encoding.obj` siblings pass; its one pre-existing residual is unchanged | retained; `_code_0010aac0` exact |
| E06 | leave verifier `field_size` uninitialized once, with no per-iteration reset, matching January/HCEA | `464/464` | `27/27` | `7ecfeee48d0b...` | none | all five siblings unchanged/exact-status preserved | retained; `_code_0010a5f0` exact |
| E07 | hoist encoder case locals to function scope in target-like C89 order | `768/768` | `44/39` | `dec56de1c3ba...` | no code change; optimizer reconstructs the same live ranges | 5/5 exact siblings unchanged | compiler-equivalent; rejected |
| E08 | use a named `encoding_state` alias for every encoder call | `768/768` | `44/39` | `dec56de1c3ba...` | no code change; alias is folded | 5/5 exact siblings unchanged | compiler-equivalent; rejected |
| E09 | move the array element cursor initialization before the recursive verifier call | `768/768` | `44/39` | `e07a9f671b66...` | prologue/frame improve to `0x14`; state now caches in EBX like target; recursive array region remains different | 5/5 exact siblings unchanged | retained as evidence-backed lifetime correction |
| E10 | introduce a separate 32-bit remaining-element down-counter | `768/752` | `44/39` | `cbdbe3befc2b...` | array loop changes to an unsigned-long pretest and removes January instructions | 5/5 exact siblings unchanged | regressed; reverted |
| E11 | move the inactive-string `zero` byte to the head of the declaration list | `768/768` | `44/39` | `e07a9f671b66...` | no code change; VC7 still colors it into `[ebp+0xf]` | 5/5 exact siblings unchanged | compiler-equivalent; rejected |
| E12 | remove the folded `encoding_state` alias and pass `state` directly after the E09 cursor-lifetime correction | `768/768` | `44/39` | `4bb1256ab4fb...` | prologue and frame now match January (`sub esp,0x18`); state/field/cursor occupy EBX/ESI/EDI; first residual is the active/inactive switch topology | 5/5 exact siblings unchanged | retained; establishes the correct local/register foundation |
| E13 | spell all six inactive fixed-field cases as separate identical call bodies | `768/816` | `44/46` | `e5078c777c8d...` | VC7 emits the desired direct nine-entry dispatch but retains six copies of the call body instead of cross-jumping them | 5/5 exact siblings unchanged | mechanism proved, over-expanded; reverted |
| E14 | keep `pad+bytes` together, but spell the other four inactive fixed-memory paths separately | `768/816` | `44/46` | `0b633ee4bb63...` | still retains five copies of the call body; a lowering-threshold change alone is insufficient | 5/5 exact siblings unchanged | rejected; superseded by January proof that inactive `pad` is a no-op |
| E15 | correct inactive `pad` to a no-op, independently corroborated by January's direct table and HCEA, while grouping only bytes/shorts/longs/int64/raw | `768/768` | `44/44` | `e955ee30862c...` | relocation identities now all exact; only 27 normalized bytes differ, confined to register/scheduling choices in the active data/raw/array region (`+0x100..+0x148`) | 5/5 exact siblings unchanged | retained; semantic and switch-topology correction |
| E16 | assign the array `element` cursor before loading `element_count` | `768/752` | `44/43` | `06d1edf5e47e...` | changes the early loop branch at `+0x16` and collapses the array loop/code shape | 5/5 exact siblings unchanged | regressed; reverted |
| E17 | restore only `data_size` to the active data-case block, following the strict-exact decoder sibling's source topology | `768/768` | `44/44` | `e955ee30862c...` | byte-identical to E15; lexical scope alone does not rotate the scratch registers | 5/5 exact siblings unchanged | compiler-equivalent; retained for readable narrow scope |
| E18 | restore all four array locals to the array-case block while retaining the E09 pre-verifier cursor assignment | `768/768` | `44/44` | `e955ee30862c...` | byte-identical to E15/E17; lexical scope without initializer topology does not affect coloring | 5/5 exact siblings unchanged | compiler-equivalent; retained for readable narrow scope |
| E19 | initialize `element_count` and `element` at their block-scoped declarations in HCEA order | `768/768` | `44/44` | `e955ee30862c...` | byte-identical to E15/E17/E18; declaration initializers are folded to the same IR | 5/5 exact siblings unchanged | compiler-equivalent; retained as concise readable topology |
| E20 | narrow `element_size` to the recursive array-loop body, matching its actual lifetime | `768/768` | `44/44` | `e955ee30862c...` | byte-identical to E15-E19; the narrower source lifetime is recovered automatically | 5/5 exact siblings unchanged | compiler-equivalent; retained for readable scope |
| E21 | name the short-lived `decoded_data + sizeof(short)` pointer in the data case | `768/768` | `44/44` | `e955ee30862c...` | compiler folds the alias; no byte or register-color change | 5/5 exact siblings unchanged | compiler-equivalent; rejected as unnecessary |
| E22 | restore the inactive-string `zero` byte to its lexical case block | `768/768` | `44/44` | `e955ee30862c...` | byte-identical; the stack byte's lexical scope does not affect volatile scratch coloring | 5/5 exact siblings unchanged | compiler-equivalent; retained for narrow readable scope |
| E23 | HCEA-style direct cursor re-read for the lower-bound data-size correction, while retaining `data_size` for the upper bound | `768/752` | `44/43` | `d95711c22306...` | changes optimizer topology and collapses code/table structure despite equivalent valid-path semantics | 5/5 exact siblings unchanged | regressed; reverted |
| E24 | name the raw-field count in a block-scoped `short` before its encode call | `768/768` | `44/44` | `e955ee30862c...` | alias is folded; scratch-register colors remain unchanged | 5/5 exact siblings unchanged | compiler-equivalent; rejected as unnecessary |
| E25 | name `field + 1` as a one-use `element_fields` pointer for the verifier call | `768/768` | `44/44` | `e955ee30862c...` | alias is folded; verifier argument construction and register colors are unchanged | 5/5 exact siblings unchanged | compiler-equivalent; proceed only by testing its meaningful shared lifetime |
| E26 | reuse the named `element_fields` pointer in both verifier and recursive encoder calls | `768/768` | `44/44` | `e955ee30862c...` | VC7 still recomputes/folds the subfield expression; no live-range or byte change | 5/5 exact siblings unchanged | compiler-equivalent; reverted as unnecessary |
| E27 | use one block-scoped `data` cursor alias for the data-size read and post-header encode source | `768/768` | `44/44` | `e955ee30862c...` | optimizer proves it identical to `decoded_data`; no live-range or color change | 5/5 exact siblings unchanged | compiler-equivalent; reverted as unnecessary |
| E28 | define a one-use `definition` alias only after the array element cursor is born, then pass it to the verifier | `768/768` | `44/44` | `e955ee30862c...` | the late alias is folded back to the stack parameter; its load remains hoisted and colors do not move | 5/5 exact siblings unchanged | compiler-equivalent; reverted as unnecessary |
| E29 | initialize `data_size` at its block-scoped declaration | `768/768` | `44/44` | `e955ee30862c...` | compiler-equivalent to the separate assignment; no scratch-color movement | 5/5 exact siblings unchanged | retained as concise HCEA-like source topology |
| E30 | define `decoded_start` from the parameter first, then define `decoded_data` from it | `768/768` | `44/44` | `e955ee30862c...` | aliases fold to the identical prologue and allocator graph; no color change | 5/5 exact siblings unchanged | compiler-equivalent; reverted to the clearer cursor-first form |
| E31 | share one function-scope post-header `element` pointer between the data and array cases | `768/768` | `44/44` | `e955ee30862c...` | SSA splits the mutually exclusive assignments; no emitted or allocation change | 5/5 exact siblings unchanged | compiler-equivalent; reverted to narrow case-local meaning |
| E32 | use the HCEA/PDB-indicated `short const *` private encoded-data parameter, with necessary byte-pointer casts | `768/768` | `44/44` | `e955ee30862c...` | ABI and IR normalize identically; public wrapper remains exact but residual colors do not move | 5/5 exact siblings unchanged | no January byte proof for the cross-build type; reverted |
| E33 | introduce an explicit signed `long` remaining-element down-counter after encoding the validated short count | `768/768` | `44/43` | `a683cffcd72d...` | loses one relocation and changes the array-loop lowering; the target's 32-bit stack counter is compiler-derived from the short post-decrement | 5/5 exact siblings unchanged | regressed; reverted |
| E34 | spell the array traversal as `for (; element_count > 0; element_count--)` | `768/768` | `44/44` | `e955ee30862c...` | VC7 normalizes it to the identical post-decrement loop and unchanged allocation | 5/5 exact siblings unchanged | compiler-equivalent; reverted to the concise while form |
| E35 | add the C `register` hint to the long-lived array element cursor | `768/768` | `44/44` | `e955ee30862c...` | VC7 ignores the hint under `/O2`; no byte or color change | 5/5 exact siblings unchanged | compiler-equivalent; reverted |
| E36 | make all three private packet walkers' definition pointers `const`-correct | `768/768` | `44/44` | `e955ee30862c...` | all six functions remain byte-identical to E15 status; qualifier does not alter alias scheduling | 5/5 exact siblings unchanged | no January type proof and no code effect; reverted |
| E37 | split the invalid data-size correction into `if` / `else if` branches | `768/768` | `44/44` | `7b2f8fe45a72...` | same calls/size but allocator and block layout diverge from `+0x05` across 503 normalized bytes | 5/5 exact siblings unchanged | strongly regressed; rejected |
| E38 | split the invalid data-size correction into two independent `if` statements | `768/768` | `44/44` | `6f64a76ca15a...` | allocator diverges from `+0x05` across 617 normalized bytes | 5/5 exact siblings unchanged | strongly regressed; reverted to the measured OR form |
| E39 | express the correction as the negation of the preceding valid-range predicate | `768/768` | `44/44` | `e955ee30862c...` | VC7 canonicalizes De Morgan's form to the identical OR graph and colors | 5/5 exact siblings unchanged | compiler-equivalent; superseded by the simpler OR form |
| E40 | assign corrected `data_size` with a ternary single-result expression | `768/768` | `44/44` | `7b2f8fe45a72...` | lowers like E37 and globally changes allocation across 503 normalized bytes | 5/5 exact siblings unchanged | strongly regressed; reverted |
| E41 | split the array element cursor into `element = decoded_data; element += sizeof(short);` | `768/768` | `44/44` | `e955ee30862c...` | optimizer recombines it to the same IR/lifetime and unchanged colors | 5/5 exact siblings unchanged | compiler-equivalent; reverted to one expression |
| E42 | split the data payload cursor into `data = decoded_data; data += sizeof(short);` after the integer call | `768/768` | `44/44` | `e955ee30862c...` | optimizer recombines it to the same pointer expression and color | 5/5 exact siblings unchanged | compiler-equivalent; reverted |
| E43 | spell the data payload as HCEA's typed `(short const *)decoded_data + 1` pointer arithmetic | `768/768` | `44/44` | `e955ee30862c...` | type scaling is canonicalized to the same byte offset; scratch-register colors do not move | 5/5 exact siblings unchanged | compiler-equivalent; reverted to the clearer byte-pointer expression |
| E44 | bind `element = decoded_data + sizeof(short)` to the verifier's `NULL` second argument with a comma expression | `768/768` | `44/44` | `e955ee30862c...` | VC7 hoists the independent first-argument load anyway; the complete object is byte-identical to E15 | 5/5 exact siblings unchanged | legal and sequenced, but compiler-equivalent; rejected as less readable |
| E45 | bind the element assignment to the verifier's first argument with `(element = ..., packet_definition)` | `768/768` | `44/44` | `e955ee30862c...` | the comma dependency is removed during simplification; packet-definition load and all scratch colors remain byte-identical | 5/5 exact siblings unchanged | compiler-equivalent; rejected as less readable |
| E46 | bind the element assignment to verifier argument 3 as `(element = ..., field + 1)` | `768/768` | `44/44` | `e955ee30862c...` | despite matching January's apparent arg4/cursor/arg3 construction order, simplification again restores the identical E15 allocator graph | 5/5 exact siblings unchanged | compiler-equivalent; rejected as less readable |
| E47 | infer a `short *` private return from the target epilogue's live `encoded_packet_size` value and return that parameter | `768/768` | `44/44` | `e955ee30862c...` | ignored call results and the already-live epilogue value make the alternate prototype codegen-identical; HCEA still supports `void` | 5/5 exact siblings unchanged | no January type proof and no code effect; reverted to `void` |
| E48 | strict-exact TIFF donor topology: initialize `element` to the base before the verifier, then add the two-byte header after the call | `768/768` | `44/44` | `19d955b417b8...` | VC7 does not hoist the post-call add here; verifier call-site relocation identity/address changes and the cursor advances only after return | 5/5 exact siblings unchanged | structurally regressed; reverted |
| E49 | use HCEA's 32-bit `int`-equivalent return type for `data_encode_integer` instead of the repository's byte `boolean` | `768/768` | `44/44` | `e955ee30862c...` | the exact callee already materializes the same EAX value, and callers ignore it; both objects are byte-identical to E15 status | 5/5 exact siblings plus `_data_encode_integer` unchanged/exact | no January type proof and no caller benefit; reverted |
| E50 | use HCEA's `short const *` cursor topology throughout the complete encoder, including typed header stepping and byte-cast stride updates | `768/768` | `44/44` | `e955ee30862c...` | VC7 canonicalizes every typed operation back to the same byte addresses; all three residual scratch regions remain unchanged | 5/5 exact siblings unchanged | compiler-equivalent; reverted to the repository's byte cursor |
| E51 | use one block-local `short const *array` view as the common lvalue for array count and element data | `768/768` | `44/44` | `e955ee30862c...` | the typed alias and both derived loads fold to the same SSA values and scratch colors | 5/5 exact siblings unchanged | compiler-equivalent; superseded by the simpler direct expressions |
| E52 | model both counted data and arrays through a readable `{ short count; byte data[1]; }` payload structure | `768/768` | `44/44` | `e955ee30862c...` | member-lvalue provenance is flattened to the same two offsets and does not affect allocation | 5/5 exact siblings unchanged | compiler-equivalent; removed because HCEA supports the simpler cursor representation |
| E53 | share one function-scope `short value_count` across the active data, raw, and array arms | `768/752` | `44/43` | `06d1edf5e47e...` | the cross-arm scalar lifetime collapses the same array topology seen in E16 and loses one relocation | 5/5 exact siblings unchanged | structurally regressed; reverted to domain-specific block locals |
| E54 | initialize a named data-payload pointer before the data-size assert and keep it live through `data_encode_integer` | `768/768` | `44/44` | `8a525e33293d...` | none; the longer live range rotates the three volatile scratch roles to January's EAX/ECX/EDX assignment in the data, raw, and array arms | all five siblings exact | retained; `_code_0010a7c0` exact and object code closes 6/6 |

## Final strict result

All six functions are strict-exact under the hardened comparator: 2,688 of
2,688 padded code bytes, with every relocation count, address, type,
destination, and addend equal. The previously residual encoder now has target
and candidate normalized SHA-256
`8a525e33293d96ad8abf852866cb1679d0ff28e794f81f617e884dc54718ecd7`.

All 14 January-owned select-any `.rdata` COMDATs are also strict-exact: 528 of
528 bytes and no relocations. The candidate's additional 15-byte
`"decoded_packet"` select-any COMDAT is byte-identical to the January copy
selected from `data_packet_groups.obj`; it is duplicate COMDAT ownership, not
new mutable state or a semantic mismatch. The target owns no `.data` or
`.bss` in this object.

The decisive general code-generation lesson is lifetime rather than spelling:
merely naming `decoded_data + sizeof(short)` did nothing in E21, but creating
that pointer before the assert kept it live across validation and the integer
encode call. That changed VC7's interference graph without adding an
instruction, rotating all three later scratch-register choices at once.

## Do-not-repeat and prohibited techniques

- Do not infer success from equal padded size; all three residuals currently
  demonstrate why relocation identity and normalized bytes are required.
- Do not use inline assembly, `volatile`, undefined behavior, byte patches,
  or per-object compiler-flag changes.
- Do not alter the three exact public wrappers without a complete six-function
  regression sweep.

## Residual classification and reopen criteria

Resolved. No function in this object requires a parked, semantic, or fuzzy
source exception. Objdiff still mis-encodes the three private walkers' local
jump-table labels, so `config/semantic_matches.json` contains fail-closed
bookkeeping credits for those functions. Each credit is independently gated
by the hardened comparator; none relaxes byte or relocation identity. The
historical experiments remain above to prevent future edits from accidentally
shortening the payload pointer's lifetime or restoring the incorrect
inactive-pad behavior.

## Original-bug policy adjudication

`code_0010a5f0` intentionally preserves one shipped undefined read. January's
machine code bypasses every assignment to `field_size` when the first field is
outside the packet version range, then consumes the low word already resident
in EDI. HCEA independently records the same uninitialized value. A bounded
25-shape XDK 3911 experiment found no fully defined readable-C form with the
same bytes: normal zero initialization remains 464 bytes with 27 relocations,
but differs in 355 normalized byte positions.

This is admitted only under the project's explicit original-bug preservation
rule. It is not newly introduced undefined behavior, an optimizer exploit, or
a code-generation cast. The production comment labels the defect and states
the safe correction: initialize `field_size` to zero before the loop. The
complete experiment and machine-code proof are in
`research/data_packets_verifier_init/REPORT.md`.

## Cross-build provenance

The October 2001 function at VA `0x11afa0` is instruction-identical through
the data, raw, and array cases, including the delayed packet-definition load.
The HCEA PDB independently preserves the original parameter names and assert
line numbers 253 and 281. These sources were used only to corroborate topology;
the January object remains the sole byte oracle.

## Disposition

Admitted. Code and owned data are strict-exact. The consolidated build passes,
the shared `data_encoding.obj` sweep remains 15/16 with only its pre-existing
`_data_decode_memory` residual, and the admission audit reports zero revoked
objects. Progress recognizes all three verified local-label exceptions and
counts `data_packets.obj` as Matching: Halo now has 262/468 completed objects,
3,132/7,574 accepted-exact functions, and 366,133/1,770,166 exact code bytes on
this branch.

The conservative all-468-unit regression check against clean `14f46f87`
reports 3,095 functions still exact, zero changed-nonexact functions, and
exactly the three packet walkers newly exact. Its only non-green structural
records are expected cross-worktree `.debug$S` path strings, the reviewed
`Matching` config change, and compiler-local `$L...` symbols owned by the three
newly exact sections; no code or non-debug data regression is present.

## 2026-08-13 independent admission revalidation

The completed object was independently transplanted onto campaign commit
`5b2fbf0ff8782dc0899038e330125f1accfa1508` and rebuilt with the configured
XDK 3911 compiler and unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.

- All six functions remain strict-exact: 2,688/2,688 padded code bytes. Every
  relocation address, type, resolved destination, symbolic destination, and
  addend agrees.
- All fourteen target-owned `.rdata` COMDATs remain semantically and
  ownership-exact: 528/528 bytes and zero relocations. The candidate-only
  `decoded_packet` string COMDAT is unchanged from the clean campaign
  baseline and remains a known select-any ownership artifact.
- A clean pre-change manifest covered `data_packets`, `data_encoding`, and
  the `data_packet_groups` consumer. The post-change regression check reports
  zero `changed_nonexact` functions in all three units. Its only non-green
  records are the intentional Matching/config ledger changes, the three newly
  exact packet walkers, and compiler-local `$L...` symbol movement inside
  those newly exact sections; no external symbol or owned-data change was
  introduced.
- The consolidated build and semantic report pass with zero unit errors;
  progress recognizes `data_packets.obj` as completed. The current tooling
  suite passes 173/173 tests.
- The house-rule audit found one return per function, explicit `return;` in
  every void function, one parameter per line, no raw tag/object access, and
  no newly introduced assembly, volatility, alignment directive, magic
  offset, or compiler-control construct. The documented uninitialized
  `field_size` read is the original shipped defect described above, not a new
  optimizer technique; the adjacent comment retains its safe correction.
