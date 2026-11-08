# CELT

[CELT](https://downloads.xiph.org/releases/celt/) 0.11.0
(`celt-0.11.0.tar.gz`, SHA-256
`c94d4d34f5a2caa1574b1a94869202cacd959b55f643a8bafe0660008acad9c3`), for
the MCC import (`port/linux/game/mcc_import_sounds.c`): Halo: The Master
Chief Collection keeps Halo 1's dialogue and music in FMOD banks encoded
with this version of CELT, which the import decodes once to write them as
the Xbox's ADPCM. BSD licence: `COPYING`.

Only the library is kept: `libcelt/`'s sources and headers, copied
unchanged, without its build files, tests and `dump_modes.c`.
`sources.txt` lists the files the builds compile (`tools/linux_build.py`),
as plain C with `FLOATING_POINT`, `CUSTOM_MODES`, `CELT_BUILD` and
`VAR_ARRAYS`.

`celt_prefixed.h` is the port's: it gives every global name of the library
the prefix `mcc_celt_`, as Opus (`port/third_party/opus`) has CELT's later
code under many of the same names. The builds include it before libcelt's
sources, and the import includes it before `celt.h`.

The desktop builds (Linux, Windows) compile it; Android and the web, which
import no MCC data, do not.
