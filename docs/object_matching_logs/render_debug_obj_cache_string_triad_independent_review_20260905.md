# `render_debug.obj` cache/string triad independent review (2026-09-05)

## Verdict

The frozen first-natural cache/string triad is credible for function-level
retention.  It advances the isolated object from **11 exact / 0 residual / 25
unwritten** to **13 exact / 1 residual / 22 unwritten** while preserving every
inherited runtime owner.  The two leaves are strict:

| Function | Candidate / January padded bytes | Relocations | Result |
| --- | ---: | ---: | --- |
| `render_debug_add_cache_string` | 208 / 208 | 17 / 17 | strict exact, normalized SHA-256 `aa27a6f4a33f3a6f97fa14d6eaab230ede15284dc76924a21a48bbe3164f9453` |
| `render_debug_string` | 112 / 112 | 7 / 7 | strict exact, normalized SHA-256 `6be81dbcfa4f615bf77467a8aa89f5c3bb0fff2c5546a8754d7a110027168be8` |
| `render_debug_add_cache_entry` | 672 / 656 | 27 / 27 | honest semantic residual; candidate/target normalized SHA-256 `253ff517307f1ce0930b283987a3440871ba949616ecc6ed43ff1ce939911398` / `758a747bdf7be941e3f5d2ed8af568da259bfd54f31c20040d5e020dd3a26039` |

No exact or whole-object credit is appropriate for the 672-byte helper.  Its
remaining difference is ordinary control-flow/register-save scheduling, not a
license for another source-shape search.  The candidate places the capacity
overflow return before the switch bodies and saves `EBX`/`EDI` after the
successful-capacity path; January places the overflow block at the tail and
saves those registers in its prologue.  Both contain the same ten cases and
default path, 27 relocation identities and balanced private cdecl ABI.

## Frozen evidence

- January split object: `build/split/source/render/render_debug.obj`, SHA-256
  `d3bbaa9484914bd9cf4695ef1775198a60c7891f466aff4fcefe87caf293fdd0`.
- Untouched isolated baseline:
  `scratch/render-debug-cache-baseline-20260905.obj`, SHA-256
  `09015e3097ac9b23724cb028065b5c09898548357f700c00f2bf93645689cd32`.
- First-natural candidate:
  `scratch/render-debug-cache-root-natural-20260905.obj`, SHA-256
  `f437cea5c3e21bc70080f2458c2343f3c7590fdd0b93dfec66125737dcf4a9c1`.
- Root-selected one-shot edit manifest:
  `scratch/render_debug_cache_triad_root_natural_20260905.edits.json`, SHA-256
  `11b69feb1feb8e0cc2994b5ae0e4b1dc57ad6504bd15c3f780a614daba823061`.
- Readable pre-gate proposal:
  `scratch/render_debug_cache_triad_natural_proposal_audit_support_20260905.inc`,
  SHA-256
  `88f5923187bb8aa7d53fdbd65d9a7b79994f5955c2d63be75bd2b30485ef34f6`.

The preserved Opus tree supplied reconstruction evidence, not original-source
authority and not aggregate-byte credit.  Earlier selective Render Debug
ledgers admitted the independent geometry leaves and left the object at
11E/0R/25U; they did not adjudicate this private caller/helper/data closure.
This review therefore does not reopen any previously parked Render Debug
residual or import the donor's broader translation unit.

## Schema, ABI and behavior

January's private `_render_debug_globals` is a file-static 0x740C-byte object.
A complete relocation-addend census proves:

- a 1024-byte string pool at `+0x0000`;
- 512 cache entries of 0x38 bytes at `+0x0400`;
- signed-short `game_time`, `entry_count`, and `string_offset` at `+0x7400`,
  `+0x7404`, and `+0x7408`;
- sticky overflow booleans at `+0x740A` and `+0x740B`.

January has no relocation to `+0x7402` or `+0x7406`.  The source therefore
keeps those two spans opaque rather than inventing field names or claiming
that they are proven compiler padding.  The tagged entry union is grounded by
the complete add-cache-entry switch and the independent Render Debug replay
switch.  The candidate retains the real two-byte type, four-byte-aligned union,
0x38 entry size, and all type-specific field widths.

The public `render_debug_string(boolean, const char *)` declaration was already
owned by `render_debug.h`; its six current source callers need no ABI or header
migration.  Both cache helpers remain genuinely private.  The owner includes
supply all external declarations; no caller-local public prototype or facade
is introduced.

The variadic boundary is source-correct for MSVC i386: the named `short` type
occupies a four-byte cdecl slot, promoted narrow arguments are read as `int`,
and promoted `real` values are read as `double`.  The candidate preserves the
following January behaviors:

- signed low-16-bit time comparison, including two `game_time_get` calls on a
  new tick;
- reset of count, pool offset, and `strings[0]` only, while warning flags stay
  sticky;
- signed 512-entry capacity test and one-time overflow reporting;
- all ten payload cases plus the default path, which counts an invalid type
  and leaves the old payload bytes untouched;
