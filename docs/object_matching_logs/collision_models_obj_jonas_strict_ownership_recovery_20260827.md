# `collision_models.obj` strict timer-ownership recovery (2026-08-27)

## Scope

- Object: `source/physics/collision_models.c`
- Target: `build/split/source/physics/collision_models.obj`
- Rebuild: `build/base/source/physics/collision_models.obj`
- Compiler edge: the repository MSVC 7 Xbox compile command emitted by Ninja
- Candidate budget: one ordinary code-producing compile after the evidence wave was frozen

This wave revisited the only strict residual in an otherwise byte-matching
object.  It did not change the comparator, add an admission exception, or use
a code-generation directive.

## Frozen baseline

The object contained seven target functions.  Six were strict exact.  The
only residual was `_collision_model_test_vector`:

- target/base padded size: `384 / 384`
- target/base relocation count: `12 / 12`
- normalized SHA-256 on both sides:
  `a72647bf7e687dc91915a48ddfb653f4f4b26d47b35cb698e8410d6ec67e7e51`
- relocation addresses, types, addends, and eleven destinations were already
  identical
- the three BSS references at offsets `+0x14`, `+0x156`, and `+0x15B`
  targeted the same eight-byte timer but used different owners

The split target owned the timer as `_collision_model_test_vector_time` at
BSS offsets `+0`, `+4`, and `+0`.  The rebuild used the decorated
function-local-static owner
`?collision_model_test_vector_time@?1??collision_model_test_vector@@9@9`.

## Evidence

Repository history supplied a direct, previously validated source topology:

- `45f8d684` hoisted the timer from function-local static storage to a
  file-scope `static __int64 collision_model_test_vector_time;` and reported
  the object at `7/7`.
- `337b8395` preserved the ownership change but also added a now-forbidden
  `#pragma bss_seg`; the pragma and externally visible definition were not
  needed for this recovery.
- `config/symbols.json` independently names the target owner
  `_collision_model_test_vector_time`.

The frozen hypothesis was therefore limited to moving the existing timer to
file scope while preserving `static` linkage.  No function logic, declaration
type, use site, or compile option changed.

## Candidate result

The first and only code-producing candidate compile was exact.

- `_collision_model_instance_new`: exact, `112` bytes, `4` relocations
- `_collision_model_test_point`: exact, `224` bytes, `4` relocations
- `_collision_model_test_sphere`: exact, `208` bytes, `4` relocations
- `_collision_model_test_vector`: exact, `384` bytes, `12` relocations
- `_collision_model_test_pill`: exact, `352` bytes, `6` relocations
- `_collision_model_get_features_in_sphere`: exact, `320` bytes,
  `7` relocations
- `_render_debug_collision_model`: exact, `192` bytes, `3` relocations

That is all seven functions and all `1,792` target code bytes.  The target and
rebuild BSS sections are both eight bytes with no relocations and normalize to
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.
Every timer use now resolves through the meaningful owner
`_collision_model_test_vector_time` with the target's exact addend.

The source remains ordinary C89.  It introduces no assembly, pragma,
intrinsic, attribute, volatile/barrier construct, address artifact, pointer
pun, or comparator/admission exception.

## Verification

The retained source was checked with:

1. an MSVC `/Zs` parse-only pass;
2. one normal Ninja object rebuild;
3. strict `tools.coff_compare` comparison of every target function;
4. strict `section_info_by_number` comparison of the complete BSS section;
5. the full repository build, semantic-progress, admission, parked-function,
   unit-test, diff-check, and policy-scan gates recorded with the commit.

Final gate results:

- `halobetacache_build`, `libcmt_build`, `semantic_progress`, and `progress`:
  pass
- semantic report: `4,338` functions evaluated, `4,192` strict semantic exact,
  `4,252` accepted, zero unit errors
- object admission: zero candidates, contradictions, or revocations
- parked functions: three active, zero stale, zero invalid
- repository tooling: `179/179` tests pass
- clean forced replay: `7/7` functions exact; complete BSS section exact
- changed source paths: only `source/physics/collision_models.c`; zero deleted
  or protected paths
- `git diff --check`: pass
- added-source policy scan: zero prohibited constructs
