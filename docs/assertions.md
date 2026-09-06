# Assertions, warnings, and halts: shared API and January matching

The owner is [source/cseries/cseries.h](../source/cseries/cseries.h).
All assertions express a condition that **must be true**. A false condition
reports the diagnostic with `display_assert(..., TRUE)` and then calls
`system_exit(-1)`. Evaluate diagnostic expressions only on the failure path.

Warnings likewise report only when their condition is false, but use
`display_assert(..., FALSE)` and **continue**. Halts are unconditional: they
report with `TRUE` and call the existing `halt_and_catch_fire()` owner.
They do not take an assertion condition.

| Intent | Current supported API | Diagnostic |
| --- | --- | --- |
| Plain assertion | `assert(expression)` | Stringified expression |
| Diagnostic assertion | `dassert(expression, diagnostic)` | Supplied string expression |
| Formatted assertion | `vassert(expression, csprintf(temporary, format, ...))` | Formatted string, produced only on failure |
| Preserve original source location | `match_assert(file, line, expression)` | Stringified expression |
| Preserve location and diagnostic | `match_dassert(file, line, expression, diagnostic)` | Supplied string expression |
| Preserve location and formatted diagnostic | `match_vassert(file, line, expression, csprintf(temporary, format, ...))` | Formatted string |
| Plain warning | `warn(expression)` | Stringified expression; nonfatal |
| Diagnostic warning | `dwarn(expression, diagnostic)` | Supplied string; nonfatal |
| Formatted warning | `vwarn(expression, csprintf(temporary, format, ...))` | Failure-only formatting; nonfatal |
| Plain halt | `halt()` | No message; unconditional fatal report |
| Diagnostic halt | `dhalt(diagnostic)` | Supplied string; unconditional |
| Formatted halt | `vhalt(csprintf(temporary, format, ...))` | Formatted string; unconditional |

Warnings and halts also have `match_warn`, `match_dwarn`, `match_vwarn`,
`match_halt`, `match_dhalt`, and `match_vhalt` counterparts. Their first two
arguments are the original `file` and `line`, followed by the ordinary API's
arguments.

`vassert`/`match_vassert` already accepted a message expression before this
cleanup. The `dassert`/`dwarn`/`dhalt` names and matching counterparts are
statement-safe diagnostic convenience wrappers for the existing behavior,
not a claim that these exact wrappers were in Bungie's January source.
Existing callers remain compatible.

The pinned XDK 3911 compiler is VC7 13.00.9254.1. Its preprocessor rejects
`#define vassert(expression, format, ...)` with C2010, and does not implement
`__VA_ARGS__`. Variadic **functions** such as `csprintf` are supported. Do not
change compiler versions, add a function-call wrapper, or force inline code
merely to obtain the newer comma-separated macro syntax.

The supplied CE reference files themselves use two-argument
`vassert(expression, message)` calls: `random_math.c`'s random-seed lock
checks, server line 1248, and client line 521. These calls corroborate the
older API, but the attachments do not include its defining `cseries.h`.
See [the source reconstruction map](user_source_reconstruction_map_20260906.md).

## Units example: assert the invariant, not the error condition

The invariant is that an inventory weapon being dropped is **not** connected
to the map:

```c
match_vassert(
    "c:\\halo\\SOURCE\\units\\units.c",
    8505,
    !TEST_FLAG(weapon->object.flags, _object_connected_to_map_bit),
    csprintf(
        temporary,
        "a %s tried to drop a %s which was connected to the map.",
        tag_get_name(unit->definition_index),
        tag_get_name(weapon->definition_index)));
```

Omitting `!` reverses the behavior: it would assert for a disconnected weapon
and accept the connected weapon. Keep the two `tag_get_name` calls and the
formatting inside the assertion message expression, so successful execution
does not evaluate them.

The matching form retains January's original filename and line number. In a
`NON_MATCHING` build, `MATCH_FILE`/`MATCH_LINE` deliberately use the actual
current source location instead. For new code with no original location to
preserve, use the ordinary `assert`/`dassert`/`vassert` form.

## Limits and adoption

- Use braces around surrounding `if`/`else` branches. Some inherited macros
  expand to an `if` statement; an unbraced outer `else` can bind incorrectly.
- Keep expressions free of unnecessary side effects. The condition is
  evaluated once, and the diagnostic only on failure, but argument evaluation
  order inside a formatted call remains the C compiler's responsibility.
- `temporary` is the existing shared 256-byte buffer and `csprintf` uses
  unbounded `vsprintf`. Preserve authenticated legacy call sites; this is not
  a thread-safe or size-safe general logging facility. Do not pass arbitrary
  unbounded input or preformat into this buffer on the success path.
- Do not add `NDEBUG`-based removal: the current project macros do not have
  that contract. Both debug and `NDEBUG` fixture builds are tested.
- Migrate one coherent owner at a time and run its strict gate plus consumer
  regression checks. Do not bulk-rewrite active lanes or lose original
  diagnostic text, argument types, failure polarity, or source locations.

The executable macro tests are
[tools/test_cseries_assertions.py](../tools/test_cseries_assertions.py), using
[C fixtures](../tools/fixtures/cseries_assertions.c) compiled by the actual VC7
toolchain. They check success/failure, lazy formatting, single evaluation,
exact matching/nonmatching locations, fatal flag and `system_exit(-1)` or
`halt_and_catch_fire()` dispatch, nonfatal warning continuation, and
connected-weapon polarity. They use bounded diagnostic API doubles; they do
not validate the termination implementation, `vsprintf` buffer safety, or the
game's complete error-reporting subsystem.

See also [common constants](common_constants.md) and the
[Units cleanup ledger](object_matching_logs/units_obj_assertion_cleanup_20260906.md).
