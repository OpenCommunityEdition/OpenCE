# `physics.obj` Jonas query-prefix wave

This Jonas-owned ledger records one bounded natural-C experiment for five
query-prefix functions in `source/physics/physics.obj`. Exactly one function,
`_physics_test_point`, was strict on the first compile and is retained. The
four nonexact definitions were removed without a source-shape, declaration,
expression-order, or compiler-control retry. The object advances from 0/17 to
1/17 strict functions and remains `NonMatching`; no data credit is claimed.

## Scope and provenance

- Authoritative integration base:
  `7d2ff479dde012f94cc08857e4cc74650b85b9af`.
- Baseline `physics.c` blob:
  `471539133bfe62aaf05f156ad77eacee0ffb522c`.
- Baseline `physics_definitions.h` blob:
  `2e6ec550625a472a7aca86b310b81adad02916ca`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `physics.obj` SHA-256:
  `3efe2ac9f9710e04d121d4151a9407b8adf859465d4d7321f1b675a2acef1ee8`.
- Local HCEA source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- Relevant HCEA blobs are
  `abe15d8e9790f3544760339092030be136537b55`
  (`physics_test_point.c`) and
  `4212dddadf8b7258d55845040c54ee7cadbe4817`
  (`mass_point_definition.h`).
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and source include paths.

HCEA supplies the signature, loop intent, and mass-point field roles. January
COFF is the sole authority for byte extent, tag-block access topology,
relocations, and ownership. The January body independently proves a 128-byte
tag-block stride, position at `+0x38`, radius at `+0x68`, a signed-short loop
index, and calls to `matrix4x3_inverse_transform_point` and
`tag_block_get_element_with_size`.

## Validated baseline

The untouched base has 0/17 exact functions and 0/15,670 meaningful code
bytes. Its target contains 15,808 padded code bytes and 657 non-code bytes;
none of the non-code bytes is strict or claimed by this lane. All five ranked
function symbols were absent from the base object.

The five January candidates measured as follows before compilation:

