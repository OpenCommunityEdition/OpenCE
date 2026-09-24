# REVIEW: source/objects/damage (worker: scratch/w/damage, FUZZY_IMPROVED)

Reviewer slug: `scratch/w/review3_damage/`. Tree: 954eebd2. Every row below was re-measured independently from files the reviewer rebuilt
from the patches. The worker's .c and .obj files were not used as inputs. The full probe log is in `LEDGER.md` here.

## Verdict

| Item | Verdict |
|---|---|
| `_object_damage_body` strict exact (static + /Od child walk) | **APPROVE** |
| Patch B `scratch/w/damage/production_minimal.patch` | **APPROVE** |
| Amended B+ `scratch/w/review3_damage/production_reviewed.patch` (B plus worker item 4) | **APPROVE, preferred** |
| Patch A `scratch/w/damage/production.patch` (the worker's PREFERRED patch) | **REJECT** for self-landing; owner-gated |
| `scratch/w/damage/config/symbols.json` (4 in-place `"static": true` lines) | **APPROVE** |
| parked.json: retire `source/objects/damage:_object_damage_body` | **APPROVE** |
| parked.json: re-baseline the AOE park (A only) | **REJECT** (A is not landing; under B or B+ the park stays valid unchanged) |

Net for the integrator: +1 strict function (1360 padded / 1347 meaningful). The object stays incomplete because AOE is still residual.

## Independent measurements

- **Patch application.** `git apply --check` is clean for A, B and B+ against the current tree. Each patch was applied to
  `git show HEAD:source/objects/damage.c` in a throwaway repo and CRLF-converted. The results match byte for byte: A == final.c,
  B == minimal.c, B+ == rev_Bplus.c.
- **Gate.** `gate --all --forbid-emitted-symbol _point_from_line3d` passed the guard in every run:

  | Build | Result | AOE |
  |---|---|---|
  | production | 32 exact / 2 residual | 1312/52 |
  | rev_A | 33 exact / 1 residual | 1312/51 |
  | rev_B | 33 exact / 1 residual | 1312/52 |
  | rev_Bplus | 33 exact / 1 residual | 1312/52 |

  January's AOE is 1328/51. `_object_damage_body` is `section_infos_equal` to January in A, B and B+.
- **Park snapshots** (measured with tools.coff_compare.section_info, as parked_functions does):
  - Under B and B+, AOE is 1312/52 sha `46952a11...`, identical to the current park base. No re-baseline is needed.
  - Under A, AOE is 1312/51 sha `93606db1...`, which confirms the worker's value.
- **Storage truth.**
  - cachebeta publics have no symbol at all at 0x126090, 0x127020, 0x128150 or 0x456E48. This is an address check, not only a name
    check: the mapping was confirmed on neighbouring publics such as `_object_cause_damage` at 0x127570.
  - HCEX.pdb reports `static function: true` for object_damage_body, area_of_effect_cause_damage_to_object and
    object_destroy_notify_children.
  - No other object in build/split (833) or build/base (621) names any of the four symbols. No header declares them.
- **Emulated split.**
  - A control csplit from a config copy matches build/split byte for byte (833/833).
  - With the worker's symbols.json, csplit changes only `source/objects/damage.obj`. The copy differs from the tracked file in exactly
    lines 5108, 5126, 5129 and 23108 (+64 bytes, not re-serialised).
  - `object_audit_split` against the emulated split gives the same result for A, B and B+: 47 January symbols, 0 differ, and the only
    differing section is AOE.
  - pdb_storage against the emulated split: 0 disagreements.
- **Surplus.**
  - B and B+ emit the same 8 code COMDATs as production, all IDENTICAL to January's selected copies. provider_link PASS.
  - A adds `_collision_test_line` and `_real_random`. Both are identical to action_charge's copies and provider_link passes. The problem
    with them is covered under patch A below.
- **Data.** 760/760 in build/report.json. All .rdata/.bss sections pass the audit.
- **Scans.** fake_match_scan: 0 leads for A, B and B+. `CL /W3 /Zs` with the ninja cflags (`w3.py`): only the 12 header diagnostics
  that production also has. 0 TU-local warnings and no C4013.

## Strip tests (body)

| Variant | Body result |
|---|---|
| production + `static` only (typed unit_get re-lookup kept) | 1392/44 residual |
| B without `static` on the body | residual [sha] |
| production + all 4 statics, no child walk | 1392/44 residual |
| B+ with the named `child_unit` replaced by an inline `((struct unit_datum *)child_object)->...` | **EXACT** |

Both levers are load-bearing, and each has independent first-party evidence:

- **`static`:** cachebeta publics (by address) and HCEX.
- **Walk without a second `object_get_and_verify_type`:**
  - January's bytes and /Od 0x770491..0x7704d9 both have exactly one verify call, `object_get(child, -1)` into `[ebp-0x2c]`.
  - Then comes the unit-mask `and eax,3`, and `[ebp-0x2c]` is copied into `[ebp-0x30]` with no verify call.
  - The player_index test (+0x1dc) reads through `[ebp-0x30]`, and the next index is read from `[ebp-0x2c]`.
- **Named `child_unit` local:** byte-inert. It is kept because /Od gives it its own slot `[ebp-0x30]`, declared after `child_object`,
  which matches the nested block's declaration order. The same idiom appears verbatim in admitted units.c (1719-1727 `rider`,
  8089-8093 `child_unit`), and also in ai_script.c:3505. There is no typed pointer-downcast macro; `unit_get` would add the verify call
  that January lacks.
