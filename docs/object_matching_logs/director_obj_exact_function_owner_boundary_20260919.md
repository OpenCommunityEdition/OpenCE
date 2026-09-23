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

## Resolution on 2026-09-24

The historical blockers above are resolved at canonical `b0992d0e`; the
September 19 status is not current. Director still gates 28/28 strict exact.
All 57 January-owned code and non-code symbols match the rebuilt object in
storage class, section, offset, normalized bytes and relocations. Its `.bss`
is now the same 1,002-byte section, with `_director_globals` at 0,
`_director_camera_switch_fast` at 1000 and private `_hyper_key_down` at 1001.

The only extra code COMDATs are genuine shared-header inlines
`_dot_product3d` (32 bytes) and `_cross_product3d` (64 bytes). Each is
byte-identical to January's selected ActionCharge or ActionObey copy.
Ordinary VC7 partial links of the rebuilt Director with each rebuilt provider,
in both input orders and as a combined three-object link, produce no duplicate
definition error. Eight extra literal/data COMDATs are likewise identical to
their January selected providers and pass two-order provider links. The partial
links have expected unresolved imports because unrelated objects are omitted;
they establish provider compatibility, not a complete executable link.
The source fake-match scan has zero leads. Both stale admission rejections are
retired and Director is admitted as `Matching`, with zero newly matched code
bytes because its 28 functions were already credited.
