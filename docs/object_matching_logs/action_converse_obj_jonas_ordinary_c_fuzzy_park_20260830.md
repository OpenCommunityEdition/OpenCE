# `action_converse.obj`: ordinary-C restoration and fuzzy park

## Outcome

`source/ai/action_converse.c` now contains a credible ordinary-C
reconstruction of `action_converse_perform`.  The former implementation used
three inline-assembly regions to transcribe January's shared-`TRUE` register
choice.  That was an exact machine-code reproduction, not credible original
gameplay source, and is not allowed by the rule limiting sparse inline
assembly to helper and math functions.

The ordinary-C function is intentionally retained at the measured compiler
boundary:

- object-local strict result: `8 / 9` functions, `640 / 896` padded bytes;
- target packet: `256` padded bytes, `8` relocations,
  normalized SHA-256
  `7a3fad8fb69307c132558b906832d4d31f0334734fc76d18fe773189b5a38d5d`;
- base packet: `256` padded bytes, `8` relocations,
  normalized SHA-256
  `288862a662942eb5b03ee84809dbd6b6aea1c023d52915f2bc373192e7c212cb`;
- objdiff fuzzy score: `93.14815%`;
- park class: `register-allocation`.

No exact-match credit is claimed for this function.

## Source credibility and semantics

The retained topology is the clean donor preserved at
`C:\halo-worktrees\opus-dual-scheduler-research\dispo-wt`, commit
`5ac58ab8731e9227de6fd87afdf0542ce318aa59`.  The same body originated in
commit `b4651a63db29932d5a6640d0c7b5af59a13c8463`.

The function performs a coherent sequence:

1. Resolve the conversation's unit to a prop when no prop is cached.
2. Mark the actor in range when the prop is fully perceived and inside the
   requested distance, or is closer than `0.7` world units.
3. Halt movement once in range.
4. Otherwise request movement toward the prop and record failure when that
   request fails.
5. Record failure when no prop can be resolved.

This is not an accidental or nonsensical byte match.  The state transitions,
callee contracts, field offsets, and relocation identities agree with the
January function.  HCEA interface evidence at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` supports the action-table return
contract and the reconstructed behavior.  The open Marathon source was
checked as a naming/style reference, but it supplies no stronger homolog for
this Halo-specific conversation action.

The numeric visibility threshold was replaced with the authentic enum
constant `_actor_perception_full`.  January accesses `prop->visibility`; an
HCEA reconstruction using `team_index` at that point is inconsistent with the
January offset and was not copied.

The local `true_value` declaration and assignment remain separate even though
the general style preference is to combine them.  This is the strongest
measured natural-C packet, and moving its initialization is one of the already
exhausted lifetime shapes.

## Residual compiler boundary

The target and base have the same eight relocation identities in the same
order:

| Relocation | Target | Base |
| --- | ---: | ---: |
| `_actor_data` | `+4` | `+4` |
| `_datum_get` | `+17` | `+16` |
| `_prop_get_base_by_unit_index` | `+67` | `+63` |
| `_prop_data` | `+107` | `+103` |
| `_datum_get` | `+114` | `+110` |
| `__real@3f333333` (`0.7f`) | `+157` | `+153` |
| `_actor_move_halt` | `+186` | `+183` |
| `_actor_move_to_prop` | `+220` | `+216` |

January retains `TRUE` in `EBX` and uses `BL` for the two byte stores.  The
lawful VC7 C output folds those values into immediate operands.  Five grounded
source shapes were exhausted without finding a credible C lever.  The former
asm forced the target register allocation directly and was therefore removed
rather than treated as a reconstruction.

## Prototype ownership cleanup

The same wave removes use-site declarations and places the contracts with the
closest associated headers:

- `prop_get_base_by_unit_index` is declared under `prototypes/PROPS.C` in
  `source/ai/props.h`;
- `actor_move_to_prop` and `actor_move_halt` are declared under
  `prototypes/ACTOR_MOVING.C` in `source/ai/actors.h`, because
  `actor_moving.c` has no dedicated header;
- `actor_move_halt` uses its authentic `boolean` return contract rather than
  the inaccurate local `void` declarations.  Existing callers ignore the
  value, so their call behavior is unchanged;
- duplicate declarations were removed from `actions.c`, `action_charge.c`,
  `action_converse.c`, `action_fight.c`, `action_guard.c`, `action_search.c`,
  `action_wait.c`, and `ai_communication.c`.

All project real-valued parameters use `real` from `cseries.h`; no raw
`float` interface was introduced.

## Verification

- Focused VC7 compile of `build/base/source/ai/action_converse.obj`: pass.
- Full configured build, semantic report, admission audit, and progress gate:
  pass with `0` unit errors.
- `tools.coff_compare` confirms the packet sizes, hashes, and relocation
  identities above.
- The pre-change regression manifest reports exactly one intentional change:
  the formerly accepted asm transcription is now nonexact.  All eight
  inherited strict-exact Action Converse functions remain exact.
- Park validation: `13` active, `0` stale, `0` invalid.
- Strict campaign board after the cleanup: `278 / 619` fully exact objects,
  `4,857 / 8,246` strict-exact functions, and
  `674,823 / 1,922,669` padded bytes.  The perform function is counted as
  parked, not exact.
- Test suite: `212 passed`.
- `git diff --check`: pass.
- The rebuilt object has no `point_from_line3d` symbol or COMDAT; geometry and
  the January inline schedule are untouched.

Raw object hashes for the frozen verification build:

- target `action_converse.obj`:
  `9a69bb69f5590a576fd82f635f47fdcb037b6ce5410d5ce2e5b4d3bd92510a35`;
- base `action_converse.obj`:
  `e12f04462cff30aa8d59e15ff5d64a786ebc5d19506ed5147103a34848dc731b`.
