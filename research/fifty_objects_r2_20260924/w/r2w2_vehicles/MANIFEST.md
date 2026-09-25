# MANIFEST: source/units/vehicles (round 2 wave 2, agent r2w2_vehicles)

Zero-credit admission fix under an existing canonical precedent. Both conditions of owner ruling 20260921 #1
hold, and so do the brief's rule-6 COMDAT conditions. The packet gains no strict function: the object stays
37/39 and `NonMatching`. The two residuals are held classes, which this packet does not touch.

## Base and tip

- Worktree `C:\halo-worktrees\claude-fifty-objects-r2-20260924`. HEAD `434f0151` is canonical `05255584` plus one
  ledger commit that touches only `docs/object_matching_logs/claude_fifty_objects_r2_20260924.md`. No source, config
  or tools change.
- `source/units/vehicles.c` is unmodified (blob `3dede853`). `git status` shows only the untracked research dir.
- Candidate source `cand.c` (sha1 `59c37e96`, LF blob `e6710a63`). Candidate object `cand.obj` (sha1 `7c109034`).

| Patch (apply in order) | sha1 | What it does |
|---|---|---|
| `patches/01_vehicles_fallback.patch` | `7e45032c` | `source/units/vehicles.c`: removes the TU-private `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` define/undef and replaces the hand expansion in `vehicle_find_pathfinding_surface_index` with the two `/Od`-attested `point_from_line3d` calls |
| `patches/02_symbols_turret_static.patch` | `2a100d69` | `config/symbols.json` l.7052, in place: `_update_turret_physics` gets `"static": true` |
| `patches/optional/03_OPTIONAL_pathfinding_scale_vector3d.patch` | `3639cd62` | OPTIONAL, not part of the covered packet (see "Optional hunk" below) |

- Both patches pass `git apply --check` at HEAD. Applied to the HEAD blobs outside the repo, they reproduce `cand.c`
  (`e6710a63`) and `cfg_T/symbols.json` (`c602dcdc`).
- The hunks are byte-identical to the suggested round-1 files (`patches/orig/`). I regenerated only the `index`
  lines, which were stale: the round-1 symbols pre-image was `3da39456` but HEAD is `f9abb17d`, and the vehicles
  post-image was recorded as `e6e72f2e` but is actually `e6710a63`.
- Integrator order: apply 01 and 02, regenerate the split with csplit only, then run a full ninja. 01 and 02 are
  independent. Production's `update_turret_physics` is already `static` in source, so 02 corrects only the split
  side.

## Precedent (governing ruling) and its conditions

**Ruling.** Owner ruling 2026-09-21 #1 (`docs/object_matching_logs/claude_lane_b_HANDOFF_20260920.md:65-68`):

> "`_point_from_line3d` guard relaxed narrowly: a TU may emit it iff January's SAME object REFERENCES it out of
> line. There are 17 such objects (list in section 7)."

**The 17 objects.** `:156-162` lists them: "ai_debug, real_math, collision_features, units, action_vehicle,
actor_combat, actor_moving, actor_perception, ai, items, projectiles, collisions, rasterizer_lights,
rasterizer_xbox_environment, rasterizer_xbox_models, bipeds, vehicles (plus action_charge, the credited owner)".

**Canonical landings of this ruling.** Each one used the same form: plain removal of the TU-private define/undef
pair, no source comment, and disclosure in the commit/ledger.
- `59925575` (action_vehicle)
- `6099e659` (items/projectiles)
- `bf937aac` (actor_perception: "`_point_from_line3d` is emitted under ruling 20260921 #1 (January references it out
  of line here); the copy is byte-identical and passes the provider link")
- `4f4c8502` (lens flares)

The COMDAT conditions those landings verified come from the brief's rule 6 and ruling #2: byte-identical to
January's selected copy, a strictly exact caller, zero regressions, and a provider link in both orders.

**Storage precedent.** `05255584` sets rows `"static": true` in place per cachebeta publics.

