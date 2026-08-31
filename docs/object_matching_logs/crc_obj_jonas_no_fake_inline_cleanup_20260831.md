# `crc.obj` source-credibility cleanup (2026-08-31)

`source/memory/crc.obj` remains fully strict exact after removing the inert
`__declspec(noinline)` attribute from its private table generator. The
attribute had no effect on the retained build and no original-source evidence,
so it was unnecessary compiler steering.

The same pass removes the remaining address placeholders:

- private `code_001088e0` becomes `crc_table_init`, the descriptive name used
  by the independently recovered cross-build CRC implementation; and
- global `bss_00456220` plus its alias macro become the directly named
  `crc_globals` owner.

`crc_table_init` is explicitly static in `config/symbols.json`. `crc_globals`
owns the exact packed 1025-byte table/initialized layout directly; no address
macro remains.

After a fresh target split and base rebuild, all three functions, all 208
padded code bytes, and the complete data footprint remain exact.
`_crc_table_init` is 64 bytes with zero relocations and normalized SHA-256
`a4b863bff24b1f220eaf4f2732125741dd27aacd866d358f0c77f51266e28c41`
on both sides.
