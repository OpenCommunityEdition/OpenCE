# `weapons.obj`: interface-state first shot (2026-08-29)

## Result

The first and only production candidate for
`_weapon_build_weapon_interface_state` is not strict exact and receives no
implementation credit. The ordinary typed C89 body reproduced January's
complete call and assertion topology: all eight relocation identities and
their offsets agree exactly. Its post-call lifetime and boolean materialization
were different, however, and the candidate emitted 304 rather than 320 padded
bytes. No declaration, temporary, field-access, condition, or placement variant
was compiled.

The candidate was removed with the exact inverse patch and the baseline object
was rebuilt. `source/items/weapons.c` is restored to Git blob
`f6abe256f6b4becb78c6db398fd577e98a8cc6e3`, byte SHA-256
`33ac3c07f2f09d0b235ce872c6edce9bf1426c416cbf9e14b40c6b1d6f01a513`.
The object remains at 37 exact, two residual, and 40 unwritten functions. No
production source, header, configuration, exception, parked entry, or protected
Units file survives changed by this wave.

## Provenance and prior-work census

All four current Weapons ledgers were read before the source edit:

- `weapons_obj_codex_checkpoint.md`;
- `weapons_obj_jonas_animation_update_first_shot_20260826.md`;
- `weapons_obj_jonas_random_first_shot_20260826.md`; and
- `weapons_obj_jonas_random_tail_rejection_20260826.md`.

Claude's complete `weapons_obj.md` reconstruction record on
`claude/near-complete-objects-20260816` was also read. It documents the exact
private-helper cluster, the accepted public functions, and the external-COMDAT
ordering boundary, but it never attempts this function. Path-limited all-ref
history finds the target name only in the initial address inventory. No
reachable `source/items/weapons.c` blob defines the body, and the registered
worktree census found only the same inventory comment.

January COFF is the ABI, instruction, size, constant, relocation, and acceptance
authority. Two independent public records agree with it:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`,
  `src/blam/items/weapon_build_weapon_interface_state.c`, supplies the named
  32-byte output layout and natural field semantics. Its donor payload SHA-256
  is `b0d58964588ace6ab46cfc5879b4cc39ad8dd9cca396cfbcfa98a2128fab5f95`.
- Pastudan full-history HEAD
  `918af885935ec470a31256ecce9a977b12b01f80`, file revision
  `6f2c94da9201e0ab0c239e29d9b0c541b20d43f2`,
  `src/halo/items/weapons.c`, supplies an instruction-faithful readable lift.
  It independently confirms the short loop index, exact fields, 10-byte output
  magazine stride, inlined magazine bounds check, and tag-block element size
  `0x70`.

The current typed owner is link-closed. `weapon_get` lowers to
`_object_get_and_verify_type`; `weapon_definition_get` lowers to `_tag_get`;
the existing private `weapon_magazine_get` supplies the second `_tag_get` plus
the exact line-`0x672` assertion; and `TAG_BLOCK_GET_ELEMENT` supplies
`_tag_block_get_element_with_size`. All are already present in the production
translation-unit closure.

## Frozen candidate

The one candidate introduced local named structures for the 10-byte magazine
record and 32-byte interface state, then used only existing named fields:

- copy `weapon.heat`, `weapon.age`, runtime overheat bit zero, and the typed
  magazine count;
- loop with a `short magazine_index`;
- obtain the runtime magazine through `weapon_magazine_get` and its definition
  through `TAG_BLOCK_GET_ELEMENT`;
- write `reloading` for states 1 or 3, `can_fire` for state 0, and the four
  loaded/remaining current/maximum short counts; and
- return explicitly.

The body used normal C89 declarations at block starts, one parameter per line,
and an explicit return. It contained no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, forced inline, optimizer barrier, raw address or
byte offset, pointer/integer reconstruction, cast or union pun, inactive-union
access, undefined behavior, synthetic caller or anchor, byte forcing, object
patch, compiler/flag change, or comparator exception. A Ninja dry run exposed
one production edge; exactly that edge emitted the candidate once.

## Strict measurement

| Measurement | January | First shot |
| --- | ---: | ---: |
| Meaningful / padded bytes | 312 / 320 | 297 / 304 |
| Relocations | 8 | 8 |
| Normalized SHA-256 | `797487e4165a50e7bab9e68ff66ceb2d1caf551c73547ba84e10e1dc3f078f1a` | `5a65ba977b500e5951f6e9f6c1bdbad0eaefc1e128f11588eabfe12535fa97b9` |

The first-shot whole object had raw SHA-256
`65452466a726db7fb4efea2152f512f621be13470f66c7068464e46f1e14e7b2`.
The immutable January split object has SHA-256
`5dd414f3f77e67aa7b6609f9131debfa8a9b3b23afab6a57fe4e790410cff2fe`.

All ordered relocation offsets, types, destinations, and zero addends agree:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x0F` | REL32 | `_object_get_and_verify_type` |
| `+0x1E` | REL32 | `_tag_get` |
| `+0x70` | REL32 | `_tag_get` |
| `+0x8E` | DIR32 | Weapons source-path string |
| `+0x93` | DIR32 | Magazine-index assertion string |
| `+0x98` | REL32 | `_display_assert` |
| `+0x9F` | REL32 | `_system_exit` |
| `+0xB8` | REL32 | `_tag_block_get_element_with_size` |

