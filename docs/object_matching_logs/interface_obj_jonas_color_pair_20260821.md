# `interface.obj` Jonas color pair (2026-08-21)

## Result and fixed boundary

This Jonas-owned source-only wave starts from exact canonical commit
`5c8b8b60aad377f230996bc4b6cb1440dfb23710` on isolated branch
`jonas/interface-color-triad-20260821`. The fixed first-shot boundary contained
exactly three ordinary-C bodies. Two were independently strict in the first
and only natural three-body code-producing candidate compile and are retained:

| Retained body | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_interface_set_bitmap_text_draw_mode` | `0x000CE2D0` | `65 / 80` | 3 | `35cd0180e365935de07279b208bbe58342000438ba989bc3b7bcf09051c7cc89` |
| `_interface_get_rgb_color` | `0x000CF430` | `108 / 112` | 9 | `22481184ff254bd468dba15e1b2a35a7414569b9b8403c677a556e2e3c3bbb56` |

`interface.obj` advances from 3/18 to 5/18 exact functions and from
55/5,966 to 228/5,966 meaningful code bytes. It remains `NonMatching`. The
strict net gain is 173 meaningful bytes, 192 padded bytes, and 12 exact
relocations. No data credit is claimed.

The third fixed body, `_interface_get_real_argb_color` at January RVA
`0x000CE250`, was a bounded first-shot miss and is absent from the retained
source. Its target is 118 meaningful / 128 padded bytes with three relocations
and normalized SHA-256
`aea422f7902d51c4d635f9249897a37b66fc4a5285d111976a017ef4864ec9a9`.
The natural candidate had the same padded size and relocation count but
normalized SHA-256
`5a4a9c140c3486c5ed710ea4211265e6f5d4319e40db49df6f3996b6a5fab4a1`:
the first two relocations were exact at `+0x0A` and `+0x30`, while the typed
tag-block helper call landed at `+0x4B` instead of January's `+0x4E`. The body
and all support used only by it were removed wholesale without a tune, retry,
or alternate code-producing compile. Its ordinary prototype remains because
both retained bodies call the target-owned external function.

## Immutable target, source, and compiler provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/interface/interface.obj`: 118,524 bytes, SHA-256
  `83f7953f8143a7f935c9ecceaeb2a8690c710b5665cf2a900040d901cb12f5f2`.
- Exact-base `source/interface/interface.c` Git blob
  `f2619fba1ae02e2af9d533e564d414399137879b`, 3,041 payload bytes, with
  Git-blob payload SHA-256
  `70339bdcb2f3973678bf1f824cb28ecd73adad0d4e3ff425eaa592068686aad2`.
- Retained translation-unit Git blob
  `b8ec0f7ad7c7e69cb6f99dfdd1a3e2c7fdbfd69a`, 4,357 payload bytes, with
  SHA-256 of the exact staged `git cat-file blob` payload
  `1f053ff8a44e900ec53654e68d36fe0b72e458647e78418b841e32fb389864d1`.
  These are raw Git-blob bytes, not the CRLF working-tree view.
- The shared `source/interface/interface.h` remains unchanged at blob
  `532b43c20142ab79919991f4bf6ab5e833b1fe0c`, 907 payload bytes, payload
  SHA-256
  `73501648dfed25b53a6cbdb325d0f2bdbb89403916c70a870d66f18742417fbe`.
- The retained production object is 2,666 bytes with SHA-256
  `b926a4ff0c21d594e0da704619749b796cbdb2899d29885e03e980890fcf31ec`.
  The immutable first-shot three-body object was 2,969 bytes with SHA-256
  `4db6da5b69af8db1f464adb8b30b3b60b4003f3bf8680e45325fb9ded3ccf564`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural repository flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  with the unchanged XDK 3911 include graph.

The clean local HCEA reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every cited file is directly
tree-reachable from that commit. HCEA supplies readable intent and layout
corroboration; January PC COFF alone governs byte acceptance.

| Clean c168 HCEA evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| real-color body | `fd0acb9cf7e73e4c800a0dce0f786e692a4211a4` | 1,563 | `68dfdc8b3d72ec596fb1318f508d017f78bbb9d5b8c17008432f99528826a98d` |
| draw-mode body | `dfb51df30dda6aef8960a79a3b10ed4224c39b36` | 1,482 | `fbaa3b06a009f624f2d34b59b0548b450a2c2a463069755efbf969cb3ac93458` |
| packed-color body | `e1086aaf9a20a21f0df42e29d9632db58e1c16a2` | 1,279 | `03c377af20a723adcae167cd3c85481487ee1fbcdf7b66e33efb3cb93d8511e6` |
| color-table entry layout | `09d5c0876a089e606e060b89139c22e0b067770b` | 329 | `787f1c9fefa6034e3578faee50f6142e129f0876f5e31fe7a839f6d558cbc269` |
| real ARGB layout | `54b1c2d857915617af210a05465416585f9e3715` | 1,261 | `9174b4c6d649628d629c6f4aa524ef1f80aa6a84ffa8026ca6ac6c90c713d038` |
| packed ARGB layout | `fc9d06cb496d93bceed4c0c4fed1c5e1d17d33ea` | 1,106 | `ec2f3159c8557876f419dbf59416f88d94a3524f458243652c55d143728b6f75` |

