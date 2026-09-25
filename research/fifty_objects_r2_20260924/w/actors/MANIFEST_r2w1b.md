# MANIFEST (r2w1b cross-check) - source/ai/actors

Written by the second worker that ran under the label `r2w1-actors`. The other worker owns `final.c` and its own
manifest. If `MANIFEST.md` exists, it is the primary manifest. This file records an independent reproduction and the audit.

## Base and tip

- Base: canonical `f6d00a8c`. The worktree HEAD is `b7c1b7d9`, the frozen round-2 baseline commit.
- Candidate recommended for landing: `final.c`, snapshotted as `r2w1b_twin_final_v2.c` (sha256 `508ab4ec...`).
- Independent exact reproduction: `cand_full.c` (strip/coreES plus the spawn hunks). All of its sections are
  `section_infos_equal` to the `final.c` object; only the `$L` label names differ.

## Result

| Function | Padded (Jan/cand) | Meaningful | Relocations | Normalized sha (Jan = cand) | Gate |
| --- | ---: | ---: | ---: | --- | --- |
| `_actor_input_update` | 2384 / 2384 | 2380 | 89 / 89 | `bf2e87448623...` | EXACT |
| `_actors_spawn_from_unit` | 672 / 672 | 668 | 29 / 29 | `38d0b2f1fcd5...` | EXACT |

`gate.py source/ai/actors --all`: `== exact 76  residual 0  unwritten 0`. At base the gate was 74/2.
The strict gain is 3,056 padded bytes, or 3,048 meaningful bytes, across 2 functions. The object becomes COMPLETE.

## Source evidence for each changed construct (/Od build 0x4486c0 and 0x44dfe0; readouts in scratch/w/triage_ai_a/)

- **Threat loop** (load-bearing; this closes the old "unit split at 0x6a6"):
  - /Od has `threat` from `object_get` (`push -1`) at [ebp-0x6c].
  - The projectile alias `[ebp-0x70]` is taken only after the type test.
  - The flag test and the danger-zone test are an if / else-if pair with two stores.
  - Production had a polymorphic cast macro and a single `||` store.
  - With this shape, January's `lea edx,[ebx+0x1ec]`, the [ebp-4]/[ebp-8] slots and the eax unit piece at 0x4d3 all
    reproduce. Four prior lanes had recorded that residual as a register tie.
- **Swarm loop**:
  - /Od makes a real call to `0x44bb30`, whose body is exactly `actor_swarm_component_refresh`. January defines
    `_actor_swarm_component_refresh` (80 B), and VC7 inlines it at this site. This fixes rule 13.
  - /Od makes a real call to `0x44e8e0` = `add_vectors3d(&component->position, &swarm->swarm_center, &swarm->swarm_center)`.
    This fixes rule 6.
  - The /Od frame has no `swarm_center` pointer local.
- **Named locals** from the /Od frame, in declaration order:
  - firing variant definition [-0x44];
  - prevehicle encounter [-0x58];
  - mismatch flag [-0x61], set by if / else-if;
  - parent object [-0x24] with a typed vehicle view [-0x3c];
  - short swarm counter [-0x14].
- **Statement shape**:
  - the single-exit swarm arm (0x44884e/0x448871);
  - the `!=` migrate polarity;
  - `normalize3d` as a separate statement.

  All three are byte-inert.
- **Spawn**:
  - /Od ternary `source_actor_index` [ebp-0x10];
  - a real `point_from_line3d` call, replacing a rule-6 hand expansion;
  - a short counter;
  - `real_random_range` (byte-inert).

## Admission audit (run on the r2w1b snapshot object)

- `object_audit.py`: PASS. Every January-owned section and symbol matches. Compared with base, the only new surplus is
  `+.text _add_vectors3d 48` and `+.text _point_from_line3d 48`.
- `pdb_storage.py`: 137 split symbols, 0 disagreements.
- `surplus_identity` (the slug copy, with a candidate path): 18 candidate-only COMDATs, 0 not identical.
  - `_add_vectors3d` is identical to `build/split/source/ai/actor_combat.obj`.
  - `_point_from_line3d` is identical to `build/split/source/ai/action_charge.obj`.
- `provider_link.py --baseline=build/base/source/ai/actors.obj`: both new symbols PASS in both orders. The full run also PASSes.
- `nodup_census`: `_add_vectors3d` has 17 definers and `_point_from_line3d` has 24; neither has a NODUP copy.
- Data: every `.rdata`/`.data`/`.bss` section is equal under `object_audit` (`coff_compare`). The hints data gap is 0.
- There are no rejections for the unit. Two parks are retired: `_actors_spawn_from_unit` and `_actor_input_update`.
- `fake_match_scan` on `cand_full.c`: 0 leads. `CL /W3` produced no new warnings.

## House-rule review

These rule-13 and rule-6 violations are removed:
- the hand-inlined TU static;
- the hand-expanded `add_vectors3d`;
- the hand-expanded `point_from_line3d`;
- the hand-expanded `real_random_range`.

Three TU-local polymorphic cast macros lose their last users and are deleted. Two downcasts remain, each taken after a
type test, each /Od-attested, and neither repeated. There are no dummy locals, pragmas, volatile or asm. There is no
header or `symbols.json` change.

The helper COMDATs come from a strictly exact caller, are byte-identical to January's selected copies, and pass the
provider link. This is the rule-6 owner exception (house rules l.33-40).

## Disclosure and held items

- The only residual left by the load-bearing threat-loop change was an x87 commutative-leaf tie in the second inlined
  `cross_product3d`. A pure dead-local oracle cannot move it.
- Two different /Od-composed sources hit January's bytes:
  - r2w1b's subset (`cand_full`);
  - the complete /Od inventory (`final.c`, where each phase-moving feature is load-bearing under leave-one-out).
- The complete inventory needs no choice among attested features, so it is the recommended landing. The subset is
  corroboration only.
- The full ownership and build sweep with zero regressions is the integrator's step. No header changed, so only
  `actors.obj` bytes move.

## Production changes (mechanical; `git apply --check` is clean at the worktree HEAD)

- `r2w1b_actors_c_v2.patch`: `source/ai/actors.c`, taken from `final.c` at sha `508ab4ec`.
- `r2w1b_parked_json.patch`: deletes the two `source/ai/actors` entries (lines 667-704) with in-place line edits.
- `r2w1b_config_json.patch`: changes `source/ai/actors.c` from NonMatching to Matching.
