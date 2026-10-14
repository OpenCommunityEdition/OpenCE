# stb_vorbis

An Ogg Vorbis decoder in one C file, by Sean Barrett and its other authors,
public domain or MIT licensed, as the user chooses (see `LICENSE`, the
notice at the end of `stb_vorbis.c`).

Upstream: https://github.com/nothings/stb, commit
2c980bb59875b0d32144a71867fbdebb2f77cd20 (`stb_vorbis.c` v1.22, SHA-256
`4c7cb2ff1f7011e9d67950446b7eb9ca044f2e464d76bfbb0b84dd2e23e65636`).

Unlike the other libraries here, `stb_vorbis.c` is **not** unchanged: a Custom
Edition map is untrusted input, and the decoder reads the map's streams, so
it carries checks upstream does not have, each marked with a `port:`
comment. As changed, its SHA-256 is
`16ceceac06245040d7ec5965d150a08aa53d663a4cecbe3302f447a12a03b6a3`.

- `alloca.h` included on macOS too.
- `submap_floor` and `submap_residue` hold 16 entries, as many as the
  mapping's 4-bit submap count (plus one) allows (upstream has 15).
- `setup_malloc` and `setup_temp_malloc` refuse a size whose rounding to 8
  would overflow.
- `lookup1_values` refuses a code book of no dimensions or entries (it
  divided by the dimensions), and compares the powers as floats rather than
  casting values past an int's range to int.
- `DECODE` looks up a sparse code book's value only for a code that was
  found (the scalar decode gives -1 when none is).
- The comment header's vendor string and comment lengths, and its comment
  count, are no larger than the stream holds; the comment list is zeroed
  when allocated, and its length 0 when it is not.
- A code book's entries times its dimensions is at most 2^24.
- `do_floor` masks the table index of the flat run after a floor's last
  point to 255, as `draw_line` masks its own (upstream reads past
  `inverse_db_table` for a floor value over 255).

`LICENSE` is a copy of that notice. The Custom Edition maps' Ogg Vorbis
sounds are decoded with it when a map loads
(`port/linux/game/custom_edition_sounds.c`, which includes it for its
declarations); it is compiled as the port's game units are and builds into
the Linux, Windows, Android and web builds.
