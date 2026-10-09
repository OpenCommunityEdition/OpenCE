# Opus

[Opus](https://opus-codec.org) 1.5.2 (`opus-1.5.2.tar.gz`, SHA-256
`65c1d2f78b9f2fb20082c38cbe47c951ad5839345876e46941612ee87f9a7ce1`), for
voice chat (`port/linux/game/network_voice.c`). BSD licence: `COPYING`.

Only the library is kept: `include/`, `src/`, `celt/` and `silk/` with
`silk/float/` (the floating-point build), without the multistream and
projection API, the demos, the tests and the SIMD and DNN code. `sources.txt`
lists the files the builds compile (`tools/linux_build.py`), as plain C with
`OPUS_BUILD`, `VAR_ARRAYS`, `HAVE_LRINTF` and `HAVE_LRINT`.
