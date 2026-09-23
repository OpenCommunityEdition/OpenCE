# source/linker_common: what a comparison base can and cannot earn

*2026-09-22, Lane C. Built at the owner's direction, from January's verified
COMMON records only, and scored separately from the board.*

## What the unit is

`source/linker_common` is not a translation unit. It is csplit's name for the
pooled COMMON block of January's image: 242 records totalling 1,272,664 bytes,
gathered by the linker from tentative definitions across the whole program
(`_render` 643,732, `_ai_debug` 547,628, `_dsound_globals` 30,924, and ~200
smaller ones, down to single-byte debug toggles). `objdiff.json` gives it a
`target_path` and **no `base_path`**, because our build produces no such object -
our COMMON symbols live in their owning objects' symbol tables with section 0.
So it reports 0.000% structurally, not because anything is wrong.

## The base, and its score

`tools/campaign/linker_common_base.py` builds a comparison base containing ONLY
verified records: a symbol January pools that our tree independently emits as a
COMMON symbol of the identical size. Nothing is invented, and nothing
initialised is copied - a `.bss` record carries no bytes, only a name and a
size, and the size used is the one our own objects request. Records we do not
emit, emit at a different size, or emit in a real section are dropped.

    kept     58 records   1,214,903 bytes
    dropped 184 records      56,787 bytes

Scored on its own (`objdiff-cli report generate` over a one-unit config):

    .bss     1,272,568    95.541%
    .rdata          96     0.000%
    credited                     0

**It earns nothing.** Data is credited per section only at 100%, so 95.541% is
worth exactly as much as 0%. The number's value is diagnostic: it says 95.5% of
the pooled block is already correct in our tree, and names precisely what is not.

## What the remaining 4.5% is

181 records, 56,187 bytes, absent from our tree entirely. Only 27 of them
(1,125 bytes) are CRT/XDK/dsound runtime symbols. The rest are genuine Halo
globals no TU of ours declares: `_dsound_globals` 30,924, `_rasterizer_lights`
7,172, `_sound_channels` 6,144, `_wind_globals` 3,340, `_error_globals` 2,056,
`_global_window_parameters` 600, and so on. Plus two pooled `.rdata` records
(96 bytes) that are not COMMON at all and so are outside this base by
construction, and one size mismatch, `_file_location_volume_names`, which is the
out-of-bounds array bug recorded separately.

Closing the section therefore needs those 181 globals declared as tentative
definitions in the right units - spread across many TUs, several of which are
not reconstructed - and it pays nothing until every one of them is present.

## Rules this establishes

* A symbol in `linker_common` was a TENTATIVE DEFINITION in January: declared
  with no initialiser. **Never add `= {0}` to such a global** - that moves it out
  of the pool. Check `linker_common` before touching any global's initialiser.
  This is how the `actors.c` `= NULL` defect was found and fixed.
* Conversely, a global January keeps in its OWN object's `.bss` (director,
  render_objects) was initialised in source, and must keep its initialiser.
* Do not wire this base into `objdiff.json`. It would fold a structurally
  uncreditable section into the board number without closing anything.