| Condition | Holds? | Evidence |
|---|---|---|
| vehicles is one of the 17 | YES | `refcensus_split.txt`: `build/split` `source/units/vehicles.obj` holds an UNDEF `_point_from_line3d`, as do exactly the other 16 listed objects. `action_charge` DEFINES it. |
| January's SAME object references it OUT OF LINE | YES | `relocsites.py`: January vehicles has 1 relocation, `_vehicle_update +0x268 -> _point_from_line3d`. The candidate has exactly that one; production has it plus a surplus one at `_update_alien_fighter_physics_new +0x138`. |
| Form: TU-private guard removed, no hand expansion | YES | Patch 01 deletes both guard lines and adds no comment, the same form as the four precedent commits. It also REMOVES the rule-6 hand expansion, its stale "Preserve the January inline schedule" comment and the `real t` staging local. `cand.c` has no `REAL_MATH_EXTERNAL`, `_inline` or `#define` besides the pre-existing `VEHICLE_ANGULAR_ACCELERATION`. |
| COMDAT byte-identical to January's selected provider | YES | `_point_from_line3d` is 48/0/`9b763841f851` in the candidate and in `build/split` `action_charge`. `surplus_identity` (redirected): IDENTICAL; 28 candidate-only code COMDATs, 0 not identical. |
| Emitting caller strictly exact (brief s2 rule 6; wave-3 law (i), P1 class) | YES | The COMDAT is anchored by `_vehicle_update` (out-of-line call, EXACT) and `vehicle_find_pathfinding_surface_index` (2 inline sites, EXACT). Lab `lab_nofighteruse.c` deletes the non-exact fighter's pre-existing use: the COMDAT is still emitted and `section_infos_equal` to the candidate's copy. P1 therefore does not apply, because no new use enters a non-exact function. |
| Provider link, both orders | YES | `provider_link --baseline=base`: `_point_from_line3d` vs `action_charge` PASS. The full run gives 86/86 rows PASS (SELECTED-PROVIDER LINK: PASS). `nodup_census`: 26 definers on the base board, NODUP none. |
| Zero regressions | YES | Gate: 37/2 before and after, and no row changes verdict. Owner-keyed diff of base vs candidate: only the fighter bytes differ, plus the new COMDAT. No header is touched, so no other object recompiles differently. The emulated split changes only vehicles.obj. The integrator's full ninja is the formal confirmation. |
| Storage follows cachebeta publics (05255584) | YES | `_update_turret_physics` (0x1A4F50) is absent from the cachebeta publics. Its neighbours `_vehicle_hover` 0x1A4F00, `_vehicle_is_flipped` 0x1A4F70 and `_set_real_quaternion` 0x1A5040 are present, so January had it file-static. No relocation names it anywhere in `build/split`: it is inlined at its only site, which is the static-emission law. `pdb_storage`: production 1 disagreement, candidate 0. |

**Differences from the precedent.** None. This is the same construct in an object that is on the list.

## Source evidence per changed construct

