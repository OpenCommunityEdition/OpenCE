# `director.obj` exact-function closeout and owner boundary — 2026-09-19

## Result

`_director_process_variables` is now strict exact at 544 padded bytes with all
27 relocation identities. Together with the inherited rows, every January
target function in `source/camera/director.obj` compares exact. The object
nevertheless remains `NonMatching`: candidate-only math COMDATs and unresolved
COMMON-versus-`.bss` global ownership fail whole-object admission.

## Natural source repair

The retained source gives the four private constants semantic names and
preserves January's expression shapes:

- `ticks_per_millisecond` (`0.03f`);
- `friction` (`5.f`);
- `acceleration_scale` (`25.f`);
- `genius_boy` (`1.3f`);
- the direct `pow((double)genius_boy, (double)speed_delta)` call; and
- `friction * director_globals.dtime` in January's multiplication order.

The private symbols replace anonymous real-literal names in `symbols.json`.
No address-derived name, compiler flag, pragma, volatile/register steering,
fake dependency, assembly, undefined behavior, or byte transcription is used.
The earlier fuzzy park for `_director_process_variables` is removed because
the function itself is proven exact.

## Remaining whole-object blockers

The rebuilt object emits select-any `_dot_product3d` and `_cross_product3d`
helper bodies that the January Director split does not own. January's selected
copies are recorded as no-duplicates; mixed target/candidate link probes fail
with duplicate-definition errors rather than demonstrating compatible
coalescing. The rebuilt globals also use COMMON ownership where the January
split records `.bss` ownership. These are ownership/layout questions, not
function-byte gaps.

Accordingly, Director receives exact function credit but no whole-object
admission. `config/object_admission_rejections.json` records the fail-closed
boundary so the treemap cannot turn the unit green from function equality
alone.

## Reopen criterion

Authenticate source/header topology that suppresses the surplus helper owners
or prove disposal against compatible rebuilt canonical owners with an ordinary
link, then reconcile the COMMON/`.bss` global ownership without source-shaping
constructs. Preserve every exact Director function while doing so.
