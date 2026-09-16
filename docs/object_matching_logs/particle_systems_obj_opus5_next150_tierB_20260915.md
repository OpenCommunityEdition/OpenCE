# `particle_systems.obj` — next-150K lane, final implementation wave, Tier B row B7 (2026-09-16)

## 1. Scope and provenance

- Unit `source/effects/particle_systems.c`; target `build/split/source/effects/particle_systems.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `6d6529d98`. Worker `tB:particle_systems`.
- Scope: exactly one function, `_particle_system_new_particle_jet` (research campaign Tier B row **B7**, 365 meaningful).
- Nothing under `source/`, `config/`, `tools/` or `build/` was written. No ninja, configure, pytest or git write
  command was run. `git diff --stat -- source/effects/particle_systems.c` is empty.
- Worker log with the full report block: `scratch/workers/tB_particle_systems.md`; evidence in
  `scratch/workers/tB_particle_systems/`.
- Read first: both worker briefs, `opus5_next150_research_campaign_20260916.md`, `scratch/w2/laws_w2.md`,
  `scratch/w3/laws_w3.md`, and every particle_systems ledger that mentions the function (house_clean 0913, 100K,
  150K w1, 250K w3c, next150 n4) plus the three worker logs.

## 2. Baseline and final

Real-file gate `gate.py source/effects/particle_systems --all --forbid-emitted-symbol _point_from_line3d`:
`== exact 21  residual 4  unwritten 0` at start and at finish (identical listing). Guard passed. No parks.

| row | target padded/meaningful/relocs/sha16 | baseline | final |
|---|---|---|---|
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11 `[sha]` | unchanged (NOT-LANDED) |

The other three residual rows (`_particle_system_render`, `_particle_system_update`,
`_particle_system_update_particle_default`) were out of scope and not touched.

## 3. Census (written before any shape)

- **Size / instructions:** 368 = 368; 136 = 136 instructions.
- **Frame:** January `sub esp,0x10` = ours. Every slot is identical: `[ebp-0x10..-8]` random direction aggregate
  (HCEX SHIP name `spread`), `[ebp-4]` `rotates_up`, `[ebp+8]` velocity then spread_scale, `[ebp+0xc]` spread_fraction
  then directed_scale, `[ebp+0x10]`/`[ebp+0x14]` read-only particle/marker. `frameslot.py`: FRAME-CLEAN.
- **Relocations by target:** 11/11, `0 differing row(s)`, every address equal.
- **Other detectors:** `inlinerule.py` CONSISTENT (identical call multiset), `f5detect.py` NOT-FAMILY5,
  `f4law.py` X87-ORDER.
- **Difference:** only the fld/fmul operand-load order inside the two inlined `real_math.h` `cross_product3d`
  copies (17 alndiff lines at +0x11b and +0x136..+0x15b).

## 4. Lens re-verification

**Lens 1 — F2 MOD8-REDUCES D=6 (17 → 8): holds.** The research detector re-run at HEAD gives
D0 17, D1 17, D2 10, D3 12, D4 12, D5 12, D6 8, D7 10, D8 = D0.

**Lens 2 — "ours has two surplus IL locals; removing them re-keys cross_product3d": does not hold as a closure.**
Per-product orientation (a = the first helper argument's leaf is loaded first; products
`a.i*b.j, a.j*b.i, a.k*b.i, a.i*b.k, a.j*b.k, a.k*b.j`):

| body | copy 1 (velocity × global_up3d) | copy 2 (forward × velocity) |
|---|---|---|
| January | `aaabba` | `bbabba` |
| D0 / D1 | `aaabaa` | `babaab` |
| D2 | `aaabaa` | `bbbaab` |
| D3 | `aaaaaa` | `bbbaab` |
| D4 | `baabaa` | `bbbaab` |
| D5 | `aaabaa` | `abbaab` |
| D6 | `aaabaa` | `bbbbab` |
| D7 | `aaabaa` | `baaaab` |

January's copy-1 fifth product (`b`) and copy-2 sixth product (`a`) occur at **no** residue. Every genuine change to
the local inventory is a uniform ordinal shift (research S2.11), so no local-count change can make this function
exact. Independent per-copy shifts cannot either. A brute-force search found no per-leaf additive mod-8 key model
(multipliers 1/3/5/7, either direction, either tie default) that fits even our own eight observations. So the
cross-product order is not a pure ordinal key, which is consistent with research notes S2.12.

**A15 "argument roles": refuted by bytes.** Every `fsubp` minuend/subtrahend already matches January, which
proves `cross_product3d(&particle->velocity, global_up3d, &particle->axis)` and
`cross_product3d(&marker->matrix.forward, &particle->velocity, &particle->axis)`. January's fall-through after
`fcomp 0.0; test ah,0x44; jnp` is the velocity × up arm, which proves the `if (rotates_up)` arm order.

**Surplus-local candidates.** Only `definition` (single use of the `tag_get` macro) is removable (−1 ≡ D7, which
measured non-closing). `velocity`, `spread_fraction`, `rotates_up`, `spread_scale`, `directed_scale` and the direction
aggregate are byte-proven homed values stored before later calls. HCEA's single `axis_reference` cross product is
refuted by January's two distinct inlined copies.

## 5. Shapes

None spent. Requirement 3 applies at baseline, because frame, relocations, call multiset and instruction count are
already identical and only x87 operand ordering remains. The one count lever the Tier B row named is measured
non-closing for all eight residues. The remaining routes are all prohibited:
- operand spellings: R3 and the 100K do-not-repeat list;
- a hand-expanded cross product: R15;
- a TU-local helper copy: R14;
- a `real_math.h` body respelling: header scope, and retired as a lottery in campaign §4.4;
- pointer aliases: R16.

## 6. Classification, proposals, reopen criterion

- **Classification:** x87 commutative operand-load order inside two inlined `cross_product3d` copies. It is NOT an
  IL-count defect. Recommend moving B7 out of Tier B (to Tier C / retire-pending).
- **Proposals:** none.
- **Scan fact:** January contains only 12 contiguous inlined cross products. The only one in an exact function
  (`_object_set_position`) takes a frame-local argument, which is M8-insensitive, so it gives no exemplar.
- **Reopen criterion:** a measured VC7 rule for inlined-`cross_product3d` load order that reproduces January's
  regular pattern here from genuine source. In January, every product loads its k-field leaf first when it has one,
  and otherwise loads the `particle->velocity` leaf first. A January-side fact changing the leaf provenance of
  `particle->velocity`, `marker->matrix.forward` or `global_up3d` would also reopen it.

## 7. Checks

1. Real-file gate unchanged: 21/4/0, row-identical.
2. `_point_from_line3d` guard: passed.
3. `scratch/parkcheck.py`: no parks for the unit.
4. Owner census: not applicable (nothing landed).
5. `tools/fake_match_scan.py`: 0 leads.
6. `git diff --stat -- source/effects/particle_systems.c`: empty.
