# REVIEW (round 2, adversarial): source/ai/actors, OBJECT_COMPLETE_CANDIDATE

Reviewer slug: `scratch/w/review_r2_actors/`. Worktree HEAD `b7c1b7d9`, which is canonical `f6d00a8c` plus scratch-only files. `git status` was clean throughout; no tracked file was touched. Every probe is recorded in `LEDGER.md` (R0-R5).

## Verdict

**APPROVE the object with an amended patch set: the primary worker's set, frozen and verified here.** Do not land `r2w1b_actors_c_v2.patch` on its own; it is superseded.

- Amended combined patch: `scratch/w/review_r2_actors/amended/actors_admission_combined.patch` (sha1 `096b9dff`).
- It is the concatenation of three frozen copies:
  - `amended/actors_c.patch`, the same as `scratch/w/actors/actors_final.patch` (sha1 `691e8fbc`);
  - `amended/parked_json.patch`, the same as `parked_retire.patch` (sha1 `7e937267`);
  - `amended/config_json.patch`, the same as `config_status.patch` (sha1 `0e7d18f0`).
- `git apply --check` passes at HEAD for each of the three alone and for the combined file.

### Why the proposal was amended (concurrency finding)

`scratch/w/actors/final.c` changed while this review was running:

- At 08:40:25 its sha256 went from `508ab4ec` to `80655bd2` (sha1 `d9dc3764`).
- The primary worker then wrote `MANIFEST.md` (08:43), which is primary under the task rules.
- Its `actors_final.patch` adds SD1, a reordering of declarations in the `_actors_spawn_from_unit` loop and throw block to match the /Od frame:
  - loop body: `placement_data` -0xb4, `spawned_unit_index` -0xbc, then `angle` -0xc0;
  - throw block: `acceleration` -0xd8, then `speed` -0xe0, then `vertical` -0xe4.
- `r2w1b_actors_c_v2.patch` still has the production order: `angle` declared first and `acceleration` declared last.

I confirmed that /Od slot order equals declaration order in this build. In `actor_input_update`, the aggregate `center_of_mass` sits at -0x34, below `parent_object` at -0x24, so /GS and RTC do not hoist aggregates. The r2w1b patch therefore contradicts the attested /Od declaration order. That is a round-1 reviewer-rejection class, even though it is byte-inert. Both patch sets build objects that are section-identical (183 of 183 sections), so this amendment costs nothing.

## Independent re-runs (reviewer)

| Check | r2w1b v2 (`patched.obj`) | primary (`primary.obj`) |
|---|---|---|
| `git apply --check` at HEAD (alone and together) | OK | OK |
| Patched HEAD equals worker `final.c` | yes (at 508ab4ec) | yes, byte for byte (d9dc3764) |
| `gate.py --all` | 76/76 EXACT | 76/76 EXACT |
| `_actor_input_update` | EXACT 2384 | EXACT 2384 |
| `_actors_spawn_from_unit` | EXACT 672 | EXACT 672 |
| `object_audit` | PASS, 137 symbols, 0 differ | PASS |
| New surplus vs base | `_add_vectors3d` 48 B, `_point_from_line3d` 48 B | same |
| `surplus_identity` logic on the candidate | 18 COMDATs, 0 not identical | 18 COMDATs, 0 not identical |
| `pdb_storage` logic on the candidate | 137 / 0 disagreements | 137 / 0 |
| `provider_link --baseline` | both PASS, both orders | both PASS |
| Full `provider_link` | 46 PASS, 0 FAIL | PASS |
| Section diff vs `build/base` | only the 2 functions, plus 2 COMDATs | identical to r2w1b |
| Section diff vs `build/split` | 137 same, 0 diff (all data equal) | same |

Surplus identity:
- `_add_vectors3d` is identical to `actor_combat`'s copy.
- `_point_from_line3d` is identical to `action_charge`'s copy.
- `_real_random_range` is identical to `action_alert`'s copy. It is already emitted at base.

`nodup_census`: `_add_vectors3d` has 17 definers, `_point_from_line3d` 24 and `_real_random_range` 12. None of them has a NODUP copy.

