# Xbox link probe (2026-09-24)

The comparison build deliberately compiles each translation unit separately.
`tools/link_probe.py` makes an **opt-in** Xbox PE link attempt using those same
compiled objects. It does not change C sources, compiler flags, object files,
`objdiff.json`, the default Ninja target, or any byte-matching credit. It never
uses `/FORCE` or substitutes dummy functions. A successful PE link would still
need conversion to an XBE and game assets before an xemu boot test.

Run from a checkout whose `build/base` objects are current:

```powershell
python tools/link_probe.py --object-root . --output-dir build/link-probe
```

An SDK library directory and specific import/vendor libraries may be supplied
with repeatable `--library-dir` and `--library` arguments. `--exclude-project`
and `--exclude-source` are only for measuring an alternative link composition;
`--replace-object` substitutes a separately compiled source candidate and
`--extra-object` includes a non-manifest input. None changes the comparison
manifest or matching status. Do not commit or distribute SDK files extracted
for a local probe.

The script makes two attempts. `full` honors object default-library directives
and is the authoritative link outcome. `diagnostic` adds `/NODEFAULTLIB` to
expose otherwise-hidden unresolved symbols. Both use `/SUBSYSTEM:XBOX`, the
January `_mainCRTStartup` entry, and a response file. `summary.json` records
the linker result, diagnostic counts, and before/after SHA-256 of every input
object. A nonzero script exit means the full PE link did not succeed.

## Current measurements

At canonical `f6d00a8c`, all 621 manifest-backed non-missing objects exist and
were passed to the linker. Their combined before/after digest matched.

| Inputs | Libraries | Full-link result |
| --- | --- | --- |
| All 621 rebuilt objects | None | `LIBC.lib` unavailable; nine duplicate definitions. |
| All 621 rebuilt objects | Local XDK 3911 library directory | 403 unresolved symbols; 33 duplicate definitions. |
| 475 objects (Halo and reconstructed Bink; excluding reconstructed `libcmt`, `d3d8`, `xapilib`) | Local XDK 3911 library directory plus selected Xbox SDK libraries | 156 unresolved symbols; nine duplicate definitions. |

An explicitly **hybrid**, non-publishable experiment selected 473 objects,
substituted scratch-compiled versions of two recorded-animation TUs, supplied
January's original data-only `linker_common.obj`, and used local later XDK/Bink
libraries. It reached **four unique unresolved symbols**
(`DirectSoundGetStreamVoiceStatus`, `DirectSoundStopStream`, `fast_ftol_C`,
`main_crash`) and **four unique duplicate definitions**. It did not link.
The original data object and later libraries are proof-of-concept inputs, not
reconstructed source or matching credit.

Adding January's original `dsstrmh/main.obj` (a vendor object containing the
two absent DirectSound functions) reduced that experiment to **two unresolved
symbols** (`fast_ftol_C`, `main_crash`) and the same four unique duplicate
definitions. These two Halo functions are already subject to separate
house-rule holds; they must not be silently replaced by dummy source or
counted as matched. The four providers require source/ownership work, not
`/FORCE:MULTIPLE`.

A further genuine source repair removed `items.c`'s hand-written
`object_get_type` copy and used the existing `objects.h` inline. Its 18/18
functions stayed exact, leaving **three** unique provider conflicts:
`cross_product2d`, `real_local_random`, and `plane2d_from_points`. The latter
two do not have an evidence-backed zero-regression source fix yet; making
them private merely to silence the linker would be unauthentic.

The recorded-animation change itself is authentic and safe: January marks the
five duplicated `_apply_*` functions static in both TUs. Making their
prototypes/definitions static eliminated those five duplicate providers; both
scratch-compiled objects retained every exact function (14/14 and 13/13).

The no-default-library diagnostic on all 621 objects reports 478 unique
unresolved externals and the same nine duplicate definitions. The nine include
`_object_get_type`, `_real_local_random`, `_plane2d_from_points`, five
`recorded_animation_playback` helpers, and `_cross_product2d`. This is a real
link audit, **not** a bootable game or a byte-match regression. XDK 3911 is
newer than the January target, so its libraries are a diagnostic candidate,
not proof of an authentic final link.

Next: recover/declare genuinely missing Halo COMMON globals, resolve duplicate
providers through correct ownership and link composition, and locate compatible
Bink/Xbox libraries. Only when the PE link succeeds should `imagebld` convert
it to an XBE, followed by an XISO boot test. Neither the proprietary SDK
libraries nor the original game assets belong in this repository.

The locally extracted XDK 3911 `imagebld.exe` reports `/IN:<Xbox PE>` and
`/OUT:<XBE>`; it has not yet been tried on a rebuilt image. Its newer SDK
version remains a compatibility caveat.