The first normalized byte difference is the stack-frame immediate at `+0x05`:
January reserves `0x0C` bytes while the candidate reserves `0x08`. Both bodies
then preserve the exact call/relocation prefix through the tag-block lookup.
After that call, January stores the reload predicate in a four-byte stack
temporary and later recomputes the output magazine pointer. The candidate keeps
that output pointer live and lowers the direct boolean expression through a
branch-local EDX value. This shortens the tail by 15 meaningful bytes. It is a
source-lifetime/materialization boundary, not a missing type, wrong field,
wrong callee, wrong constant, or relocation problem.

Do not repeat the HCEA-shaped direct output-field assignment, change the
predicate or temporary spelling, or sweep declaration/lifetime variants. Reopen
only with authenticated original-source topology or an independent same-compiler
record proving the target's four-byte boolean temporary and output-pointer
lifetime.

## Whole-TU boundary and restoration

The frozen candidate manifest also reproduced the previously documented
Weapons external-COMDAT boundary. Thirty inherited accepted owners remained
full-fingerprint exact. Seven private accepted owners were reported changed by
numeric section ownership:
`_code_000ec1a0`, `_code_000ec960`, `_weapon_put_away`, `_weapon_ready`,
`_weapon_reset`, `_weapon_set_state`, and `_weapon_stop_reload`. One anonymous
`.debug$F` owner and the symbol set moved as well. No exception or ownership
recipe was added.

After the inverse patch and baseline rebuild, the frozen manifest reports all
37 accepted functions `still_exact`, `changed_nonexact: []`, no data failure,
and no warning. The phase-specific restored object SHA-256 is
`850c29eabda7919dc7b7212f0897852d2eeca1107fe1b4a15f2f00a50022d268`.

## Final gates

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Strict Weapons gate: 37 exact, two residual, 40 unwritten; the target remains
  unwritten at 320 padded bytes.
- Strict board: 277/619 complete objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,810 accepted exact, and zero unit
  errors or ordinary rejections.
- Campaign progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes.
- Admission: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid.
- Protected Units gate: 189/189 functions exact. The sentinel
  `_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
  bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Complete tool suite: 212/212 passed; the sole warning is the sandbox's denied
  optional `.pytest_cache` creation.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- Frozen restored regression check and `git diff --check`: pass.

No candidate source or production object is retained, no push was performed,
and this additive ledger is the lane's only intended tracked result.