| Function | Meaningful / padded | Relocs | Target normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_pin_fraction` | `113 / 128` | 2 | `7db92c519ccb57b12f19b04d4b99fc5caaa4190ed48b4025b301357f1d6c023b` |
| `_physics_get_features_in_sphere` | `171 / 176` | 3 | `2bb49780912ee4b1a8ca51d86d50370013c762559a833a912e17516e6b783699` |
| `_physics_instance_new` | `248 / 256` | 6 | `1858dbc01b28d1ed83a44a0c5f27b802380081567417ac8f0dcbbcafa1a8b418` |
| `_physics_test_point` | `154 / 160` | 2 | `2ff01415c7e3798fcb87bd26fc2392bdd80db2ff2a038dba17100ed1b8dc01e3` |
| `_physics_test_vector` | `284 / 288` | 5 | `4b4c96c4cfa798065b040f669deb12744c5417a06596f24a63bebec87f665169` |

## Bounded natural experiment

All five typed bodies were compiled together exactly once. The formulation
used repository-native object getters, tag getters, tag-block accessors,
named matrix fields, and ordinary math helpers. It did not reproduce HCEA's
raw tag-block-address casts or matrix row casts. One body was strict; the
other four were immediately withdrawn.

| ID | Function/source shape | Size T/B | Relocs T/B | Candidate hash | First preserved divergence | Decision |
| --- | --- | ---: | ---: | --- | --- | --- |
| E01 | HCEA branch topology for `pin_fraction` | `128/128` | `2/2` | `13498d367fb575e9cd36dd55ea8b6d627e0c7d8f4560d91ac38d62e3c3b5a7fe` | target literals relocate at `+0x1F/+0x34`; candidate at `+0x63/+0x6B` | removed |
| E02 | Typed mass-point feature loop with `TAG_BLOCK_GET_ELEMENT` | `176/176` | `3/3` | `b810430bddcc09288b19d2949924af37d3205f410ce0db3cb21d8503ad7c1c34` | all relocation offsets agree; the raw first byte offset was not preserved before required withdrawal | removed |
| E03 | Typed object/tag getters and named-field matrix construction | `256/256` | `6/6` | `15a96e72e937b69bb0ec757392ad9e6b6c5fa7b513297282989f0b8631c7532c` | second `_tag_get` relocation is target `+0x45`, candidate `+0x48` | removed |
| E04 | Typed inverse transform, tag-block loop, and `point_in_sphere` | `160/160` | `2/2` | `2ff01415c7e3798fcb87bd26fc2392bdd80db2ff2a038dba17100ed1b8dc01e3` | none; normalized bytes and relocations are identical | **retained** |
| E05 | Typed sphere-vector loop and component plane equation | `288/288` | `5/5` | `8667107cb46fe1567a0b9cd0d4d5b840186f18f94a1f850ad7c72ed9f9d48f62` | final plane-transform relocation is target `+0x102`, candidate `+0xFE`; earlier raw byte offset was not preserved | removed |

No sibling could regress because the untouched baseline emitted no function
section. The final object contains only E04; all four rejected public symbols
are absent.

## Accepted control and typed boundary

The retained function:

1. inverse-transforms the query point through the instance's named world
   matrix;
2. iterates the definition's mass-point block with a signed `short` index;
3. obtains each element through `TAG_BLOCK_GET_ELEMENT`, preserving the target
   helper call and authenticated `0x80` stride;
4. tests the typed `position` and `radius` fields with the existing
   `point_in_sphere` inline; and
5. explicitly returns `TRUE` on the first containment and `FALSE` otherwise.

`mass_point_definition` is added to `physics_definitions.h` with natural
fields and compile-time proofs for size `0x80`, position offset `0x38`, and
radius offset `0x68`. No raw padding view, byte-offset dereference, or tag
address cast is used.

## Data and ownership boundary

The lane adds no source-defined global, COMMON owner, `.bss`, `.data`, or
runtime `.rdata`. Each pre-existing source global remains byte-identical to
the pre-wave candidate object: the four initialized real globals retain their
shared 16-byte section hash
`61324a19b6c2392d9ef4cf216a1dfa1d66d065c99f936630fcb31beffc509028`,
and `depths_of_hell` retains its four-byte zero section hash
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
Those sections do not match January's combined ownership shape and receive no
credit.

## Do not repeat and residual classification

- Do not retry alternate branch orderings for `_pin_fraction`; this lane was
  capped to the first natural source formulation.
- Do not reorder expressions or declarations in the three equal-sized
  physics bodies merely to steer scheduling or relocation placement.
- Do not replace `TAG_BLOCK_GET_ELEMENT` with `tag_block.address` arithmetic.
- Do not cast points to vectors, matrices to row arrays, or object pointers to
  byte-layout views.
- Do not use assembly, `volatile`, force-inline annotations, optimizer
  pragmas/barriers, undefined aliasing, signed-overflow tricks, raw addresses,
  synthetic anchors, or object-byte forcing.

The four residuals are natural-source topology/scheduling mismatches. Their
sizes and public dependency sets corroborate the semantics, but equal size is
not matching evidence. There is no private-EAX ABI or ownership blocker in
this prefix.

## Reopen criteria

Reopen a removed body only with independent January-era source, PDB local
topology, or another compiler-authentic source artifact that fixes its exact
control/data-flow shape. Disassembly-guided spelling permutations alone do
not satisfy that threshold.

## Disposition and validation

`_physics_test_point` is strict exact and eligible for ordinary function
credit. `physics.obj` remains `NonMatching` at 1/17 functions,
154/15,670 meaningful code bytes, and 0/657 non-code bytes. The final retained
section has 160 padded bytes, two `REL32` relocations, and normalized SHA-256
`2ff01415c7e3798fcb87bd26fc2392bdd80db2ff2a038dba17100ed1b8dc01e3`.

- Final `physics.c` blob/SHA-256:
  `5a61b6e14734043154e2656bc09bd1e95a84fb87` /
  `84dfb679f1d33aff2db79f326af19e25421deb0cc107578ce75b3589425dcc3d`.
- Final `physics_definitions.h` blob/SHA-256:
  `c6778ea68a2e143933c0617a26290db7f18ea4c6` /
  `e5f6011def5de39ab2c1ed206406013d13bf4e0121f7dfae46c0fc288174286b`.
- Complete `halobetacache_build` and `libcmt_build`: pass, 568 actions.
- Strict semantic audit: 470 units, 3,995 functions evaluated, 3,855
  semantic exact, 101 hidden exact / 61,876 hidden code bytes, 3,916 accepted
  exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,903/11,060 functions,
  472,397/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,736/7,574 functions and
  459,483/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The fail-closed pre-wave manifest reports `_physics_test_point` as the sole
  `NEWLY_EXACT` function and `changed_nonexact: []`. The intended function
  COMDAT necessarily changes compiler debug and symbol ownership records, so
  the generic old/new structural comparison reports `.debug$S` and symbol-set
  review findings; no adjudication or configuration exception is added.

A clean post-commit snapshot/check followed by deletion and forced rebuilding
of `physics.obj` is the final same-path reproducibility proof and is recorded
in the handoff. No configuration, exception ledger, parked record, frozen
object, or pre-existing Markdown file is changed, and no push is performed.
