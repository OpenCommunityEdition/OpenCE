# MANIFEST: source/ai/actors (round 2, agent r2w1-actors)

The whole object is a candidate for admission: 76 of 76 functions are strict exact and the object audit passes.

## Base and tip

- Worktree: `C:\halo-worktrees\claude-fifty-objects-r2-20260924`.
- HEAD: `b7c1b7d9`. That is canonical `f6d00a8c` plus one lane commit that only touches scratch files.
- `source/ai/actors.c` is unmodified. `git status` is clean.
- Candidate source: `scratch/w/actors/final.c` (sha1 `d9dc3764`).
- Candidate object: `scratch/w/actors/final.obj` (sha1 `50d847eb`).
- Production changes, all mechanical. Each one passes `git apply --check` at HEAD, and the patch reproduces `final.c` byte for byte.

| Patch | sha1 | What it does |
|---|---|---|
| `actors_final.patch` | `691e8fbc` | `source/ai/actors.c` only. No header, no `symbols.json`, no flags. |
| `parked_retire.patch` | `7e937267` | Retires both actors parks (86 to 84 entries). |
| `config_status.patch` | `0e7d18f0` | Sets `source/ai/actors.c` from `NonMatching` to `Matching`. |

## Gate: target vs candidate

| Function | January (size / relocs / sha) | Candidate | Production today |
|---|---|---|---|
| `_actors_spawn_from_unit` | 672 / 29 / `38d0b2f1fcd5109d` | 672 / 29 / `38d0b2f1fcd5109d` | 688 / 29 / `6920d010` |
| `_actor_input_update` | 2384 / 89 / `bf2e87448623ee92` | 2384 / 89 / `bf2e87448623ee92` | 2384 / 89 / `f6ff0151` |

- `gate.py --all` reports `== exact 76 residual 0 unwritten 0` (`final.gate.keep.txt`).
- `relocdiff` on `_actor_input_update`: 89/89 relocations, 0 differing rows.
- `real_regions`: 0 REAL regions. The 8 remaining regions are relocation spelling only.
- The other 74 functions and every data section are byte-identical between production and the candidate (`base_vs_final_sections.txt`).

## Source evidence, per changed construct

All `/Od` addresses below refer to `halo_cache_symbols.exe`, read with `odbuild.py`.

### `_actors_spawn_from_unit` (`/Od` 0x44dfe0)

- **Ternary source actor.** The value goes through compiler temp `[ebp-0xe8]` into named local `[ebp-0x10]`, then `cmp -1`. `actor_get` still reads `unit+0x1b8`. This is Lane A res9 p1, hunk 1.
- **`point_from_line3d(&pos, &fwd, 0.3f, &pos)`.** This is a real call to 0x42e0d0, whose body is `result = v*t + p` (`od_42e0d0.txt`). It replaces a hand expansion that broke rule 6. This is p1, hunk 2.
- **`real_random_range(lo, hi)` at 3 sites.** The two-argument calls go to 0x42f360, whose body is `real_seed_random_range(get_global_random_seed_address(), lo, hi)`. That is `real_math.h:1464`. The change is byte-inert and removes a rule-13 manual inline. actors already emits `_real_random_range`, so it adds no new COMDAT.
- **`short spawned_actor_count`.** `/Od` evidence: word slot `[ebp-8]` with `movsx`/`add`/`mov word`, and the return is `mov ax,[ebp-8]`. The change is byte-inert and removes one C4244 warning.
- **Declaration order.** `/Od` evidence: `placement_data` at -0xb4, `spawned_unit_index` at -0xbc, `angle` at -0xc0. In the throw block, `acceleration` at -0xd8, then `speed` and `vertical`. The change is byte-inert.

### `_actor_input_update` (`/Od` 0x4486c0, full readout in `od_actor_input_update.txt`)

- **Swarm arm.** Short counter at -0x14. `component` is at -0x18. The loop makes a real call to `actor_swarm_component_refresh` at 0x44bb30 and a real call to `add_vectors3d` at 0x44e8e0 (a+b written to result; 79 `/Od` callers).
  - Production hand-expanded both helpers, which broke rules 13 and 6.
  - January inlines the static helper at this site: there is no call relocation, and the swarm region bytes are identical either way.
- **Swarm tail.** `if (swarm_unit_index != NONE) sample(...)`, then `else` (0x44884e `je` skip, 0x448871 `jmp` end). This gives a single exit and replaces two early returns plus a bare block. The change is byte-inert.
- **(a) Parent object.** `object_get(vehicle_index)` (push -1) at -0x24. A typed vehicle view at -0x3c is a pure copy inside the vehicle block.
- **(b) `firing_variant_definition` at -0x44.** A named call result inside the gunner block. Its assignment follows the `gunner = TRUE` store, both in `/Od` and in January's 0x2aa, where the store precedes the call.
- **(c) Polarity.** `/Od` tests `ABS(encounter) != fake_encounter` first and sets `migrate = TRUE`. The change is byte-inert and kept for fidelity to `/Od`.
- **(d) Named `prevehicle_encounter` at -0x58.** The encounter pointer is a named local, and the flag is set through it.
- **(e) No `stored_prevehicle_encounter` alias.** `/Od` reads `actor->meta` bit 1 directly.
- **(f) `firing_position_type_mismatch` flag at -0x61.** It is initialised to 0 and set to 1 in the `if` arm (0x448da3) and again in the `else if` arm (0x448dc6). The `else if` re-tests `flying`.
- **(g) Threat loop.** `threat` is fetched with `object_get` at -0x6c. `projectile` is a typed view at -0x70. There are two stores, `if` super-explode at 0x448f6f and `else if` danger zone at 0x448f9a.
- **(n) Separate `normalize3d(&left)`.** `/Od` pushes the recomputed address, not the pointer returned by `cross_product3d`. The change is byte-inert.
- **Removed macros.** The three TU-local polymorphic accessor macros lose every user and are removed.
- **Bisection.** Probes: `b_*` (one edit alone on top of I1) and `lo_*` (leave-one-out from I4).
  - Load-bearing: the swarm shape and edits a, b, d, e, f, g. Dropping any one of them breaks exactness (2 to 22 REAL regions).
  - Inert: c, n, the single-exit change and the declaration order. They stay in for fidelity to `/Od`.
