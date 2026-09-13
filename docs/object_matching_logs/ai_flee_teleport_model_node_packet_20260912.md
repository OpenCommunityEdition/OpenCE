# AI flee, AI teleport, and model-node packet (2026-09-12)

## Result

This focused reconciliation admits four strict-exact owners for **920
meaningful / 944 padded code bytes**. Meaningful sizes come from the canonical
objdiff report and exclude trailing alignment bytes.

| unit | exact owner | meaningful | padded | relocations |
|---|---|---:|---:|---:|
| `source/ai/action_flee` | `_action_flee_control` | 395 | 400 | 8 |
| `source/ai/ai_script` | `_ai_scripting_teleport_starting_location_if_unsupported` | 94 | 96 | 7 |
| `source/ai/ai_script` | `_ai_scripting_teleport_starting_location` | 94 | 96 | 7 |
| `source/models/model_animations` | `_animation_graph_node_matrices_from_orientations` | 337 | 352 | 13 |

The focused gates preserve every inherited exact owner: action flee is 10/13
exact, AI Script is 100/116 exact after the target's stale address name is
aliased, model animations is 25/32 exact, and HS remains 446/448 exact after
the teleport declarations move to their owner header. Every focused compile
also rejects an emitted `_point_from_line3d` owner.

## Provenance and house-rule audit

The January split objects provide the controlling ABI, section, relocation,
and instruction evidence. HCEA's reconstructed symbol database independently
supplies the semantic names and complete field layouts in:

- `src/blam/ai/action_flee_control.c`;
- `src/blam/ai/ai_scripting_teleport_starting_location_private.c` and the two
  public wrapper files;
- `src/blam/models/animation_graph_node_matrices_from_orientations.c`; and
- `src/headers/animation_graph_node.h` (DB-verified 0x40-byte layout).

The model-node layout is deliberately TU-private. Earlier audits rejected a
duplicate *guessed* layout; the later HCEA DB evidence now authenticates every
field, offset, and total size used here. Moving the layout into a broadly
included header is still unsafe under VC7 definition-position sensitivity.
This evidence update supersedes only the old guessed-layout objection; the
previous owner-scheduling warning remains in force.

Action flee uses the January TU's 3/6/7 priority values with semantic local
names. HCEA authenticates the meanings, but its later-build shared enum has a
different numeric layout and is not transplanted. Object and prop access use
their typed subsystem macros, movement receives `NULL` for its path pointer,
and the function ends with an explicit `return;`.

The teleport declarations live in `source/ai/ai_script.h`, and HS includes
that owner rather than redeclaring the public functions locally. The private
helper has the authenticated name
`_ai_scripting_teleport_starting_location_private`; the target symbol map no
longer publishes `_code_000452d0` as its identity.

## Honest fuzzy frontier (zero credit)

The private teleport helper is retained as the best complete, typed HCEA-
backed reconstruction, but it remains residual and receives **zero** exact
credit. Target and candidate are both 288 padded bytes with the same 13
relocation destinations. Their normalized hashes are respectively
`755b7a6e2990f977b15fb61eb09a0106990c68fcd7d69128922e69cc12e52c5e`
and
`9c0415c2243227d13b1b96ca944e476f75acf40417113dab2fc3edcaa501b22e`;
objdiff reports 99.9375%. The remaining one-byte schedule displacement begins
after the January `biped_approximate_surface_index` call. Widening that shared
function's return type can close this helper but perturbs accepted consumers,
so the ABI change is rejected.

`animation_update_internal` was also tested from HCEA evidence. Its natural
body requires a 32-bit owner contract that conflicts with the accepted shared
header and exact callers; the experiment was fully removed and the function
remains unwritten. The previously rejected AI Script allegiance-broken,
follow-target-AI, and command-list-status paths likewise remain unwritten.

## Focused verification

```text
python tools/campaign/gate.py source/ai/action_flee --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/ai/ai_script --all --alias ai_scripting_teleport_starting_location_private=code_000452d0 --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/hs/hs --all --forbid-emitted-symbol _point_from_line3d
python tools/campaign/gate.py source/models/model_animations --all --forbid-emitted-symbol _point_from_line3d
```

`git diff --check` passes for every source and header touched by this packet.
