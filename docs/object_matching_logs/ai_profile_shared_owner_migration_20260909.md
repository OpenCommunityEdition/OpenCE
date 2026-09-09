# AI profile shared-owner migration (2026-09-09)

## Scope

This packet replaces incompatible, consumer-local declarations of the
January `ai_profile` global with one typed declaration in its actual owner,
`source/ai/ai_profile.h`. It is a source-ownership and correctness cleanup;
it receives no exact-byte credit by itself.

The migration covers `ai.c`, `ai_profile.c`, `ai_debug.c`, `actions.c`,
`actors.c`, `actor_perception.c`, `encounters.c`, `ai_communication.c`, and
the Halo Script external-global table. `action_search.c` and
`actor_perception.c` also consume `ai_test_line_of_sight` through its real
owner, `ai.h`, instead of keeping incompatible private prototypes.

## Evidence and names

The complete profile has 28 `0x88`-byte meter records beginning at `0x0C`
and is `0xEEC` bytes overall. Compile-time layout assertions remain with
`ai_profile.c`, while the shared structures, meter enumeration, and external
declaration live in `ai_profile.h`.

The external Halo Script table supplies independent names for the leading
fields: `ai_profile_disable`, `ai_profile_random`, `ai_show`,
`ai_show_stats`, `ai_show_actors`, `ai_show_swarms`, `ai_show_paths`,
`ai_show_line_of_sight`, `ai_show_prop_types`, and
`ai_show_sound_distance`. The table now points at typed fields rather than
raw offsets from an `extern byte` array. Its target and candidate data section
remain exactly equal: 7,076 bytes, 1,322 relocations, normalized SHA-256
`1e4346aef4491a55772f4f51a6989e06074dc5245a2c01aa1be97fb2e221ae8c`.

All meter writers now use the named expression
`ai_profile.meters[_ai_meter_*].accumulator`. This removes raw padding maps,
anonymous `field_B34` members, and mutually incompatible definitions of one
external symbol without changing the authenticated offsets.

## Honest fuzzy retention

`ai_test_line_of_sight` remains parked at zero exact credit. Its retained
source initializes `collision_fraction` to `1.0f` before the fog path; the
older draft could read it uninitialized when the initial collision vector was
clear. January is 1,008 padded bytes and the safe candidate is 1,040, both
with 50 relocations, at 97.97885% objdiff. Correctness takes precedence over
the closer undefined draft.

`ai_debug.obj` still emits a 48-byte private `_point_from_line3d` helper that
is absent from the target object. A frozen build of pushed baseline
`6f8644c0cc7b7748f98b9a4624e89678ff5263c4` emits the identical helper
(zero relocations; normalized SHA-256
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`).
The migration therefore does not introduce this owner debt, but it also does
not claim to resolve it. Attempts to suppress the helper by globally changing
the math schedule regressed five exact owners and were rejected.

Both helper references belong to residual `_ai_debug_render_actor`. A
translation-unit scalar substitute can remove the COMDAT while preserving the
56 already-exact owners, but it worsens that residual from 24,848 bytes and
1,903 relocations to 25,216 bytes and 1,910 relocations (January is 24,976 and
1,905). That source-steering workaround is rejected. Reconstructing the real
large owner is the authentic route to retiring the inherited helper debt.

## Verification

Focused all-owner gates compile the affected translation units without exact
owner loss. `ai.obj` remains 41 exact / 4 residual / 1 unwritten;
`ai_profile.obj` remains 23 / 0 / 0; and `ai_debug.obj` remains 56 / 4 / 0.
The canonical parked ledger is valid with no stale entries. A full build and
stable-verdict sweep are required at batch admission because the shared header
has multiple consumers.