- **HCEX.** It names only `center_of_mass` (stack aggregate) for this function; scalars are enregistered and have no names. So the new local names are descriptive:
  - `firing_variant_definition` is the canonical name used in `action_charge`.
  - `firing_position_type_mismatch` matches the `actor_debug_info` field `firing_position_type_mismatch_ticks`.
  - `parent_object`, `prevehicle_encounter` and `projectile` are plain descriptive names.
  - `unit_index` is this file's own convention for the swarm loop counter (lines 648 and 761).

## Symbols, storage and surplus

- **`object_audit.py`: PASS.** All 137 January symbols match: name, storage, section and offset. `symbols.json` is unchanged.
- **PDB storage.** Candidate symbols checked against cachebeta publics: 0 disagreements. Production `pdb_storage.py` also reports 0.
- **New surplus vs production.** 48 B each, 0 relocations, selection ANY:
  - `_point_from_line3d`: identical to January's selected copy, `build/split` `action_charge`.
  - `_add_vectors3d`: identical to January's selected copy, `build/split` `actor_combat`.
  - Both are cachebeta publics. There are no NODUP definers on the base board (24 and 17 definers).
- **`surplus_identity` logic on the candidate.** 18 candidate-only code COMDATs, 0 differ.
- **`provider_link.py` (both input orders).**
  - `--baseline`: both new symbols PASS against `action_charge` and `actor_combat`.
  - Full run: SELECTED-PROVIDER LINK PASS, 46 rows.
- **Admission class.** January's `actors.obj` neither defines nor references either helper. This is the all-inlined-TU class admitted by owner ruling 5 (Lane B handoff s3 item 2; house rules l.33-40). Its conditions:
  - genuine shared-header `__inline`: met;
  - byte-identical COMDAT: met;
  - strictly exact caller: met;
  - provider link: met;
  - no hand expansion: met, since this change removes two;
  - zero-regression full sweep: the integrator must still run it.

## Data

- Every non-`.text` owned section is byte-identical to production. `.drectve` is identical.
- `report.json` actors data is 2288/2288 (100%).
- No `semantic_data_matches` entry is needed.

## House-rule review

| Rule | Status |
|---|---|
| 6 | The `pfl3d` emission falls under the owner exception. |
| 13 | Three manual inlinings are removed (refresh, `add_vectors3d`, `real_random_range`) plus the `pfl3d` expansion. |
| 3 | Single exit. |
| 17 | One-line declarations with initialisers where the statement order allows. `firing_variant_definition` cannot be initialised at its declaration, because the store precedes the call. |
| 19 | `real` throughout. |

- **Casts.**
  - Point-to-vector casts on `add_vectors3d` carry per-site `/Od` attestation, identical layout and byte-inert strip tests (`strip_add.c`). The same function already uses point-to-vector casts for `scale_vector3d`.
  - Object-to-unit and object-to-projectile typed views sit after the type test, as in `/Od`. Their strip tests are byte-inert (`strip_views.c`).
- **Duplicated `TRUE` / store arms in (f) and (g).** Both are attested in `/Od` as two separate stores, with the `else if` re-test of `flying`. They are load-bearing. They are disclosed here as authentic, not as filler.
- **Tool checks.**
  - `fake_match_scan`: 0 leads.
  - `/W3`: 6 pre-existing C4244 warnings, versus 7 in production. No C4133 or C4013.

## Credit

- Code: +3,056 padded bytes (672 + 2384), 3,048 meaningful by report sizes (668 + 2380). 2 functions.
- Whole-object admission of actors: 76/76. The Halo object count goes up by one.
- No data credit (it is already at 100%).

## Notes for the integrator

- Apply the three patches and run the full ninja / stable diff. Only `actors.obj` bytes change, and no header is touched, so there are no other consumers.
- Then run `parked_functions` validation, where 84 entries are expected.
- `_add_vectors3d`'s provider is `actor_combat`, which is RESERVED for the external Codex packet. This packet does not edit it. If that packet changes `actor_combat`'s COMDAT emission, re-run `provider_link` for actors.
- An optional, pre-existing item that is not part of this lane's delta: `_actor_handle_unit_effect` (448 B) is strict exact, but objdiff 3.3.1 scores it 96.8%. The cause is 5 compiler-local jump-table labels, with equal internal relocations on both sides. It is eligible for a `semantic_matches.json` entry.
- There are no held items for this object.

## Concurrency note

Two agents ran this task under the same label, `r2w1-actors`, in this slug. The other agent's cross-check is `MANIFEST_r2w1b.md`. It reached the same January sha independently, from a subset of the `/Od` shapes (`cand_full.c`), and recommends `final.c`.

Its `r2w1b_actors_c_v2.patch` is the SP1 state. It lacks only the byte-inert SD1 declaration reorder, so it is superseded by `actors_final.patch`. Apply the three patches listed at the top of this file.
