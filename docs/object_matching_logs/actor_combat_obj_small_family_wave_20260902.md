# actor_combat.obj small-family wave (2026-09-02)

## Scope and baseline

This bounded lane began from canonical commit `0444c57fc97cdd690560a4788b065011b7cf8b3f` in a detached worktree. The untouched `actor_combat.obj` gate contained 13 exact, three parked residual, and 18 unwritten functions. It carried 1,072/12,416 strict-exact padded bytes.

The historical audit covered every local commit touching `source/ai/actor_combat.c`, including Claude's bulk reconstruction `e02971dfd` and the already-audited canonical subset `4277f892c`. It also read the existing actor-combat ledgers and the local HCEA per-function lifts under `_research_cache/halocea/src/blam/ai/`. HCEA was used only to corroborate semantics and names; January x86 code, relocations, assertion literals, and the repository's typed definitions remained the admission oracle.

## Admitted exact functions

| January address | Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| ---: | --- | ---: | ---: | --- |
| `0x0000F740` | `_actor_get_weapon_vector` | 184 / 192 | 11 | `85723ddbe9f2de7ecd72753f5dae121db9025d84ea12007f1833c84499b532fa` |
| `0x0000FA20` | `_valid_real` | 25 / 32 | 0 | `83977ed5f0ae820e1554588c354ae57295d798d933e591ee898dc39d29731d4f` |
| `0x00010580` | `_valid_realcmp` | 55 / 64 | 1 | `af811d6df5f204708d247a7d50e091f7828989672ded12773edd93431f74852c` |
| `0x000105C0` | `_valid_real_normal3d` | 88 / 96 | 2 | `0958b20f958923b9a2b2561eebda24ae8838b04180389aaaf3926319b19599c0` |
| `0x00011150` | `_actor_combat_retarget_grenade` | 89 / 96 | 4 | `55b123bd2b02dbd795d23d85bd01172f9fce94aae32a15d7f8b519f0d363cb67` |

The gain is five strict functions, 441 meaningful bytes, and 480 padded bytes. The object advances to 18/34 strict functions and 1,552/12,416 strict padded bytes. The whole-tree strict padded total advances by the same 480 bytes.

`actor_get_weapon_vector` is independently named by HCEA and January's call topology. It uses `actor_get`, `vehicle_get`, the typed vehicle-definition tag accessor, the `_vehicle_ai_weapon_cannot_rotate_bit` enum, `TEST_FLAG`, and the unit aiming APIs. The translation-unit-private vehicle-definition view exposes only the authenticated flag at offset `0x2F0`; it does not expand the shared public header or invent a full tag layout.

`actor_combat_retarget_grenade` uses the typed actor-variant accessor and named grenade-combat radii. The three exact real validators are natural emissions of the existing assertion macros used by the recovered grenade/projectile routines rather than copied machine-code bodies.

## Honest fuzzy parks

Three complete ordinary-C bodies were retained and recorded in `config/parked.json`:

- `_actor_combat_reaim_grenade`: 448/448 padded bytes, 16/16 relocations, 162/162 instructions, and 98.509674% objdiff. After restoring HCEA/January's velocity-before-gravity ordering, the residual is a four-byte frame/stack-slot coloring shift plus one independent `tag_get` result/cleanup schedule.
- `_actor_aim_projectile`: 720/720 padded bytes, 27/27 relocations, 237/237 instructions, and 98.017395% objdiff. January assertion literals prove the public parameter name `vector`. The remaining meaningful divergence is the ordering of independent loads and stores in one debug-record aggregate copy at `+0x274`; the streams reconverge afterward.
- `_actor_aim_grenade`: January is 544 padded bytes and the coherent candidate is 576; both have 22 relocations. The candidate is 79.125% objdiff. Its point/vector reuse expresses the vehicle path proved by January without retaining Claude's older coincidental uninitialized-local rendering. The broader frame/x87 schedule is left unclassified.

These functions contain no register forcing, volatile lifetime steering, barriers, fake dependencies, forced inline/noinline controls, gameplay assembly, or nonsensical exactness carriers. The January inline schedule still calls the out-of-line `normalize2d`/`normalize3d` owners and does not emit a `point_from_line3d` COMDAT.

## Rejected historical candidates

- Claude's exact `actor_combat_find_nearby_target` form called `point_from_line3d` and violated the authenticated January COMDAT ownership schedule. The canonical semantic residual remains parked.
- The prior exact collateral-damage candidate was not readmitted: its header/layout route had already been rejected after a `units.obj` regression. The canonical typed residual remains parked.
- The older `actor_aim_grenade` candidate's vehicle path could consume an uninitialized working vector. A coincidental match is not evidence, so that form was replaced with the representation-safe dataflow proved by the target.
- `random_boolean` was not reopened; the existing whole-translation-unit ownership investigation already rejected its isolated exact form.

## Header and ownership boundary

`source/ai/actors.h` is the owning declaration site for the public `actor_aim_projectile` prototype. A pre-edit includer census found 41 translation units, so the full build and the 189/189 `units.obj` gate are required rather than treating this as a local header edit. The recovered private function is named `_actor_get_weapon_vector` in `config/symbols.json`; no `code_<address>` or `bss_<address>` placeholder is introduced.

`normalize3d` is now declared beside `normalize2d` in `source/ai/actions.h`, the
closest header for the `action_charge.c` compiland that owns January's emitted
out-of-line copies.  `actor_combat.c` and `actor_moving.c` deliberately repeat
only the declarations after their inline-import blocks: while those blocks are
active, the owner-header declarations are macro-renamed along with the inline
definitions.  Moving the `#undef`s ahead of `actions.h` was tested and rejected
because it emitted surplus `_normalize2d` and `_normalize3d` COMDATs that are
absent from January.  The narrow redeclarations therefore preserve the
authenticated inline schedule rather than serving as misplaced ownership.

## Validation

- Focused hardened gate: `actor_combat.obj` is 18 exact, six residual, and ten
  unwritten. Each of the five admitted functions independently gates exact;
  each retained fuzzy function independently gates residual.
- Detached-base, target-address-keyed whole-tree strict sweep: 8,235 target
  addresses compared, all 5,047 baseline-exact owners preserved, exactly the
  five functions above newly exact, and zero regressions. The independent
  actor-combat exact-set comparison likewise reports `lost_exact: []` and the
  same five gains, including the anonymous-to-semantic rename at `0x0000F740`.
- Full `ninja all_source progress semantic_progress` after reconciliation:
  pass. The all-code total is 663,601/2,198,102 meaningful bytes and
  5,040/11,060 functions; the Halo campaign subtotal is
  650,531/1,770,166 bytes and 4,868/7,574 functions. This remains an exact
  +441-byte/+5-function delta over the then-current canonical baseline.
- Semantic audit after reconciliation: 473 units, 5,217 functions evaluated,
  5,072 semantic-exact, 5,083 accepted exact, and zero unit errors.
- Protected `units.obj`: 189 exact, zero residual, zero unwritten;
  `_unit_preprocess_node_orientations` remains independently exact at 1,920
  padded bytes.
- Parked-function validation: 94 active, zero stale, zero invalid. Object
  admission audit: zero contradictions and zero revocations.
- Changed-file fake-match scan: zero review leads. `git diff --check`: pass.
- Tool suite: 255/255 tests pass with a workspace-local `--basetemp`.
- COFF ownership: `_actor_get_weapon_vector` is static in both target and
  candidate, `normalize2d` and `normalize3d` remain undefined external owners,
  and the candidate emits no `point_from_line3d` symbol or COMDAT.

No canonical worktree was edited and no push was performed by this lane.
