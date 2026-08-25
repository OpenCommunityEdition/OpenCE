# `units.obj` Jonas postprocess / IK cluster (2026-08-24)

## Result and fixed boundary

This Jonas-owned wave starts from exact clean canonical commit
`8549f46cd0370eefba28b309e2f5db66bf204f60` on isolated branch
`jonas/units-postprocess-wave-20260824`. The candidate boundary was frozen
before emission at exactly three same-translation-unit private helpers and the
public node-matrix postprocess caller. One natural four-body code-producing
compile produced the following fail-closed result:

| Fixed body | January RVA | Meaningful / padded | Relocations | First-shot disposition | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_code_00198050` | `0x00198050` | `18 / 32` | 0 | absent; removed without retry | `eda63f46a5417149c356fd1b320ca113fb8c9dbf68f135dbdb862b9edf53befc` |
| `_code_001980d0` | `0x001980D0` | `87 / 96` | 4 | strict; retained | `511f513f4eb239481a8b8603d80a90622a39a1acf869eca29f53093c893c581e` |
| `_code_00198130` | `0x00198130` | `53 / 64` | 4 | strict; retained | `db45f34650c1bcf2bdb51275462bd8a457df15df3854b90fca9c222eb60bf553` |
| `_unit_postprocess_node_matrices` | `0x0019FDA0` | `377 / 384` | 13 | strict; retained | `90297d3fbe176962213d34f9ea6f42c63ef39261c79b2ce141897af4a4241807` |

The retained dependency-closed cluster adds three strict code owners, 517
meaningful bytes, 544 padded bytes, and 21 exact relocations. The two private
helpers account for 140 meaningful bytes of hidden exact credit; the public
caller accounts for the 377-byte ordinary-report gain. `_code_00198050` has
no surviving January caller and optimized VC7 correctly did not emit its
unreferenced static definition. The source definition and prototype were
removed wholesale after adjudication. No dummy caller, export, synthetic
anchor, or linkage change was introduced to force it into the object.

`units.obj` remains `NonMatching`. The fixed boundary excludes every other
private helper and every other missing public body in the translation unit.
No header, protected source, configuration, semantic exception, parked
record, completion label, or storage owner is edited.

## Immutable target, canonical source, and compiler

- January executable `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Exact canonical `source/units/units.c` Git blob
  `19a15c4a3a47849ffd8ba0c13989c0c389452519`: 118,776 raw payload bytes,
  payload SHA-256
  `44930da19d5f2331995a282524f8b01cce5e6f283564ac76c695fb9c1f81f76d`.
- Retained `source/units/units.c` staged Git blob
  `b0c6fd683b271214a2a663cedb3ce58f860fd253`: 123,077 raw payload bytes,
  payload SHA-256
  `00c9968113b551607c64651e117e0f421a52717e67a79e7f8cf01dd7a0f93383`.
- Exact canonical `units.obj`: 72,398 bytes, SHA-256
  `b21a7d4dd3ba5bec2d7e1e0d9ec9db5503ee62bd474c3fa9e296cc3474cfc4f6`.
- Immutable four-body first-shot object
  `build/audit/units_postprocess_wave_firstshot.obj`: 73,886 bytes,
  SHA-256
  `b294c8bc6063671d847f9f3448af434268719a3d5a22aecea5d0928c7cead741`.
- Final retained-source pre-commit object: 73,886 bytes, phase-specific
  SHA-256
  `d4642f98a8271d3e67ce4b5ed801c997f0e102010558188d53e9cb4f41e7bf9e`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural repository `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` flags and
  XDK 3911 include graph are unchanged.

The pre-emission canonical inventory contained 633 COFF symbols and 121
non-code sections in the full regression fingerprint. The hardened function
census accepted 97/189 target code COMDAT owners before this wave. None of the
four fixed identifiers was present in the canonical object.

## Claude history and clean-source evidence

All existing Claude Units object documentation was treated as reconstruction
evidence and left untouched: `units_obj.md`,
`units_obj_animation_helpers_lane.md`, `units_obj_combat_lane.md`,
`units_obj_leaf_lane.md`, and `units_obj_state_lane.md`. The repository
matching methodology, exact-match acceleration playbook, hardened COFF,
semantic, regression, progress, admission, parked-function, and transfer
tooling were also read as governing evidence. No historical commit was
wholesale cherry-picked or transplanted.

