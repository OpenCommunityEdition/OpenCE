# `first_person_weapons.obj` update-cluster reconciliation (2026-09-12)

## Result

The four previously unwritten January owners now have complete, semantic
implementations in `source/interface/first_person_weapons.c`.  Three close
strictly and one is retained as the best complete fuzzy reconstruction:

| January owner | Semantic owner | Target meaningful / padded | Result |
|---|---|---:|---|
| `_code_000cd610` | `first_person_weapon_new_unit` | 84 / 96 | exact |
| `_code_000cd920` | `first_person_weapon_next_state` | 368 / 368 | exact |
| `_first_person_weapons_update` | unchanged | 236 / 240 | exact |
| `_code_000cda90` | `first_person_weapon_update` | 1,533 / 1,536 | credible complete fuzzy; zero credit |

The strict gain is **688 meaningful / 704 padded bytes in three functions**.
All 26 inherited exact target owners remain exact.  The focused gate moved
from 26 exact / 4 residual / 4 unwritten to 29 exact / 5 residual / 0
unwritten, and the `_point_from_line3d` emitted-symbol guard passed.

The private target owners were renamed in `config/symbols.json` to their
HCEA-corroborated semantic names and marked static.  Their full definitions
and prototypes remain TU-private.  The existing public
`first_person_weapons_update(void)` prototype remains in its owning header.

## Fuzzy boundary

`first_person_weapon_update` is the natural, complete state-machine body.  It
matches the previously frozen best candidate recorded in
`first_person_weapons_obj_update_private_cluster_boundary_20260904.md`:

- target: 1,533 meaningful / 1,536 padded, 59 relocations, normalized SHA-256
  `912d95683f14ff39699c4a1056af92441a20ceeec0584007512ce26b5d70478d`;
- current candidate: 1,520 padded bytes, 60 relocations, normalized SHA-256
  `a112cf32c0e3baf805bd69c440b5beadbe51394f6a7f3161f61d4c470ea88292`.

It receives no exact or semantic-match credit.  The remaining difference is
the established VC7 local scheduling / January inline-schedule boundary,
including an external `magnitude3d` call where January retains the arithmetic
inside this owner. No volatile,
register forcing, noinline/forced-inline directive, raw offset access,
representation punning, inline assembly, dead branch, or other fake matching
construct was introduced.

## Resolved helper ownership boundary

The intake candidate initially emitted six code COMDATs absent from January's
physical `first_person_weapons.obj`:

- `_magnitude3d`
- `_magnitude_squared3d`
- `_square_root`
- `_signed_angular_difference`
- `_real_local_random`
- `_real_local_random_range`

Narrow declaration-only schedules in the owning `real_math.h` now prevent all
six surplus definitions without changing any of the 29 exact target owners.
The final target and candidate each contain 34 code owners. Focused emitted-
symbol guards pass for all six helpers and for `_point_from_line3d`. This
resolves the physical helper-owner debt without forced inline/noinline,
manual expansion, or a false export. The five remaining target functions are
still residual, so this is not a whole-object Matching claim.

## Evidence and verification

January COFF controls bytes, relocations, ABI, and branch behavior.  HCEA
commit `c168af2e747d3095d9a29418ae401f3a39544863` supplies the authenticated
semantic names and reconstruction map; its two-player manager bound was not
copied, because January's disassembly proves the four-local-player loop.

Focused verification used one grouped PID-isolated compilation:

```text
python tools/campaign/gate.py source/interface/first_person_weapons \
  --all \
  --forbid-emitted-symbol _point_from_line3d \
  --forbid-emitted-symbol _magnitude3d \
  --forbid-emitted-symbol _magnitude_squared3d \
  --forbid-emitted-symbol _square_root \
  --forbid-emitted-symbol _signed_angular_difference \
  --forbid-emitted-symbol _real_local_random \
  --forbid-emitted-symbol _real_local_random_range \
  --out scratch/first-person-weapons-packet-c.obj
```

The root integration regenerated the semantic target names, passed the full
Ninja build, and produced a zero-regression stable sweep. Aggregate publication
evidence is in the sixth 10K batch ledger.