Other checks:
- **HEAD baseline.** HEAD `actors.c` gates 74/76. Its object is section-identical (181 of 181) to `build/base`.
- **objdiff 3.3.1 mini report** (frozen sha1 3130e428; target is the split, base is the candidate):
  - Code goes from 15,638 to 18,686 of 19,130, which is +3,048 meaningful bytes.
  - Functions go from 73/76 to 75/76. Data is 2288/2288.
  - The single remaining row below 100% is `_actor_handle_unit_effect` at 96.8%. It is strict EXACT and already sits in `semantic_report` `hidden_exact`/`accepted_ledger`, so the strict view is 76/76. The `Matching` label will therefore not be revoked by `audit_object_admission`.
- **Config patches.**
  - `parked.json` goes from 86 to 84 entries, removing only the two actors parks. The JSON is valid.
  - `config.json` changes only `actors.c` (index 438), from NonMatching to Matching. The JSON is valid.
  - The r2w1b and primary patch sets produce identical `parked.json` and `config.json`.
- **Warnings (`/W3 /Zs`).** Primary has 18 warning lines against 19 at HEAD. The spawn `return long->short` C4244 is gone. No new warnings, and no C4133 or C4013.
- **`fake_match_scan`.** 0 leads, the same as HEAD.

## /Od re-derivation

I regenerated the /Od readouts with `odbuild.py`:
- `od_input_update.txt`, function 0x4486c0, located through the "is flying but was placed in a non-3d" string;
- `od_spawn.txt`, function 0x44dfe0;
- the bodies of 0x42e0d0 (point_from_line3d), 0x44e8e0 (add_vectors3d) and 0x42f360 (real_random_range).

Every changed construct is attested:

- **Swarm arm.**
  - The short counter is at -0x14.
  - `component` is at -0x18 and is fetched before the real call `0x44bb30(unit_indices[i], component_indices[i])`.
  - The real call `0x44e8e0(&comp->position, &swarm_center, &swarm_center)` is add_vectors3d. There is no pointer local.
  - `if (swarm_unit_index != NONE) sample` is followed by the only `jmp` to the epilogue, so the function has a single exit.
- **Parent object.**
  - The `?:` temp at -0x80 receives `object_get(push -1)` and is stored to -0x24.
  - The vehicle view at -0x3c is a pure copy taken after `type==1`.
- **Gunner block.** The `or [0x128],8` store comes before the call to 0x4507a0, whose result goes to the named slot -0x44. The declaration therefore cannot carry the initialiser.
- **Migrate block.**
  - The `!=` arm sets migrate to 1 first.
  - The encounter at -0x4c is fetched before `migrate = 1`.
  - The squads are at -0x50 and -0x54.
- **Prevehicle encounter.**
  - It is a named slot at -0x58 and is updated by a read-modify-write `or`.
  - The non-vehicle arm makes 4 stores, then reads meta bit 1 directly, with no alias.
- **Mismatch flag.**
  - It is at -0x61 and set to 0.
  - It is set to 1 at 0x448da3 (the `if` arm) and again at 0x448dc6 (the `else if` arm, which re-tests flying).
- **Threat loop.**
  - `object_get(push -1)` goes to -0x6c.
  - After the `type==5` test, the projectile copy goes to -0x70.
  - There are two stores, at 0x448f6f and 0x448f9a.
- **normalize3d.** It is a separate call on the recomputed address.
- **Spawn.**
  - The short count is at -8.
  - A ternary temp at -0xe8 feeds `source_actor_index` at -0x10.
  - `actor_get(unit+0x1b8)` uses `actor_index`, the same semantics as the production OR chain, so no new behaviour is introduced.
  - `real_random_range` appears at 3 sites.
  - `point_from_line3d(&pos, &fwd, 0.3f, &pos)` is called.
  - The declaration order matches SD1.
- **Frame inventory.**
  - Every user slot from -0x8 to -0x7c has a local in `final.c`, in declaration order.
  - -0x80 to -0x90 are compiler temps (?:, bool and x87 temporaries).
  - `final.c` is therefore the complete /Od inventory. No attested feature was selected away, and none is missing.
- **HCEX.** For `actor_input_update` it names only `actor_index` and `center_of_mass`, so the new names are descriptive. They follow file conventions or field names:
  - `unit_index` is the swarm loop counter used 4 more times in this file;
  - `firing_position_type_mismatch` matches the `_ticks` field;
  - `firing_variant_definition` matches the getter's name.

## Strip tests (reviewer)