The most complete readable Claude source was authenticated at tree state
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`. Its
`source/units/units.c` blob is
`124f29e32497c091c4163b5faafc58dbf8890161`: 305,068 raw payload bytes,
payload SHA-256
`173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4`.
That historical translation unit emitted all three retained bodies strict
against January. Only the readable typed bodies in the fixed boundary were
reapplied to the current canonical translation unit; unrelated historical
source, headers, storage, and documentation were not imported.

Two earlier history points independently constrain this wave:

- `f90c85bf` (`Recover Units animation helper predicates`) records the exact
  private-helper fixture proof. Its ledger blob is
  `2385c28d7dd26dba3d106480a27978128cd25149`: 7,040 raw payload bytes,
  payload SHA-256
  `049e321a62a6c984f8044e78d3ce672ac365d81634170032c073f50082a90adb`.
  It explicitly requires unreferenced static `_code_00198050` to remain
  absent until a real caller topology exists and prohibits dummy callers or
  export-based emission.
- `14ba19f7` records an earlier exact postprocess cluster. Its Units source
  blob is `081a9bc01fecbe83b92c6915e3a0ab0e3ce77287`: 202,518 raw payload
  bytes, payload SHA-256
  `a97bac37e66a7253324d651ca71507b26bb90458724af99c177321708431cf96`.
  Its shared-header addition was not needed or imported because current
  `object_types.c` already owns the public declaration used by the object-type
  dispatch table.

The clean HCEA semantic reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. It corroborates behavior and
typed layout; January PC COFF alone governs acceptance.

| Clean c168 HCEA evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `src/unit_postprocess_node_matrices.c` | `d8518c2c9bfffa765feaa65c71f56c8f3acd743c` | 5,655 | `e65e043824722bb5d6a317c2eea14938a3597f368ee3ca202bcf85ae881ba0c7` |
| `src/headers/unit_animation.h` | `0374e07bf8219ded18eff310b78a00f00c19cac6` | 1,820 | `e1b8312530087b3a4a8fc4c6e9196b34486a7924dbed64389d7526d3b6bd2668` |
| `src/headers/animation_graph_unit_seat.h` | `d93535a494774352b27a5c9bc01b7c8286b217de` | 917 | `d2396cb0205325aa2117ac55202b164683811328e1f206ef853f4f5fe6b0b5f8` |
| `src/headers/animation_graph_weapon_class.h` | `ec45d32aaa6f954b7267c6b0380b0a780ef4b1a5` | 1,066 | `6ad8efa399a2d41fd4092b51d952ecc798a81b057df85c51b0d7306783a1d34d` |
| `src/headers/unit_definition_flags.h` | `080527301fb19b80dd61aa548c303a9ccd9daf84` | 3,591 | `dd283f1e4383137f7da20be67f65689c69137e079c5cb6a1d22407f8a6ad237d` |
| `src/headers/unit_animation_flags.h` | `404935a9f0658842c0ca75630d4a1ef94de9982d` | 759 | `209eed62016099cfe930d5b7ace8775104169c8571de0c3f41e0a1010e56d468` |
| `src/headers/tag_block.h` | `917eae0c40137673e1889d041e6a1129926bfba7` | 371 | `191ecccf8b1a75fc0905a612541438020e186c72c17d35d68ec2a5da47e2f807` |
| `src/headers/unit_datum.h` | `1a8bcca480b4e2c25ecb3f87716be80911d1366f` | 718 | `06ec763618bbe9f45c87436607576846a294e0b45d60b00233744ae1d3219bd3` |
| `src/headers/animation_graph.h` | `045b27900e785a6fc70f43c8b970ea2345d1a30a` | 1,074 | `8eb0949258ea6cd693bc94741b220aa344f479dd5d25e9316843fdd5b11cde7e` |

HCEA corroborates `unit_animation.state` at `+0x0B`, `action` at
`+0x0C`, `soft_ping_animation.index` at `+0x1A`, unit-seat IK points at
`+0x4C`, weapon-class IK points at `+0xA4`, and a 64-byte IK point made of two
32-character marker names. It also corroborates unit-definition simple
creature bit 11 and animation postpone-weapon-IK bit 0.

The current canonical typed owners were authenticated before emission:

| Current typed dependency | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `source/units/units.h` | `54492a4ee4cb8b37739f9ebbfa89c72fbd0e08fc` | 20,780 | `e71cfb640579ea5374f06a174b84a3d80e1c816dddb39992353af4a0f04afc83` |
| `source/units/unit_definitions.h` | `40f984e5f2cf9767e92300ec2447cc3dd1f4aa33` | 4,564 | `6384f4f1451a84b7757c03ad2131440acb4c171433daa6ce3ee513d9fed82bf1` |
| `source/models/model_animation_definitions.h` | `bddc1e3c76962e0fa0981902788620d1f2a04bac` | 8,617 | `6ef4b9a92f204175fb4aec4cbed70b87bda50d3fbdfebc2b5f5949db16a5b4d7` |
| `source/objects/objects.h` | `738c831f77a73e2dc3a1e7e9443144857c480ce8` | 16,905 | `94a6abe8acd6c1bc0d807d15a11457d5cea7c6a2cb84f6a9cd9fa71c564a32b0` |
| `source/objects/object_types.c` | `3473be06ddc2cba9b00d8552cc756183462cf384` | 40,512 | `36966d6130637a7b7276977c5c2d2f4b487f7263e2d67b12f39ef657af02a10b` |
| `source/tag_files/tag_groups.h` | `8c9d6343d520da08990cb67a571edc8c46326ec3` | 2,834 | `88a4aa9f40fb5c671f7d64c03acab81e0bcb0521cd9a7d71206da14e4245233b` |

No relevant Claude-only type or declaration delta remained after comparison
with these canonical owners. In particular, no shared-header edit is needed.

## Typed behavior, callers, and private convention

The public contract is the source-local declaration already consumed by
`object_types.c`:

```c
void unit_postprocess_node_matrices(
	long unit_index,
	real_matrix4x3 *node_matrices);