Current typed dependencies were also pinned before emission: `real_math.h`
blob `f14567675126171a4b1c2d3052e8058c68c029c2`, whose 43,695-byte raw
payload SHA-256 is
`c5b4659b368565ff7db2f51536b631694d5db9df01631be8cc1385934e51cf01`;
and `draw_string.h` blob `37a7635ed7653d416df99964752a54d5d2072c2d`,
whose 1,134-byte payload SHA-256 is
`310a8967ddfb6689e6dd838b252c009bd268ee9cd05ba9e119886ba5d2669e18`.

Canonical history has no earlier committed definition of either retained
body. `git log --all -S` reaches only the initial symbol-inventory comment,
not a body or an earlier matching ledger. Repository-wide and live-worktree
collision checks found no competing `interface.c` implementation. The January
target defines each screened external symbol once.

## Typed ABI, callers, and layout

The retained ordinary-C signatures are source-local:

```c
void interface_set_bitmap_text_draw_mode(
	short interface_font_index,
	short style,
	short justification,
	unsigned long flags,
	short color_table_index,
	short color_index);

argb_color *interface_get_rgb_color(
	short interface_color_table_index,
	short color_index,
	argb_color *color);
```

Both January bodies consume their complete argument lists through normal
stack slots and return with plain `ret`. Reconstructed identifiers naturally
emit the leading-underscore COFF names. All observed calls are `REL32`, with
ordinary caller cleanup, so neither body crosses a private or register ABI.

January direct callers of `_interface_set_bitmap_text_draw_mode` are
`_code_00041e70` in `ai_profile.obj` (`+0x3D`), `_code_001b5860` in
`bink_playback.obj` (`+0x283`), `_collision_log_render` (`+0x5B7`),
`_rasterizer_frame_statistics_draw` (`+0x228`), and five bodies in
`render_debug.obj`: `_render_debug_string` (`+0x3F`),
`_render_debug_string_at_point` (`+0xF0`), `_code_00179630` (`+0x1FF`),
`_code_001799e0` (`+0x14B`), and `_code_00179bc0` (`+0x4D`). The split-COFF
census found no direct or address-taken caller of `_interface_get_rgb_color`.
That public leaf is self-typed locally and therefore requires no shared-header
change.

The final private `argb_color` is a union containing `word n[4]`, with a
compile-time size proof of eight bytes. Current `word` is an unsigned 16-bit
type. Current `real_argb_color` is a 16-byte union of four `float` channels in
alpha/red/green/blue order at offsets `+0/+4/+8/+C`. The retained packed-color
body retrieves that typed aggregate through the authenticated ordinary-cdecl
prototype and converts each channel independently to `(word)(channel *
65535.0f)`. There is no raw overlay, pointer pun, inactive-union read, or
layout guess.

The draw-mode body resolves the font with the existing declared
`interface_get_tag_index(short)`, retrieves a typed local `real_argb_color`,
and passes the exact existing
`draw_string_set_draw_mode(long, short, short, unsigned long,
real_argb_color const *)` contract. January governs this PC helper topology;
HCEA only corroborates the readable operation.

## Exact relocation and semantic proof

The retained draw-mode relocation schedule is exactly `+0x0C` to
`_interface_get_tag_index`, `+0x1F` to
`_interface_get_real_argb_color`, and `+0x35` to
`_draw_string_set_draw_mode`.

The retained packed-color schedule is `+0x14` to the real-color helper,
followed by four exact pairs: absolute `.rdata` references to
`__real@477fff00` at `+0x1D,+0x2E,+0x42,+0x54` and `REL32` calls to
`__ftol2` at `+0x25,+0x39,+0x4B,+0x5D`. The hardened comparator proves
identical meaningful and padded code, normalized hashes, and every
relocation's address, type, destination, addend, and order. Nothing is
accepted from an objdiff similarity score alone.

All three inherited accepted lifecycle bodies remain strict:

