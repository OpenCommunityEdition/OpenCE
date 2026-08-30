# `rasterizer_cinematics.obj` closure census (2026-08-30)

## Outcome

The object remains at 12/16 strict-exact functions and 1,008/1,568 padded
code bytes. A single fresh snapshot-lifetime family for the convolution and
filter setters was compiled once, rejected, and inverse-restored. No
production source survives this wave.

The retained gate is again:

```text
UNWRITTEN   112  _rasterizer_screen_effect_set_convolution
UNWRITTEN   112  _rasterizer_screen_effect_set_filter
UNWRITTEN   288  _rasterizer_screen_effect_set_video
UNWRITTEN    48  _rasterizer_screen_effects_initialize_for_new_map
== exact 12  residual 0  unwritten 4  (of 16 listed)
```

Stable restored identities are:

- source Git blob: `333ba044758c895247fad6cf53e628186bedd341`;
- rebuilt object SHA-256: `5410AD73AC514A5EA5F7C8F9AAB5C36A862384BD250B76B31C190B901447457F`;
- January split-object SHA-256: `257B38976236EB2FB5913F4BDA88186C6B4DE1D498C5677EDDDED99B5B7E6400`.

All seven prior object ledgers were read before the experiment. The complete
authenticated HCEA setter and map-reset donors, January disassembly, current
typed layouts and callers, and all registered Claude/Fable source and artifact
trees were screened read-only.

## Census

The registered-tree census covered 1,577 source copies and 2,332
target-named objects with zero parse failures. Thirteen physical source
variants reduce to integration stages, rejected/reverted experiments, and the
current source. No compiled strict residual owner exists outside the January
split object.

The only surviving non-target residual artifact is the earlier direct-global
video shot:

| Function | Target | Candidate |
| --- | --- | --- |
| `_rasterizer_screen_effect_set_video` | 288 bytes, 20 relocations, SHA `eb1b757f0f5e5dbfb23a66ea08ec4cd3b8eeb4ce6385ea9c8a0f3e4e5916c46a` | 368 bytes, 33 relocations, SHA `ec301ad3671ddc26b138e7f4e6ca54a6eb63d1be1cc70f171c2c406ed1e8368f` |

It differs in 238 normalized byte positions and does not preserve either the
relocation address or destination sequence. It is not a donor.

## Fresh setter result

January reloads `_bss_004662f4` after `game_time_get`, a lifetime pattern
already proven exact in `_rasterizer_screen_effect_start`. One natural typed
candidate kept an entry snapshot for the parameter/range stores and refreshed
the typed state pointer before the time and trailing-flag stores.

| Function | Target | Fresh candidate | Result |
| --- | --- | --- | --- |
| `_rasterizer_screen_effect_set_convolution` | 112 bytes, 4 relocations, SHA `7beef58f5328b7768b506801a5528055389723414ad6c8ea1e4eead421e8aa48` | 96 bytes, 4 relocations, SHA `c25517ce64035012df733eece7556cb8b74d8ad225b391f65162d5537daee29c` | rejected |
| `_rasterizer_screen_effect_set_filter` | 112 bytes, 4 relocations, SHA `de4709a02eba43f11818fd1edf0124d200de56a81157be6670dfa8fff0834794` | 112 bytes, 4 relocations, SHA `f80be79aaebdf3fea70af61eb4d84983df5b74cacedcb7173436fea0ff383609` | rejected |

Convolution now has the exact relocation count, destinations, and addresses,
but VC7 still orders the radius stores before the video-reset stores and uses
`fst` instead of January's `fld st(0)` / `fstp`, leaving a 96-byte section.
Filter has the target extent and destination sequence; its remaining 65-byte
distance is store scheduling. January zeros the video fields before the four
filter-bound stores and writes the three mask/additive bytes after both time
stores. The candidate follows the donor statement groups instead.

The complete candidate was removed without a second spelling. The rebuilt
baseline again exposes only the twelve protected exact functions.

## Remaining fixed points and Claude-ready handoff

- New-map reset already has an exact-size/exact-relocation candidate whose
  only difference is stack cleanup between the third and fourth `1.0f` store.
  Separate assignments and the natural chained assignment canonicalize to the
  same nonexact schedule. Do not retry them without original statement/scope
  evidence.
- Video's natural HCEA local-snapshot adaptation is 272 bytes with 16
  relocations; the direct-global family is 368/33. January is 288/20 and also
  contains a null-global assertion path, a missing-bitmap error path, and two
  tag/block lookups. Reopen only with original January source or authenticated
  source for those error and pointer-lifetime paths.
- For convolution/filter, original statement order around the video resets,
  range stores, floating duplicate, and trailing flags is now the useful
  missing evidence. Do not declaration-order tune the rejected spelling.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, raw
address/offset access, punning, undefined behavior, byte forcing, object
patching, compiler flag, comparator exception, or storage rehoming was used.