```

It uses ordinary 32-bit C cdecl. The function obtains the typed unit datum and
definition, skips simple creatures or units without an animation seat, then
walks the seat and weapon IK-point tag blocks. Parent-object IK is gated by
`code_00198130`; current-weapon IK is gated by `code_001980d0`. Each accepted
IK point calls the typed `object_inverse_kinematics` interface with named
marker fields and the supplied matrix array. Successful weapon IK clears the
named postpone flag. There are no raw offsets or reconstructed addresses.

`code_001980d0` begins with the typed soft-ping-index test, separately clears
the result when `animation->action` is nonzero, and rejects the authenticated
flying, death, seat, user-animation, AI-impulse, melee, grenade,
resurrection, and leap states. `code_00198130` rejects the authenticated
hard-ping, death, seat, AI-impulse, and resurrection states. Both are private
`static boolean` source helpers over `struct unit_animation *`.

The complete January caller census over all split objects proves:

- `_code_00198050` has zero references anywhere in the tree;
- `_code_001980d0` has exactly one REL32 caller, the public postprocess body;
- `_code_00198130` has exactly one REL32 caller, the public postprocess body;
- `_unit_postprocess_node_matrices` has no direct text caller and exactly one
  DIR32 address reference, the `object_types.c` `_object_data_definition`
  table at `.data+0xE8`.

The January csplit storage class is synthetic for split functions and cannot
establish source linkage. The real caller cluster does: keeping the helpers
`static` lets VC7 select the private ECX convention naturally while the
public table callback remains external cdecl. No explicit register spelling,
declaration coercion, or public helper export is used.

The 13 January relocations for `_unit_postprocess_node_matrices`, in target
order, are:

1. `REL32 +0x0F` to `_object_get_and_verify_type`.
2. `REL32 +0x1E` to `_tag_get`.
3. `REL32 +0x4C` to `_tag_get`.
4. `REL32 +0x5F` to `_tag_block_get_element_with_size`.
5. `REL32 +0x77` to `_tag_block_get_element_with_size`.
6. `REL32 +0x93` to `_code_00198130`.
7. `REL32 +0xB5` to `_tag_block_get_element_with_size`.
8. `REL32 +0xCB` to `_object_inverse_kinematics`.
9. `REL32 +0xFC` to `_code_001980d0`.
10. `REL32 +0x125` to `_tag_block_get_element_with_size`.
11. `REL32 +0x13A` to `_object_get_and_verify_type`.
12. `REL32 +0x14A` to `_unit_inventory_get_weapon`.
13. `REL32 +0x155` to `_object_inverse_kinematics`.

Each helper has four internal DIR32 switch-table relocations. Hardened
comparison accepted all 21 relocation addresses, types, destinations, and
addends as well as every padded byte.

The retained implementation is readable typed defined C. It contains no
assembly, intrinsic, force-inline or calling-convention attribute, optimizer
pragma, `volatile` scheduling device, raw-address or byte-offset access,
pointer or union pun, inactive-union read, undefined overflow, synthetic
anchor, serialized representation trick, or object-byte forcing.

## One-shot discipline and strict retain/remove

The four source bodies, declarations, types, caller topology, and statement
forms were frozen before emission. One `/Zs` parse-only prerequisite passed
and preserved the pristine canonical object hash. The generated object path
was then compiled exactly once by its normal Ninja VC7 edge for the fixed
four-body candidate.

The first-shot object was copied immutably before adjudication. Both called
private helpers and the public caller were immediately strict.
`_code_00198050` was absent because its static definition had no surviving
source caller. Under the fixed per-body rule, absence is rejection: its
prototype and body were removed wholesale without modifying any retained
body, type, expression, declaration, or call topology. A later normal full
graph build mechanically rebuilt the retained source; it was not a new
candidate formulation or tuning attempt. Direct comparison reconfirmed the
same three strict bodies.

## Preserved inventory and ownership

Direct final comparison proves every one of the 97 inherited exact January
code COMDAT owners remains strict. Adding the retained trio advances the
hardened target code census from 97/189 to 100/189 exact; 10 target code
owners remain emitted but nonexact and 79 remain absent. The ordinary object
report advances only its externally visible accounting, from 96 to 97 of 192
functions and from 9,771 to 10,148 of 53,478 meaningful code bytes. Its
36.800526% fuzzy score is not used for admission.

The canonical and retained objects each have exactly 105 runtime non-code
sections under the ownership comparator, with identical keys and zero
changed section fingerprints. Target `_unit_globals` remains an exact 4-byte
`.bss` owner with normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
The complete 1,564-byte target `.data` section remains strict in
the rebuilt object with seven relocations and normalized SHA-256
`6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7`.
No data, BSS, COMMON, aggregate, literal, or storage-owner credit is added by
this wave.

## Pre-commit validation

Only `source/units/units.c` and this newly created Jonas ledger are tracked
changes. No shared header, protected source, configuration, storage owner,
semantic exception, parked record, completion label, pre-existing Markdown,
Claude-owned file, deletion, or unrelated source is changed.

- Complete `halobetacache_build` plus `libcmt_build`: pass; all production
  edges completed. Only inherited warnings in scenery,
  `network_game_manager`, TIFF, and protected `ai_debug.c` were emitted.
- Direct hardened final comparison: all three retained functions and all 97
  inherited accepted functions are strict; `_code_00198050` remains absent.
- Semantic audit: 470 units, 4,167 functions evaluated, 3,562 missing-base
  symbols, 4,027 semantic exact, 103 hidden exact / 62,016 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals
  skipped, 4,088 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,073/11,060 exact functions,
  483,497/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 objects, 3,906/7,574 functions,
  470,583/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `units.obj`: 97/192 ordinary exact functions and 10,148/53,478 meaningful
  code bytes; strict hidden accounting additionally captures the two private
  helpers. The object remains nonmatching and claims only the inherited four
  data bytes in the ordinary report.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass. `git diff --check`,
  changed-path, deleted-path, protected-source, source-policy, and storage
  checks are clean.

A whole-TU regression snapshot was captured from clean
`8549f46cd0370eefba28b309e2f5db66bf204f60` with `--no-build`. The ignored
manifest is 4,479,107 bytes and records the exact canonical object, 97 strict
accepted code owners, 121 full-fingerprint non-code sections, and 633 symbols.
The pre-wave check against the retained candidate fails closed on the expected
new private symbols, function-debug sections, symbol-set growth, and
translation-unit debug/section renumbering. It reports exactly
`_code_001980d0`, `_code_00198130`, and
`_unit_postprocess_node_matrices` as `NEWLY_EXACT`; direct hardened comparison
separately proves all 97 inherited accepted code owners and the complete
runtime non-code inventory unchanged. No regression-tool exception or
metadata weakening is made. A clean post-commit snapshot/check below freezes
the accepted new state without those expected pre-wave structural deltas.

An earlier support-only attempt to capture the baseline snapshot without
`--no-build` invoked normal baseline compilation and then stopped on a
network-blocked optional tool download. `units.obj` remained byte-for-byte at
its canonical SHA-256 above, no production source changed, and the final
pre-wave manifest was created with `--no-build`. This support event is not a
candidate compile.

No committed-state replay is claimed in this implementation commit. After
this source and ledger are committed by Jonas Volman and the worktree is
clean, a fresh committed-state snapshot, verified same-root object deletion,
normal same-path rebuild, immediate regression check, direct strict
comparison, ownership census, and dry run will be appended in one additive
ledger-only commit. No push, amend, history rewrite, or worktree removal is
performed.

## Clean committed-state replay

Implementation-and-ledger commit
`11e2ba084b214255e70f617d08155261ca77133c` was clean before replay and is
authored and committed by Jonas Volman. Re-reading that commit proves the
retained source blob
`b0c6fd683b271214a2a663cedb3ce58f860fd253`: 123,077 raw Git payload bytes
with SHA-256
`00c9968113b551607c64651e117e0f421a52717e67a79e7f8cf01dd7a0f93383`.
Its initial ledger blob is
`9d3738c7373c96c9aea6a9ca9c5185811d02566e`: 18,534 raw Git payload bytes
with SHA-256
`1ca3a4cf47213aed02025613ad1208e7cfccc306d5878287224ca7e025bf32ad`.
`git status --porcelain=v1 --untracked-files=all` printed no path.

A fresh one-unit committed-state snapshot was written with `--no-build` at
that exact clean commit. The ignored manifest
`build/regression_units_postprocess_committed_20260824.json` is 4,509,319
bytes with phase-specific SHA-256
`8d721b8a1940d8667fc27d8979e94f38cd376d7e7428b0ee968d1fdd4cdd2454`.

The resolved generated path
`build/base/source/units/units.obj` was proven to begin with this isolated
worktree's resolved root. The snapshotted object was 73,886 bytes with
SHA-256
`d4642f98a8271d3e67ce4b5ed801c997f0e102010558188d53e9cb4f41e7bf9e`.
That exact file was deleted with `Remove-Item -LiteralPath`, and a second
existence check proved it absent. The normal generated Ninja target then
executed exactly one `[1/1] CL build\base\source\units\units.obj` action with
the unchanged natural VC7 rule and flags.

The immediate no-build regression check returned `ok: true`, zero failures,
zero warnings, `changed_nonexact: []`, `newly_exact: []`, and exactly 100
`still_exact` functions. That set contains the three retained functions and
all 97 inherited strict functions.

Direct hardened January comparison again returned `all_equal: true` for
`_code_001980d0`, `_code_00198130`, and
`_unit_postprocess_node_matrices`, including every padded byte and all 21
relocation identities. The complete direct target code-owner census is
100/189 exact, 10 emitted nonexact, and 79 absent. Removing the retained trio
from the exact set leaves the 97 inherited strict owners. Rejected
`_code_00198050` remains absent.

The rebuilt symbol table records `_code_001980d0` and `_code_00198130` as
storage-class-3 private definitions and the public postprocess callback as a
storage-class-2 external definition. `_code_00198050` has no symbol. This
reconfirms the intended private/public linkage convention after a clean
rebuild.

The rebuilt aggregate `.data` owner is again strict at 1,564 bytes, seven
relocations, and normalized SHA-256
`6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7`.
`_unit_globals` is again strict at four BSS bytes, zero relocations, and
normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
The rebuilt object is 73,886 bytes with phase-specific SHA-256
`a43fbc43eddd93702e576e50c90fc36dd1c6a01dbe0fe530bfc5f468df2840cc`;
the whole-object hash change is the expected COFF timestamp effect.

A normal Ninja dry run reports `ninja: no work to do.` The source blob is
unchanged. This section is the sole change in an additive Jonas ledger-only
follow-up. Nothing is pushed, amended, history-rewritten, or removed from the
worktree.