- **Park rejection:** this is not the "repeated raw object_get downcast" that the park evidence rejected. It is one generic fetch, a type
  test, and a single view of the same pointer.

## Why patch A is rejected (owner-gated, not self-landable)

1. **Rule 6 (docs/campaign_house_rules.md exception; brief section 2).** A TU may emit a shared-header `__inline` COMDAT only when the
   caller is strictly exact. Patch A restores the genuine `collision_test_line` (collisions.h) and `real_random` (real_math.h).
   - This is /Od-attested: 0x76c50b and 0x76c558 call 0x432880, and 0x76c6ca calls 0x42e1c0.
   - But it newly emits `_collision_test_line` and `_real_random`, and their only caller is AOE, which is not strictly exact.
   - January's damage.obj references neither symbol.
   - This is the same class as the held actor_combat P1 item: a header COMDAT whose caller is not yet strictly exact. It needs an owner
     ruling.
   - Production's hand-expansions are existing debt in a zero-credit parked function. Patch B does not touch them.
2. **Item (5) overstates "/Od-faithful".**
   - The /Od clamp frame has distance `[-0x13c]`, scale `[-0x140]`, radius_delta `[-0x144]`, and computes scale as a `?:` (temp
     `[-0x168]`) followed by PIN. A keeps production's `distance, radius_delta, scale` order and an if/else clamp.
   - The worker's own v1_declorder and od2 variants had the /Od forms, but final.c does not include them.
   - This is byte-inert but falls under law (d).
   - The core block (x_axis, y_axis, `collision_blocked = TRUE`, axis, `short ray_index`, collision, offset, offset_point) does match the
     /Od frame.
3. A would also re-baseline an AOE park for zero credit. B and B+ leave that park valid as it stands.

**Reopen criteria for A's AOE half:**

- AOE goes strict-exact (the remaining gap is the `fxch st(1); fstp st(0)` vs `fstp st(1)` kill form at +0x42e), or the owner admits
  the P1 class.
- Either way, take od2's `?:` clamp and v1's `distance, scale, radius_delta` order at the same time.

## Amended patch B+ (reviewer-produced, verified)

`scratch/w/review3_damage/production_reviewed.patch` is production_minimal.patch plus the worker's item (4). Item (4) drops the
non-/Od `byte region_damage` local and re-reads `object->object.region_damage[region_index]`.

- /Od 0x7702f1..0x770347 has no slot for such a local. It stores the byte and then re-reads it with `movzx [edx+ecx+0x134]`.
- `*(1.f/255.f)` is kept because January's bytes use fmul 1/255. The /Od build's divss by 255 is later-revision source.
- B+ passes every check above: gate 33/1 with the guard passed, body strict-equal, AOE sha equal to the park base, audit 0 symbol
  differences against the emulated split, surplus identical, provider_link PASS, 0 fake leads, 0 TU-local warnings.
- Plain B is also acceptable. B+ is preferred because it is the plainer, /Od-attested spelling in a function that now takes credit.

## Integrator instructions (mechanical)

1. `git apply scratch/w/review3_damage/production_reviewed.patch`, or `scratch/w/damage/production_minimal.patch` for plain B. Apply
   exactly one of them. Do **not** apply `scratch/w/damage/production.patch`.
2. config/symbols.json: add `, "static": true` in place to lines 5108 (`_object_destroy_notify_children`), 5126
   (`_object_damage_body`), 5129 (`_area_of_effect_cause_damage_to_object`) and 23108 (`_damage_globals`). The full copy is
   `scratch/w/damage/config/symbols.json`. Then run a csplit-only regen; only `build/split/source/objects/damage.obj` changes (verified).
3. config/parked.json: delete the `source/objects/damage` / `_object_damage_body` entry. Leave the
   `_area_of_effect_cause_damage_to_object` entry unchanged (base 1312/52/46952a11... still measures equal).
4. No shared header is edited. The blast radius is damage.c, plus the damage.obj split regen.

**Still blocking whole-object admission:** the AOE x87 kill-form residual; the consumer-local `boolean unit_unsuspecting(...)`
prototype (its owner is units.c); and the consumer-local `extern boolean debug_damage;` (public in cachebeta, with no definer in source).