| Inherited body | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_interface_initialize` | `20 / 32` | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |
| `_interface_dispose_from_old_map` | `15 / 16` | 3 | `b9032760bc3fde17757be12527d259547b8029d0b1dca531056c51c1d603a143` |
| `_interface_dispose` | `20 / 32` | 4 | `39689e05a9fbdf90c5204e8fec70f236d6065e18a101691cd2009010245d8032` |

Direct hardened comparison proves the complete five-body final inventory.

## One-shot, ownership, and explicit exclusions

The private types, includes, prototype, and all three complete fixed bodies
were frozen before code emission. VC7 `/Zs` parse-only validation passed and
the candidate object path was verified absent. Exactly one natural
three-body object compile followed. Draw-mode and packed-color were strict in
that immutable object. Real-color missed as recorded above and was removed
wholesale; no alternate expression, declaration, cast, field view, lifetime,
scheduling, constant, topology, qualifier, compiler switch, or second
candidate object was tried. The subsequent full production build merely
reproved the unchanged retained pair after removing the miss.

The final object defines exactly five external code COMDATs: the three
inherited lifecycle bodies and the two retained bodies. It owns no writable
`.data`, `.bss`, or COMMON symbol. Its only runtime non-code owner is the
compiler-owned four-byte `__real@477fff00` `.rdata` COMDAT required by the
packed-color body; that literal receives zero standalone data credit. The
new undefined runtime dependencies are exactly
`_interface_get_real_argb_color`, `_interface_get_tag_index`,
`_draw_string_set_draw_mode`, `__fltused`, and `__ftol2`, in addition to the
inherited lifecycle callees.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, cast overlay, pointer or union pun, inactive-union access,
undefined signed overflow, data-dependent representation trick,
private/register ABI, volatile scheduling device, optimizer pragma or
barrier, synthetic anchor, or byte-forcing expression.

`_interface_get_tag_index` and every other target body are explicitly outside
the fixed wave. In particular, January's zero-count path for
`_interface_get_tag_index` performs a null-derived interface-table dereference,
so it is rejected as raw/undefined behavior. `_interface_initialize_for_new_map`
has an analogous null-derived tag-reference path. The fullscreen-overlay body
depends on private helpers and donor-divergent call order. None is defined,
declared as support, or credited here.

## Pre-commit validation

Only `source/interface/interface.c` and this new Jonas-owned ledger are
tracked changes. No shared header, configuration, semantic exception, parked
record, completion label, storage owner, pre-existing Markdown, or tracked
path deletion is changed. The five protected sources remain untouched:
`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass, 569/569 Ninja
  edges. The only diagnostic is the inherited `ai_debug.c` warning.
- Direct hardened final comparison: all five emitted functions strict,
  including exact padded bytes and relocation semantics.
- Semantic audit: 470 units, 4,153 functions evaluated, 3,576 missing-base
  symbols, 4,013 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 local symbols
  skipped, 4,074 accepted exact, and zero unit errors.
- Semantic campaign progress: 375/833 complete objects, 4,061/11,060 exact
  functions, 482,746/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo semantic progress: 273/468 objects, 3,894/7,574 functions,
  469,832/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `interface.obj`: 5/18 functions and 228/5,966 meaningful code bytes; it
  remains nonmatching and claims no standalone data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, changed-path, source-policy, protected-five, storage,
  and deleted-path checks are clean.

The clean committed-state replay is intentionally recorded only after the
implementation-and-ledger commit exists. No push, amend, or history rewrite
is performed.

## Clean committed-state replay

Implementation-and-ledger commit
`4a9c28db0fe843f67f2d58650925f7f9e142dd75` was clean before replay. Its
committed translation-unit blob is exactly
`b8ec0f7ad7c7e69cb6f99dfdd1a3e2c7fdbfd69a`; the shared header remains
baseline blob `532b43c20142ab79919991f4bf6ab5e833b1fe0c`. `git status
--short --branch` reported only the branch header and no changed or untracked
path.

A one-unit regression snapshot was captured from that exact clean commit at
`build/regression_interface_color_pair_20260821.json`, with status
`SNAPSHOT_WRITTEN` and sole unit `source/interface/interface`. The generated
candidate path resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\interface-color-triad-20260821\build\base\source\interface\interface.obj`.
It was explicitly verified to begin inside the resolved isolated-worktree
root. Before deletion it was 2,666 bytes with SHA-256
`b926a4ff0c21d594e0da704619749b796cbdb2899d29885e03e980890fcf31ec`.
That single file was deleted and a second existence check proved it absent.

The untouched production Ninja graph then rebuilt the same path through
exactly one `[1/1] CL` action, using the unchanged natural `/O2 /Oy- /DDEBUG
/Dxbox` command and include graph. There was no source change, body retry, or
alternate build edge. The immediate no-build regression check returned
`ok: true`, no failures, no warnings, `changed_nonexact: []`, and
`newly_exact: []`. Its `still_exact` set is exactly
`_interface_initialize`, `_interface_dispose_from_old_map`,
`_interface_dispose`, `_interface_set_bitmap_text_draw_mode`, and
`_interface_get_rgb_color`. A subsequent dry build reported `ninja: no work
to do.`

Direct hardened January comparison after the rebuild returned
`all_equal: true` for the complete five-body inventory. It reproduced all 173
newly retained meaningful bytes, all 192 padded bytes, all 12 relocations, and
each recorded normalized SHA-256. The rebuilt object is 2,666 bytes with
phase-specific whole-file SHA-256
`f864c078cb5e8f23f13ab9d28979c56d2ea60e7806ba4a4d70b74974c6460948`.

An independent post-rebuild COFF ownership dump again found exactly five
defined external code COMDATs and the sole four-byte compiler-owned float
literal. It found no `.data`, `.bss`, or COMMON owner. The retained undefined
runtime dependencies remain exactly as documented above. The final tracked
change after replay is this additive ledger section only.

Nothing is pushed, amended, or history-rewritten.
