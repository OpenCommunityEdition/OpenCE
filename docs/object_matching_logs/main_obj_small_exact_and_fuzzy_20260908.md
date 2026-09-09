# `main.obj` small exact and credible fuzzy packet (2026-09-08)

## Result

`main_skip_private` is strict exact against January and adds 163 meaningful /
176 padded code bytes.  All inherited exact functions remain exact.

`main_load_last_solo_map` is retained as the most complete source-credible
candidate found in the reconciled Claude lanes.  It is still fuzzy and receives
zero exact-match credit.  Its remaining compiler-shape differences are not
papered over with fake temporaries, nonsensical control flow, or guessed raw
offsets.

## Ownership and style

Public declarations live in `main.h`; private declarations remain in
`main.c`.  The packet uses project types and named subsystem interfaces,
contains no `code_` or `bss_` production identifiers, avoids forced inlining,
and preserves the January helper-emission schedule.  The fuzzy body is kept so
future work begins from the strongest credible reconstruction rather than an
empty stub.

## Verification boundary

Only the 163 meaningful / 176 padded strict closure is credited.  Publication
requires the PID-isolated gate, forbidden `_point_from_line3d` check, global
zero-regression sweep, full build, and full tests.