**Removing the guard.** January vehicles holds exactly one out-of-line reference, in `_vehicle_update`, and the
fighter inlines the helper. The external declaration forced an extra call in the fighter: 33 relocations against
January's 32. With the header `__inline` the fighter reaches 32/32 and `real_regions` falls from 5 REAL regions to 3
(369 to 363 instructions against January's 363). `_vehicle_update` stays EXACT with the out-of-line call.

**Pathfinding.** `/Od` `halo_cache_symbols.exe` fn 0x8fd970 (`od_pathfinding.txt`):
- call 0x42e0d0 at 0x8fda42, args (`&origin` [ebp-0x444], `[0xa31e2c]` global_up3d, 0.4f @0x93dd48, `&origin`);
- call 0x42e0d0 at 0x8fdad2, args (`&origin`, `&vector` [ebp-0x458], `result.t` [ebp-0x430], `position`
  [ebp+0xc]).

fn 0x42e0d0 (`od_pfl3d_0x42e0d0.txt`) is `result->x = v->i*t + p->x` (x3) and returns `result`, which is
`point_from_line3d(p, v, t, result)`. The candidate spells exactly these two calls with the same argument order.
January inlines both sites, and the function is EXACT before and after: 304/15/`0aa51d7685af`.

## Target vs candidate (gate rows; padded size / relocs / normalized sha)

| Function | January | Production | Candidate |
|---|---|---|---|
| `_vehicle_find_pathfinding_surface_index` | 304/15/0aa51d7685af | same, EXACT | same, EXACT |
| `_vehicle_update` | 2320/98/97b1962975b8 | same, EXACT | same, EXACT |
| `_update_turret_physics` | 32/1/3a21ab6d0c01 | same, EXACT | same, EXACT |
| `_update_alien_fighter_physics_new` | 1088/32/a6f30f76266b | 1088/33/2415b93027e4 [relocs, sha] | 1088/32/04b515cc423c [sha] |
| `_update_alien_scout_physics` | 2464/91/c3b6daa95a50 | 2464/91/c0c804247f7b [sha] | unchanged [sha] |

- `gate.py --all`: base `== exact 37 residual 2`, candidate `== exact 37 residual 2`. The same holds against the
  emulated split (`cand_vs_splitT.gate.txt`).
- The gate-compiled `base.obj` matches `build/base` vehicles.obj on all 172 sections and 450 symbols.

## Section-4 audit against the emulated split

- **Emulation.** `build/tools/csplit.exe -i cachebeta.exe -p cfg_X -o split_X`. `mk_cfg.py` makes an in-place line
  copy of `config/`.
  - `split_ctl` matches `build/split` on 833/833 files.
  - `split_T` differs from `split_ctl` in 1 file, `source/units/vehicles.obj`. In it, 68/68 sections are
    `section_infos_equal` and 1 symbol row changes: `_update_turret_physics` storage 2 -> 3.
- **`object_audit`, candidate vs `split_T`.** 66 January-owned sections ok (28 `.rdata`, `.data`
  `_vehicle_update_section` and 37 `.text`). 2 DIFF: the held fighter and scout. 68 January symbols, 0 differ.
  FAIL(2) is residuals only.
  - Base vs `build/split` is FAIL(3): the same two plus the turret storage row.
  - Surplus delta vs base is exactly +`.text _point_from_line3d` (48 B).
- **Data.** `tinfo --data` is identical between candidate and production, apart from section numbers and the
  `.debug$S` object path. `report.json` vehicles data is 1744/1744. The candidate changes no data section.
- **Surplus and provider link.** Covered in the precedent table above (surplus_identity 0 differ; provider_link
  86/86 PASS in both orders).
- **Rejections and parks.** No vehicles entry in `config/object_admission_rejections.json` or `config/parked.json`.

## House-rule review of the changed lines

- Genuine header helper with no cast.
- `real` literal `0.4f`, as production already uses.
- No new name, prototype, macro, pragma or local. The `real t` staging local is removed.
- Removes a rule-6 hand expansion and a TU-wide inline-suppression macro.
- `/W3` diagnostic multiset: 27, identical for base and candidate. No new C4133.
- `fake_match_scan`: 0 leads for both.

## Tests run

These are all read-only, or write into this slug only:
- `gate.py` for base, candidate and the labs;
- `with_split.py`-redirected gate, `object_audit`, `pdb_storage`, `surplus_identity` and `provider_link`;
- `real_regions`, `warn.py` (`/Zs /W3`), `fake_match_scan`, `nodup_census`;
- csplit emulation (ctl + T);
- `odbuild` on 0x8fd970, 0x42e0d0 and 0x42e220.

I did not run ninja, configure or pytest.

## Credit

- **Meaningful and padded code:** 0 strict functions and 0 bytes.
- **Data:** 0 (already 100%).
- **Whole object:** not completed; config status stays `NonMatching`.
- **Admission value:** removes a rule-6 hand expansion and a TU-wide suppression macro. Fixes the only storage
  disagreement with cachebeta publics (object_audit symbols 1 -> 0 differ). The fighter moves from a structural S1
  residual (relocation multiset) to a sha-only tie-shaped one.

## Optional hunk (NOT covered by the audited precedent; integrator's choice)

- **What it changes.** `patches/optional/03`: `add_vectors3d(global_down3d, global_down3d, &vector)` becomes
  `scale_vector3d(global_down3d, 2.0f, &vector)`.
- **Evidence.** `/Od` calls 0x42e220 at 0x8fda65 with (`[0xa31e38]` global_down3d, 2.0f @0x93dd64, `&vector`).
  0x42e220 is `result = s*v` (x3), which is `scale_vector3d`.
- **Measured.** `lab_scale.obj` is section- and symbol-identical to `cand.obj` for the whole object: VC7 folds `*2`
  to `fadd`. The `/W3` set is identical.
- **Why it is optional.** It is a byte-inert `/Od` spelling of a pre-existing exact line, and it was not part of the
  precedent audit.

## Held items and reopen criteria (unchanged by this packet)

- **`_update_alien_fighter_physics_new`.** Owner "keep the fighter held" (`3ba2eb91`), plus the actor_combat NODUP
  `cross_product2d` provider, which is RESERVED to the Codex packet. Round-1 f1_held (view-cast
  `cross_product2d`) is EXACT on top of this packet.
  - Reopen when the Codex actor_combat provider repair lands and the owner releases the fighter hold.
- **`_update_alien_scout_physics`.** HELD_CLASS: the leveling `point_from_line2d` is a count-oracle landing that
  contradicts `/Od` 0x8f8b4a (review_vehicles R7). Two aggregate 3D->2D view copies are load-bearing, which is the
  open owner question in structural packet s5 (R11).
  - Reopen on an owner ruling for s5, together with a genuine, `/Od`-consistent 1-temp construct for the S3 M8
    leaf.
- **Whole object.** The inherited invented private helper owners `_vehicle_{cross,dot,triple}_product3d_target`
  and `_vehicle_dot_product3d_test` (boat and preprocess) still block admission even with 39/39 exact. The
  round-1 boat /Od transcription left 17 D-invariant blocks.
