# `damage.obj` Jonas wave-two checkpoint

This Jonas-owned ledger records a bounded, source-only recovery in
`source/objects/damage.obj`. The unit advances from 15/34 to 17/34 strict
functions and remains `NonMatching`. The prior Codex checkpoint and every
pre-existing or Claude-authored Markdown file remain untouched.

## Scope and provenance

- Authoritative integration base:
  `1953e260372c6fc1dd74ab078cbdeb45d348e26a`.
- Baseline `damage.c` blob:
  `138e904acf7a14457edc6d2145301a3975a10a63`.
- January split `damage.obj` SHA-256:
  `3fe5495d2a6ce3075c3fa6b04782cb7bbf97c3a3890f2e6f6f4f29ef8e5b08ec`.
- HCEA semantic oracle commit:
  `c89106c4964f2df3a98ef7b4035d6750372797cd`; its
  `object_initialize_vitality.c` blob is
  `f3f425eaff8c5a0a9fa1ab9417b272f0b36602c7`.
- HCEA supplied names, types, and behavior only. The January PC object was the
  sole byte and relocation authority, and the retained C was written against
  this repository's typed definitions.
- Only `source/objects/damage.c` and this new ledger are changed. The frozen
  `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units` objects and all
  configuration are untouched.

## Bounded experiment

One three-body candidate wave covered vitality initialization, overshield
charging, and child-notification recursion. The first and third were strict on
the first compile. `_object_double_charge_shield` had the target's 96-byte
section and all three relocation identities but only 96.25% ordinary similarity,
so its body and temporary declaration were removed without a tuning pass.

The other compact-looking residuals were rejected before compilation:
`_code_001260e0` and `_code_00126230` use an EAX input convention that cannot be
expressed by this lane's defined-C policy, while `_object_deplete_shield` and
`_area_of_effect_cause_damage` require still-nonexact same-TU callees. No caller
depending on a nonexact local body was retained.

## Strict code result

The untouched baseline measured 15/34 functions and 675 meaningful code bytes.
The retained source measures 17/34 and 968 meaningful bytes: a gain of two
functions, 293 meaningful bytes, 304 padded bytes, and 12 relocations.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_object_initialize_vitality` | 214 | 224 | 8 | `b52a9b328d59c696720c3706320e56a1dce3a9d1690c81219bd690abe02be221` |
| `_code_00126090` | 79 | 80 | 4 | `4c97a3277a8f6e2fa6c7c0c96049f5d2e39c7d6672a8114f381c226fc2d6ff3e` |

Independent semantic COFF comparison reports both functions equal, including
every relocation address and destination. The same direct comparison reports
all 15 pre-existing exact functions equal. No data definition changed; the
target's 760 non-code bytes remain uncredited.

Adding two function COMDATs changes raw section-number and symbol-ownership
fingerprints for some existing relocation-bearing functions and shared float
COMDATs. The clean fail-closed regression snapshot therefore reports exactly
the two intended `NEWLY_EXACT` functions plus those structural evidence changes;
it reports no changed nonexact function. Direct semantic comparison is the
acceptance authority for the preserved 15-function baseline. A clean committed
snapshot and forced post-commit replay establish the reproducible 17-function
state.

## Source policy and gates

The retained implementation is readable typed C with one parameter per line and
explicit terminal returns. It uses named object, tag, collision-model, and
damage-resistance fields. It contains no assembly, volatile scheduling device,
forced inline, optimizer pragma/barrier, raw layout cast, pointer/integer address
reconstruction, union pun, synthetic anchor, or byte-forcing device.

The complete Halo and libcmt builds pass. Semantic audit reports 470 units,
3,962 functions evaluated, 3,822 semantic exact, 3,883 accepted exact, and zero
unit errors. Campaign progress is 375/833 complete objects, 3,870/11,060 exact
functions, 471,201/2,198,102 exact code bytes, and 1,834,808/4,176,062 matched
data bytes.

Admission reports zero candidates and zero revocations, with only the inherited
`source/shell/shell_xbox` contradiction. Park validation reports three active,
zero stale, and zero invalid entries. The complete tooling suite passes 179/179
tests. No GitHub push is performed.
