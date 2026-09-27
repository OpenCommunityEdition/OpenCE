# Independent source review: b659a777

Scope: read-only review of donor b659a777e4086d34ff3036b30bdf8bc6dcd92ce8
against canonical cdf1c42b, with donor current tip 798f5c2c. No source edits,
builds, or donor/canonical writes were made. Existing donor objects were read
as corroboration, not substituted for a fresh canonical build.

## Result

The delta is source-admissible subject to fresh strict and whole-board gates.
It adds no forbidden decoration, casts, compiler controls, hand-expanded
helper, ABI changes, shared-header changes, or owner/provider exception.
The helper parameter-reuse while loop, clip-pointer topology, and color helper
call have independent first-party machine witnesses. Their compiler effects
are legitimate consequences of these source shapes; the disclosed VN-ordinal
effect does not by itself turn an attested shape into filler.

`git diff cdf1c42b b659a777 -- source/interface/ui_widget.c` is exactly the
commit's three changes plus local names; no earlier donor source prerequisite
is hidden in this file. `modulate_pixel32_by_real_alpha` is already defined
earlier in the same TU. Its call inlines naturally under the existing flags.
The cumulative-alpha helper is TU-private, not a new shared-header COMDAT.

## Independently read raw PE evidence

Read only, through pefile and Capstone, without executing the supplied image:

`research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe`

SHA-256: `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
Image base: `0x400000`.

- Helper `0x668190`: one scalar local `[ebp-4]`; alpha load from widget+0x24
  at `0x66819e`; parent load from widget+0x30 at `0x6681ab`; parameter home
  overwritten at `0x6681ae`; loop test `0x6681b1`; multiply `0x6681bf`;
  next parent stored into the same parameter home at `0x6681cf`; back edge
  `0x6681d2 -> 0x6681b1`. Call from render_recursive at `0x66a06d`.
- Clip: test at `0x66a240`, eight-byte copy at `0x66a246..251`, address of
  local clip stored back to clip pointer at `0x66a254..257`, then stores
  through that pointer at `0x66a26a` (+2 = x0), `0x66a27e` (+6 = x1),
  `0x66a291` (+0 = y0), and `0x66a2a4` (+4 = y1). This matches the existing
  rectangle2d prefix and field layout, not an imported cross-build layout.
- Color: alpha argument built at `0x66a33d..343`, `push -1` at `0x66a348`,
  call at `0x66a34a -> 0x40a6dc -> 0x6627d0`. Result stored in a scalar local
  at `0x66a352`, before parameter-block initialization, and read at
  `0x66a37e` for the draw call. The name `color` is descriptive, not proven.
- Color helper `0x6627d0`: low 24-bit mask at `0x6627ef`, alpha-byte shift
  at `0x6627f8`, multiply at `0x66281c`, conversion call at `0x662827`,
  shift at `0x66282f`, OR at `0x662832`. The relevant shape is the existing
  source helper; later SSE instruction selection is not imported.
- RTC descriptor `0x66a65c` has five entries at `0x66a664`:
  `bounds` (-52, 8), `local_clip` (-72, 8), `multitexture_params` (-240, 140),
  `widgets_were_deleted` (-257, 1), `null_event` (-276, 8). Only the first
  three names relate to retained January locals; later-only logic was not
  imported.

## January corroboration and current donor object check

Using the existing hardened tools/coff_compare.py on existing donor split/base
objects (Python -B, no build), section_infos_equal was true for:

| Function | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| _widget_instance_render_recursive | 752 | 27 | 3be047db499880358c7494730c048a9109b4798bce707a6dbece4967b60bfabe |
| _widget_instance_get_cumulative_alpha_modifier | 32 | 0 | 31a73877c297d4577ef2d5f94199ec4201c6ea11aa1fa84c7c5b3786d6b105b6 |
| _modulate_pixel32_by_real_alpha | 64 | 1 | 2b826837864844e7029d6b020508d1f08ef53876fcef2e2c127abfb381ed59c5 |

January render_recursive has the inline alpha chain at +0x1c..3f; the
game-data-input indexed load is `66 8b 14 01` at +0x78. Clip copy/update and
local-address storage occur at +0x143..161. The color calculation has the
255.0 constant relocation at +0x1a3, `fstp`/`fld` at +0x1a7/+0x1aa,
`fistp` at +0x1ad, shift at +0x1b8, OR 0xFFFFFF at +0x1c2, and the draw
call relocation at +0x1d7. No call relocation to either inline helper occurs
in this January caller. Return at +0x2e6 gives 743 meaningful bytes, followed
by nine alignment NOPs; do not report 752 as meaningful credit.

Fresh canonical build/sibling/section/provider checks remain the main
reviewer's responsibility. Donor saved logs claim only this owner changes,
329 sections before/after, no added/removed sections, no warnings or exact
losses; those saved receipts are not fresh canonical receipts.

## Qualifications and inherited concern

- `0xFFFFFFFF` represents opaque white for the existing unsigned-long
  pixel32 type. Raw /Od proves the value, not the original literal spelling.
  This is a disclosed spelling uncertainty, not a semantic concern. Do not
  replace it with `NONE`: a pixel bit pattern is not an absent-index sentinel.
- The parameter-reuse helper's change to VN ordinal mod 4 is disclosed in
  RF-L L-1/L-1V and DECODE. Diagnostic forced-key builds are not production
  candidates. The production source is ordinary C and the final stock build
  must supply strict proof.
- No current named hold was found for this particular source shape. TRIAGE's
  older decorative-parenthesis render_recursive candidate is not this delta;
  it does not authorize a parenthesis edit elsewhere.
- A historical note in ui_widget_obj_opus5_150k_w3_20260914.md:124 recommends
  replacing the inherited never-read `sequence` local with a bare
  TAG_BLOCK_GET_ELEMENT statement on the next landing. This commit does not
  add that local. Later /Od independently does have the sequence pointer
  store at 0x66a189 and reads at 0x66a18c/193/19b/1aa, but its surrounding
  logic differs from January (including sequences.count > 0 guard). Thus
  its precise January declaration cannot be proved from /Od alone. Treat
  this as an inherited source-cleanup/provenance note, not hidden dependency
  or newly introduced fake source; do not silently remove it without a
  source review and a fresh full-TU comparison.
- The raw later function additionally contains parent locals, a parameter
  block initializer, and later-only logic not present in January. Their
  existence grants no permission to import them. The reviewed delta stays
  narrow.

## Minimal preservation packet

Preserve the actual source delta; this review; RF-L REPORT.md, EVIDENCE.txt,
DECODE.txt, TRACES.txt, cards/L-1.txt, L-1V.txt, L-S.txt, and the L-D1/L-D2/
L-D2b diagnostic cards (including failed prediction); RF-L gate/keyed/W3
logs; RF-E WAVE2_EVIDENCE.md and cards2/RR-1.txt/RR-2.txt; the lead
od_66a020_full.txt readout; R2_render_recursive/summary.json and stable_diff.log;
and OWNER_PACKET.md's B3 disclosure. Preserve donor research/scratch in place,
but private executables, SDK/compiler binaries, and raw trace assets do not
belong in a routine public source publication.