- suffix-by-byte string lookup, bounded copy, forced final NUL, clamped pool
  offset, and one-time pool-overflow reporting;
- a temporary cached-string result for types 8 and 9, so failure decrements
  the count without first overwriting the stale union pointer.

The last point corrects an old donor reconstruction mistake and follows the
January instruction stream; it is semantic recovery, not byte tuning.

The independent Unicorn differential
`scratch/render_debug_cache_behavior_20260905.py` (SHA-256
`6705d663d86f5185fcf9b7e71ce9e0549d9716bd3c86399a97bc2cecd8d28964`)
ran the frozen target and candidate through **41 cases / 81 invocations** with
zero failures.  It compares full cache bytes, ordered external observations,
input immutability, guard regions, stack balance, nonvolatile registers and
x87-stack balance.  Coverage includes immediate/deferred public strings, all
ten valid entry types, same/new tick and signed-wrap cases, counts 511/512,
interning and suffix reuse, truncation, failed insertion, and sticky warnings.
Result:
`scratch/render-debug-cache-root-natural-behavior-20260905.json`, SHA-256
`3303ea659e99dd9e7c28133181e31b86318704d48967563d4c07b72f4a7382c9`.

The equivalence claim is limited to the genuine private-caller topology with
non-aliasing geometry/color inputs and finite promoted values.  It does not
claim behavior for an invented caller that aliases an input into the cache
entry being written.  The default/invalid-type path was verified from both
instruction streams rather than exercised by the typed harness.

## Complete runtime-owner audit

The independent census
`scratch/render_debug_cache_root_natural_owner_audit_20260905.py` (SHA-256
`52983b3239d07bbd7f6b3071cbb46c4d5666eedeef355252e8155e0edb5b27e6`)
produced
`scratch/render_debug_cache_root_natural_owner_audit_20260905.json` (SHA-256
`bd786c38d286e0af21e02fb9ddf2b01e82ee9240d5f5cf455ffa043c74e1da4e`).
It compares every external/file-static owner in `.text`, `.data`, `.rdata`,
and `.bss`, including symbol value, storage class, type, section size, flags,
alignment, relocation count, COMDAT selection and normalized contents.

The baseline has 11 code and 12 non-code owners; the candidate has 14 and 16.
All **23 inherited runtime owners** remain present with identical symbol
properties and section fingerprints.  Inherited compiler-local label
properties are also unchanged.  The three new code owners are exactly the
requested functions: the two helpers are file-static and the public wrapper
is external.  There is no candidate-only code relative to January, no COMMON,
no extra helper, and no `_point_from_line3d` definition.

The four new non-code owners are exactly:

| Owner | Size | Ownership result |
| --- | ---: | --- |
| `"render debug cache overflow."` | 29 | same payload/properties and same-TU January owner |
| `"render debug cache string overflow"` | 35 | same payload/properties and same-TU January owner |
| `"string"` | 7 | ordinary selection-2/select-any literal; payload/properties match January's selected `source/hs/hs.obj` copy and current copies in Rasterizer Text, Draw String, Unicode, and Render Debug |
| `_render_debug_globals` | 29,708 | genuine file-static `.bss`, 16-byte alignment, zero relocations, selection 0, same size/flags/storage/type and normalized zero contents as January |

The ordinary candidate emits uninitialized `.bss` with no raw payload while
the split January object materializes zero bytes (`raw_present=false` versus
`true`).  That representation difference is recorded explicitly; it is not
misreported as byte-for-byte COFF metadata identity.  The only new pooled
literal has matching select-any selection and bytes.  A complete census of
**833 target objects and 572 current base objects** completed with zero parse
errors and found no conflicting selected owner.  This is owner evidence, not
a claim of a successful whole-program link or separate seven-byte data credit.

The candidate-only non-code list relative to the split Render Debug object
also contains five inherited pooled diagnostic literals (`point`, `color`,
`bounds`, `points`, and `vector`) because January selects those copies in
other objects.  They predate this packet and are unchanged from the isolated
baseline.  The only new undefined references are the genuine owner APIs
`csstrcmp`, `csstrlen`, `csstrncpy`, `error`, `game_time_get`,
`interface_set_bitmap_text_draw_mode`, and `rasterizer_draw_string`.

## Known boundary and recommendation

January's separate, still-unwritten `render_debug_circle` caller omits the
final promoted offset even though cache type 0 reads that double from its
variadic stack.  The harness confirms the resulting value depends on incoming
register spill state.  That original caller mismatch is neither introduced
nor repaired here; this triad must not be cited as approval to import the
circle body unchanged.

Recommendation: retain the two exact leaves and park
`render_debug_add_cache_entry` as a credible, measured function-level residual
after the normal canonical/full-build checks.  Do not claim the remaining
Render Debug object complete, do not award exact credit to the residual, and
do not retry its control-flow spelling without authoritative new source/local
evidence.

Independent semantic report:
`scratch/render_debug_cache_root_natural_semantic_review_audit_support_20260905.json`,
SHA-256
`5e01a9e6262400fd7d085490b57b0328f6a4fd9c78daa6c1ef599beb0382dcdd`.