| Variant | Result | Reading |
|---|---|---|
| Drop the 3 `(real_vector3d *)` casts on `add_vectors3d` | 76/76, 183 of 183 sections identical; +3 C4133 | Casts are type-required and byte-inert. The rule-24 narrow admission is met: per-site /Od pushes point addresses to the vector helper, the layout is identical `real[3]`, the caller is exact, and the manifest discloses it. |
| Drop the `(struct unit_datum *)` and `(struct projectile_datum *)` view casts | 76/76, identical | Byte-inert. This is the standard polymorphic downcast after the type test, and it has precedent (`actor_perception.c:4876/5187`, `units.c`, `damage.c`). |
| Merge the threat-loop arms into one `\|\|` store | 75/76 (`input_update` [sha]) | Load-bearing. It stands on the /Od two-store attestation, which I verified. It is disclosed as authentic, not as filler. |
| Merge the mismatch-flag arms into one `\|\|` condition | 75/76 | Load-bearing. It is /Od-attested (two stores, with the else-if re-testing flying). |

The worker's leave-one-out also covered the other features:
- **Load-bearing:** parent object and view, the firing-variant local, the prevehicle encounter local, no stored alias, the flag, the threat loop and the swarm shape.
- **Byte-inert, kept for /Od fidelity:** migrate polarity, separate `normalize3d`, single exit, SD1 and `real_random_range`.

Each load-bearing feature is independently /Od-attested. The only residual that these features steer is the x87 leaf phase of the second inlined `cross_product3d`. r2w1b's `cand_full` shows that a smaller /Od subset also reaches the same sha. The complete-inventory landing avoids selecting among attested features, and that is the right choice.

## Rule and held-class review

- **Rule 6 / owner ruling 5 (all-inlined COMDAT class).**
  - Both helpers are genuine non-static `__inline` functions in `real_math.h` (lines 1080 and 1243).
  - Both COMDATs are byte-identical to January's selected copies, and the callers are strictly exact.
  - The provider link passes in both orders, with no NODUP copy.
  - The patch removes two hand expansions (`add_vectors3d` and `point_from_line3d`).
  - The full zero-regression sweep is the integrator's step. There is no header change, so only `actors.obj` moves.
- **Rule 13.** The hand-inlined refresh, `add_vectors3d` and `real_random_range` are replaced by real calls.
- **Rules 3, 17 and 19.** Single exit. Declarations are combined with their initialisers wherever statement order allows. `real` is used throughout.
- **Removed macros.** The three removed macros were TU-local and have no other users (checked with `git grep`).
- **Other checks.**
  - No header, `symbols.json` or flag change.
  - No dummy local, pragma, volatile, asm, invented aggregate or consumer-local prototype.
  - No rejection is recorded for the unit.
- **Held items.** Neither a round-1 result nor a retirement covers actors. Section 8 lists no held items for actors. The earlier refusals of `point_from_line3d` (Lane A R27, closeout 0919, opus5 w3) predate ruling 5.

## Integrator notes

1. Apply `amended/actors_admission_combined.patch` (or the three frozen parts), then run the full ninja build and the stable/zero-regression sweep. Only `actors.obj` bytes should move. Then run `parked_functions` validation, which should report 84 entries.
2. Do not gate actors with `--forbid-emitted-symbol _point_from_line3d`. This landing emits that COMDAT on purpose, under ruling 5.
3. `_add_vectors3d`'s January provider is `actor_combat`, which is RESERVED for the Codex packet. If that packet changes `actor_combat`'s COMDAT emission or selection, re-run `provider_link` for actors.
4. Credit: +3,056 padded / +3,048 meaningful strict code bytes, 2 functions, and whole-object admission of actors (76/76). No data credit, since data is already at 100%.

---
## Addendum: pass 2 by a second independent reviewer, 09:10

A second reviewer independently re-ran the full battery on objects rebuilt by applying the patches. The verdict is **APPROVE**, in agreement with the above. Details are in `REVIEW_pass2.md` and in the second-pass section of `LEDGER.md`.

Additional non-blocking notes:
- **Apply without `--cached`.** The patches are CRLF, so apply them with plain `git apply` to the working tree. `--cached` fails.
- **Assert-anchor drift.** Drift is +13 at January line 2733 and +21 at 3313, and the 8 lines in between cannot be localised. This is residual risk for the tie-only `/Od` declarations; it is disclosed.
- **Spawn defect.** January's spawn has a latent `actor_get(unit.actor_index)` after the swarm ternary. Production already behaves identically.
- **`odproj/` overwritten.** At 09:02 I overwrote the shared `odproj/` directory. The numbers are identical: 18,686 of 19,130 code bytes and 75/76 functions.
