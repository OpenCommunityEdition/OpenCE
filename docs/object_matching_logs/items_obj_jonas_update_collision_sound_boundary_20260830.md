# `items.obj`: `item_update` collision-sound lifetime boundary

Date: 2026-08-30

## Result

The ordered high-byte fallback reached `source/items/items.obj::_item_update`
only after the two remaining `actor_type_infection.obj` owners were confirmed
to be existing fixed points.  One new, evidence-backed C89 lifetime candidate
was compiled exactly once.  It did not match and was inverse-restored.  No
production source or header change is retained.

- January target: 2,352 padded bytes, 110 ordered relocations, normalized
  SHA-256
  `78237a8528e5d6a858b7aced1df0d8037225ec19da1b3b625c74c900d8ee8b02`.
- Sole collision-sound-local candidate: 2,368 padded bytes, 110 relocations,
  normalized SHA-256
  `6211a0c82e68cf0bb0e607d0904a5298cbe6a0026c0317058c4960244f6358ff`.
- Restored production candidate: 2,368 padded bytes, 110 relocations,
  normalized SHA-256
  `1386e3029840eae1ca28f52d5a7b5c1e652d3c74e4214b89b2a76283bd2e8f26`.
- Strict exact gain: zero functions and zero padded bytes.  The object remains
  17/18 exact; `_item_update` remains its sole residual.

## Ordered fallback audit

The infection object was not reopened.  Its complete ledgers already freeze
both residuals:

- `_code_00027470`: 3,616 target bytes / 104 relocations / normalized SHA-256
  `64eacc5c1e2a60caa70303a25621254f64fb070f72e5efa693892a0de1f19fee`;
  the retained typed support body is 3,360 / 103 /
  `8efce2e0c5b5bde12c28fdbe02450a62381c813ff68d65e7ed976128b0df7bdb`,
  with a fixed stack/register/cooldown-layout boundary.
- `_infection_swarm_aim_jump`: 688 target bytes / 22 relocations /
  `5c2698e07de29daaa887aa39b751163d55cc325ec73ba07e444a91f572d89d09`;
  the retained typed body is 704 / 22 /
  `ceb8bb43749a44bc50ee47c84b8698ed24fcdc6969ac69d7aa14b840e71697e6`,
  with the remaining three-byte vector-copy boundary requiring rejected
  cast/pun shapes.

The Items audit then covered all six prior object ledgers, complete git
history, and 1,554 local worktrees.  The worktrees collapse to five Git-content
families; none contains uncommitted Items source, a later 17/18 body, or a
hidden exact artifact.  The HCEA, Stian complete-history, and Pastudan donors
were inspected.  HCEA establishes later-build semantics, Stian supplies the
closest January-shaped reconstruction, and Pastudan is incomplete in this
area.  January COFF remains the ABI and code-shape authority.

The existing Items ledgers had already measured and frozen candidate/velocity
slot exchange, a block-scoped left-vector temporary, a defined candidate and
z-up-left lifetime union, and both direct clamp spellings.  None had tested a
named collision-sound tag-index lifetime.

## Sole natural candidate

January code loads `definition->item.collision_sound.index` into EAX, compares
that register with `NONE`, keeps it live while constructing the sound location,
and pushes the same EAX value to `unattached_impulse_sound_new`.  The
authenticated HCEA donor independently names the value `impulse_sound` and
reuses it across the same test and call.  That evidence fixed one natural
candidate: declare a block-local typed `long collision_sound_index`, assign it
immediately before the test, test it, and pass the same local to the sound API.

No other expression, declaration, control-flow, type, header, or sibling was
changed.  The candidate remained readable C89 and introduced no raw address,
offset access, pointer/integer reconstruction, type pun, undefined behavior,
asm, volatile/register steering, pragma, intrinsic, attribute, barrier, byte
forcing, or comparator/tool exception.

The candidate preserved the padded-size bucket and relocation count but was
not a local code-generation solution.  The normalized instruction-bijection
walk moved from the restored body's 464 events
(`376 STRUCT`, `37 O-EXTRA`, `38 T-EXTRA`, `13 REG`) to 465 events
(`377 STRUCT`, `37 O-EXTRA`, `38 T-EXTRA`, `13 REG`).  The pre-existing first
divergence also remained: January uses the candidate point at `[ebp-0x0c]` for
the early cross-product temporary while the retained body uses the velocity
slot at `[ebp-0x18]`.  Subsequent velocity-copy and collision-call scheduling
therefore remain part of a whole-function lifetime/control-topology boundary.

Do not retry this collision-sound cache by itself or combine it with the
previously rejected micro-shapes.  Reopen `_item_update` only with new
authenticated source or a single whole-lifetime/control-topology hypothesis
that simultaneously explains the early `[ebp-0x0c]` temporary, the target's
three-load/three-store velocity copy, candidate integration/call scheduling,
and later stack-slot ownership while retaining the exact 0xC8 frame and all
110 relocation owners.

## Restoration and regression evidence

- Canonical base commit:
  `58387f05c8a6a168fafae289bce9f9b67257825a`.
- Clean pre-edit regression manifest:
  `build/audit/items_update_collision_sound_preedit_20260830.json`, SHA-256
  `e08bfd0efd10f59624b92e28ff2b1122ac8b7bcb9b561ef5d0fec31e65e7fec4`.
- Sole candidate object SHA-256:
  `82da1de8c559ef4811c69c47068318c83950f77af8ebfcd1a2c2d5ed11346156`.
- Restored source Git blob:
  `0c7e6403e0dcfd29fb867ae0c0bb413594ecd2ce`, identical to canonical.
- Restored regression-manifest check: `ok: true`, no failures or warnings,
  `_item_update` returned to unchanged-nonexact status, and all 17 inherited
  exact owners remained `still_exact`.

The one candidate compilation was:

```text
[1/1] CL build\base\source\items\items.obj
```

The source was then inverse-restored and rebuilt only to recover the canonical
object state.  This ledger is the only tracked change from the lane.

## Full gates

- Full Ninja report, semantic audit, and progress build: pass.
  - 470 semantic units scanned; 4,954 functions evaluated;
  - 4,837 semantic-exact and 4,847 accepted-exact functions;
  - 131 hidden exact functions; zero unit errors;
  - campaign progress: 384/833 objects, 4,808/11,060 functions,
    610,370/2,198,102 meaningful code bytes;
  - `build/report.json` SHA-256:
    `fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`;
  - `build/semantic_report.json` SHA-256:
    `3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.
- Object admission: zero candidates, contradictions, or revocations.
- Parked-function validation: 12 active, zero stale, zero invalid.
- Tooling tests: 212 passed.
- Protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes and 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Restored Items whole-TU regression: all 17 inherited owners still exact,
  no failure or warning.
