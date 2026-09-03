# `director.obj` camera-load closure and scripted-camera park

## Result

This 2026-09-03 pass continues the ordinary-C Director recovery from canonical
commit `4c4ba569dc3badc9d627aef0b9ae5e0fa00602ca`. It closes the previously
unwritten `_director_load_camera` at 336 strict-exact padded bytes and recovers
the complete credible behavior of `_director_script_camera` as an honest
fuzzy residual. The focused Director frontier moves from 23 exact / 0 residual
/ 5 unwritten to 24 exact / 1 residual / 3 unwritten.

The accepted camera loader reads `d:\\camera.txt`, parses the saved position,
forward, up, and field-of-view values, initializes the typed flying camera,
derives its roll, installs the flying-camera update procedure, and records the
authenticated `_camera_flying` mode. Its exact source shape computes the
cross product with `cross_product3d` and immediately passes that typed result
to `dot_product3d`. This natural helper composition reproduces January's x87
schedule exactly without assembly, optimizer controls, or a fake dependency.

| Function | Candidate status | Target bytes | Candidate bytes | Target relocations | Candidate relocations | Comparator |
| --- | --- | ---: | ---: | ---: | ---: | --- |
| `_director_load_camera` | accepted strict exact | 336 | 336 | 26 | 26 | normalized SHA-256 identical |
| `_director_script_camera` | accepted credible fuzzy park | 304 | 256 | 23 | 19 | 86.36364% objdiff; strict residual |

The strict normalized SHA-256 for both target and candidate
`_director_load_camera` is
`7ab64c92e6aeb8cdc65b56e815dcea26357b91356ec09db5bb786c43ad7f4f0b`.

## Scripted-camera residual

The retained `_director_script_camera` body follows the January behavior and
the independent HaloCEA reconstruction: it updates the global scripted-camera
state, loops over local players, resolves the typed Director state, installs
`scripted_camera_update` and resets debug controls when enabling, otherwise
restores game perspective, then calls `scripted_camera_enable`.

January emits a second bounds-checked `director_get` assertion inside each
branch after already resolving the same player state. The natural source uses
one necessary typed accessor. Its target/candidate identities are:

- target: 304 padded bytes, 23 relocations,
  `dfba772232540bf6926d16fb6554f65c256948c84e827cf14e52db1252c203ed`;
- candidate: 256 padded bytes, 19 relocations,
  `fd2c8c5b220ad335bf62943e3e4e0d5065913b93d202bf8e15fff8f140dc8faa`.

A semantically equivalent `director_set_camera` version also remained 256
bytes / 19 relocations and diverged earlier. An unused duplicate accessor or
fabricated dependency would exist only to mimic January's extra assertion
sequences, so none is retained. The function is parked `unclassified` and
should reopen only when authoritative January source/local records or a
natural compatible-compiler donor explains that repetition.

## Prototype ownership

The pass also removes local cross-subsystem prototypes. Public Director
camera-load declarations are owned by `camera/director.h`; scripted-camera
enable/update declarations are owned by `camera/camera_scripting.h` with
forward declarations for their structure types. `camera_scripting.c` includes
its own header and the Director owner header, and `hs.c` includes
`camera/director.h` rather than redeclaring three Director functions locally.

The focused gates prove these ownership changes preserve:

- `camera_scripting.obj`: 13 exact / 0 residual / 0 unwritten;
- `hs.obj`: 445 exact / 3 pre-existing residual / 0 unwritten.

## Provenance and rejected shapes

Evidence, in descending authority:

- January target COFF, disassembly, relocations, and hardened comparison;
- `_research_cache/halocea/src/blam/camera/director_load_camera.c`;
- `_research_cache/halocea/src/blam/camera/director_script_camera.c`;
- the existing authenticated Director structures, enums, and camera helpers.

Rejected shapes include a direct `triple_product3d` call, the flattened donor
expression, a local triple-product temporary, a reversed comparison, and the
scripted-camera setter variant. They either produced a 320-byte loader or left
the x87 cleanup order/Scripted Camera residual different. No raw assembly,
`volatile`, `register`, forced inline/noinline, pragma, optimizer barrier,
manual bit pattern, raw field offset, address-derived name, invented global,
or nonsensical dependency is retained. The January inline schedule remains
intact, and this change emits no `point_from_line3d` COMDAT.

## Validation record

Focused production gates after the final source and owner-header edits:

- `python tools/campaign/gate.py source/camera/director --all`:
  24 exact / 1 residual / 3 unwritten;
- `python tools/campaign/gate.py source/camera/camera_scripting --all`:
  13 exact / 0 residual / 0 unwritten;
- `python tools/campaign/gate.py source/hs/hs --all`:
  445 exact / 3 residual / 0 unwritten.

Aggregate build, stable-verdict regression diff, parked-registry validation,
fake-match scan, tooling tests, and COFF symbol audit are recorded at canonical
integration time rather than claimed here in advance.
