# render_sprite.obj exact-match log

## Scope and provenance

- Atomic lane: `agent/render-sprite-atomic-20260813`; experiments began at
  campaign commit `3328df18`, and this park record was transplanted onto the
  current authoritative campaign tip before handoff.
- January target: `build/split/source/render/render_sprite.obj`, compiled candidate: `build/base/source/render/render_sprite.obj`.
- Compiler: XDK 3911 Microsoft C/C++ 13.00.9254.1 with the repository flags `/O2 /Oy- /DDEBUG /Dxbox`.
- The production file was an empty symbol skeleton. No local branch, worktree, object log, or later Git commit contained a reconstruction.
- HCEA's recovered C supplies an independently named topology donor for all ten functions. The January disassembly, relocations, assertion text/lines, section ownership, and XDK output remain authoritative; HCEA names or layouts are never accepted without January corroboration.

## Baseline

The January object owns ten functions and 4,192 padded code bytes:

| Function | Padded bytes | Relocations | Baseline |
| --- | ---: | ---: | --- |
| `code_0017c760` | 304 | 27 | missing |
| `code_0017c890` | 112 | 3 | missing |
| `build_sprite_prepare_for_window` | 144 | 16 | missing |
| `code_0017c990` | 384 | 20 | missing |
| `build_sprites_begin` | 160 | 9 | missing |
| `build_sprites_end` | 304 | 15 | missing |
| `code_0017cce0` | 480 | 31 | missing |
| `build_sprite_compute_vertex_fade` | 112 | 2 | missing |
| `build_sprite` | 1,552 | 59 | missing |
| `build_sprite_rotational` | 640 | 38 | missing |

The target additionally owns a four-byte `one_over_full_circle` constant, the sprite-orientation enum/name table (24-byte `.data`, four relocations), a one-byte warning flag in `.bss`, and the function-specific string COMDATs. The baseline candidate owns none of those sections.

## Recovered layouts

- `build_sprite_data`: 0xA4 bytes; bitmap tag at +0x00, maximum sprite count +0x04, shader +0x08, total count +0x0C, flags +0x10, centroid +0x14, group count +0x20, eight 0x10-byte groups from +0x24.
- `build_sprite_group`: dynamic vertex-buffer index +0x00, locked vertices +0x04, sprite count +0x08, bitmap +0x0C.
- `bitmap_group_sequence`: 0x40 bytes, sprite block at +0x34.
- `bitmap_group_sprite`: 0x20 bytes, bitmap index +0x00, bounds +0x08, registration point +0x18.
- `dynamic_screen_vertex`: 0x18 bytes, point +0x00, packed color +0x0C, texture coordinates +0x10.
- `build_sprite_globals`: 0x28 bytes; coverage +0x04, large-sprite counter +0x08, screen-area scale +0x0C, viewer-space up +0x10, viewer-space forward +0x1C.

All offsets above are corroborated by January instructions, not merely copied from HCEA.

## Experiment families

### Family 1: direct HCEA topology, retargeted to January evidence

The first readable-C pass recovered nine of the ten bodies (the rotational builder remained unwritten) and immediately produced three strict-exact functions. This is strong evidence that the cross-build control-flow topology is useful, while the January strings, layouts, private helper conventions, and exact statement order still have to be recovered independently.

| Function | Target/base bytes | Target/base relocs | Result |
| --- | ---: | ---: | --- |
| `code_0017c760` | 304 / 304 | 27 / 27 | near, caller-driven private-register context differs |
| `code_0017c890` | 112 / 112 | 3 / 3 | **strict exact** |
| `build_sprite_prepare_for_window` | 144 / 144 | 16 / 16 | normalized code exact; only the format-string relocation named the wrong literal |
| `code_0017c990` | 384 / 384 | 20 / 20 | near, allocation success/failure branch order and literals differ |
| `build_sprites_begin` | 160 / 160 | 9 / 9 | **strict exact** |
| `build_sprites_end` | 304 / 304 | 15 / 15 | near, group eligibility predicate differs |
| `code_0017cce0` | 480 / 544 | 31 / 34 | nonexact; an unintended out-of-line `square` call and assertion spelling add code/relocs |
| `build_sprite_compute_vertex_fade` | 112 / 112 | 2 / 2 | **strict exact** |
| `build_sprite` | 1,552 / 1,504 | 59 / 59 | near; same call/relocation inventory, but source spelling and downstream scheduling differ |
| `build_sprite_rotational` | 640 / absent | 38 / 0 | not yet implemented |

The first pass also proved several literal defects by comparing relocation destinations, which a raw byte-only comparison would miss. The target says `%.1f`, `accomodate`, `screen geometry -- tell Bernie!!`, `references bitmap -1 (tell matt).`, and `sprite count exceeded.` exactly. These are corrected in Family 2 rather than waived.

### Family 2: January literals, exact layouts, and compiler-shape corrections

The second pass corrected every January-proven literal and layout, removed an
accidental out-of-line `square` call, recovered the private helper argument
order, and implemented the rotational builder from HCEA topology plus January
instructions. It reached five strict-exact functions and made every owned data
section exact. This is the best measured state, but it is not admissible as a
whole object.

| Function | Target/base bytes | Target/base relocs | Strict result | Target/base normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `code_0017c760` | 304 / 304 | 27 / 27 | nonexact | `c6b9005a5dc2824803c3b2e4fe8b928323d78afa1882c3ca25c880df2254af98` / `82adc7313576982572b73a180d11b6a95db74518a274ddf7f6c65fd34bef2469` |
| `code_0017c890` | 112 / 112 | 3 / 3 | **strict exact** | `2c566047667adab39f57f5d21899fed1361cb7c004afd0a935e655dd81b008b2` / same |
| `build_sprite_prepare_for_window` | 144 / 144 | 16 / 16 | **strict exact** | `94e2d13923f87367e679198ef1960910aeeabb0fe9adfad6a4147580bf79d321` / same |
| `code_0017c990` | 384 / 384 | 20 / 20 | one instruction differs | `f2e9787070fc78bf43c1c343885bd3862c9ec91ddaabe67b82ec322326e2b780` / `9b5feb5ccea3c320af8db9e5bc47da5df24b2b36c6e857b7ac4f10cf77ff1ee5` |
| `build_sprites_begin` | 160 / 160 | 9 / 9 | **strict exact** | `83862e3a9a5af2feb609a6680c00525bf4cbf0d9fb03e4be4f4978794150cfb3` / same |
| `build_sprites_end` | 304 / 304 | 15 / 15 | near | `5632bbe3bff6861292599a52d5e8fed53ae2e83c3179b3c2374accfab5e69c47` / `9e2448999e9cc653cff35661d15d6cd94a28693eef592c4c3d7dc6f64f1b5d1c` |
| `code_0017cce0` | 480 / 480 | 31 / 31 | **strict exact** | `b29a24b6398f7bf7e12b8997d78b1c4c11c63ebcd5b4a10b267f7f4b07ddd167` / same |
| `build_sprite_compute_vertex_fade` | 112 / 112 | 2 / 2 | **strict exact** | `71d935a90f9b815d7aef01a33b1b212825e8765f76fccc69c8e5aa977388ec17` / same |
| `build_sprite` | 1,552 / 1,536 | 59 / 59 | broad codegen divergence | `2a5e72c78e39ebdd8a0634b1b21cfdbaf43385682039b87916dc0080bc3fb68f` / `edc7bec86748a285e06831aa8d59ae2166c9b6f8416c64809e8e5518223669a2` |
| `build_sprite_rotational` | 640 / 656 | 38 / 38 | broad codegen divergence | `99c5dce7c5488e3c60354b64eb628a531cc904fcf73166212c4c3270855990ce` / `3399fb9b9497a1bab279edb0b3b2aa4f659ec368d2bf1ae5814ace2ca7a99019` |

The five exact functions total 1,008 padded code bytes. The object remains
5/10, so none of this production source is retained under the atomic-object
rule.

## Owned data evidence at the best state

All four owned data symbols passed `section_infos_equal`, including bytes,
relocation identities/addends, and section ownership:

| Symbol | Target/base bytes | Target/base relocs | Strict result | Normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `one_over_full_circle` | 4 / 4 | 0 / 0 | **strict exact** | `1648c99fd1eb13d11073d9725893dbf161a5ddbc37fe943b26d2b8fb9b9ca7fe` |
| `data_0030e778` | 24 / 24 | 4 / 4 | **strict exact** | `bb1adbd4c472c8af046c05edd2479b31f7c0419f326ec47704062eb7d81d774f` |
| `global_sprite_render_orientations_enum` | 24 / 24 | 4 / 4 | **strict exact** | `bb1adbd4c472c8af046c05edd2479b31f7c0419f326ec47704062eb7d81d774f` |
| `bss_004c0518` | 1 / 1 | 0 / 0 | **strict exact** | `6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d` |

## Measured source families and rejected shapes

### `code_0017c760`: caller-coupled private ABI

The readable transform logic and all 27 relocation destinations are recovered,
and target/candidate padded sizes are both 304. The target private helper keeps
`data`, origin, and direction in a different EBX/ESI/EDI assignment from the
candidate. The first relocation is already displaced by three bytes and later
transform calls move by six bytes. This is driven by the two large callers'
live register context rather than a missing call or branch. No isolated helper
prototype change is evidence-backed until those callers match more closely.

### `code_0017c990`: return-width tie

The best body is identical for 124 of 125 instructions. The sole difference at
`+0x176` is:

```text
January:  movw   %di, %ax
candidate: movswl %di, %eax
```

Both sides are 384 bytes with 20 identical relocations. The following shapes
were measured:

- Changing the function return type to `short` or `unsigned short` changed
  early `NONE` exits and terminal bitwise operations, so it regressed the body.
- A corpus-backed named `short result = NONE`, assigning `group_index` only on
  success while retaining the `long` function prototype, enlarged the body to
  400 bytes. This disproved the last plausible control suggested by the exact
  `choose_random_array_element` donor.
- The best `long` return form was restored after each experiment.

### `build_sprites_end`: already-live byte zero-extension

The best target/candidate are both 304 bytes with 15 relocations. Target has
106 instructions and candidate 108. The meaningful residual is target
`movzbl %al,%eax` at `+0xe4` before remapping the first-person flag; candidate
uses the already-full-width EAX directly. The downstream push of ECX occurs at
a different slot, and the candidate ends with three padding NOPs.

Measured and rejected spellings:

- direct `(byte)data->flags`;
- a full-width flags local;
- a byte flags local;
- a byte/dword union;
- an unsigned eight-bit bitfield;
- stepwise mask/shift expressions;
- a byte `geometry_flags` result;
- a static byte-return helper (which either inlined to the same code or created
  an unwanted out-of-line symbol);
- narrowing only the extracted first-person Boolean before promotion.

The last shape changed the full-width flags load into a byte reload, produced
105 instructions, and substantially reshuffled the draw-call arguments. It did
not produce the target sequence and was reverted. A corpus motif scan found no
other reconstructed readable-C donor for this exact live-AL pattern.

### `code_0017cce0`: exact only with the January local set

The first topology pass accidentally emitted an out-of-line `square` call and
three extra relocations. Replacing it with explicit multiplication, correcting
the assert spelling, and retaining an otherwise-unused `unsigned long flags`
local between `mode` and `origin` recovered the January frame/register layout.
The result is strict exact at 480 bytes and 31 relocations. This is retained in
the log as a useful local-set observation, not as partial production code.

### `build_sprite`: structurally correct but not a closeout candidate

The candidate has all 59 target relocation destinations in the same semantic
inventory, but it is 16 bytes short (1,536 versus 1,552), and relocation
addresses drift throughout the function. The first relocation is already one
byte early, the fade/color block diverges further after the private helpers,
and the debug-draw tail is tens of bytes displaced. HCEA confirms the broad
control-flow topology, but January source spelling, private helper allocation,
FPU ordering, corner-loop form, and local lifetimes remain unresolved. This is
not a single scheduling tie.

Tested families include direct HCEA structure, January assertion/literal
corrections, explicit January-sized point/vector/basis locals, explicit corner
stores, scalar centroid accumulation, and the recovered debug-draw sequence.
These recovered semantics but did not converge the machine-code regions.

### `build_sprite_rotational`: topology recovered, codegen still broad

The readable HCEA topology recovered both polar/equatorial arms and all 38
relocation destinations, but the best candidate is 656 bytes versus January's
640, with 223 versus 211 instructions and frame `0x1c` versus target `0x24`.
The target emits the polar arm first; the candidate's optimizer selects a
different block order and register/local allocation.

Measured and rejected shapes include:

- independent versus nested pole-weight clamps;
- function-scope versus block-scope sprite-count locals;
- HCEA local names and explicit `angle`/`equator_weight` temporaries;
- declaration-order permutations;
- separate per-arm bitmap/sequence locals;
- direct versus temporary fmod/frame expressions.

Declaration/name changes did not create the target frame. No remaining local
spelling has evidence strong enough to justify another blind sweep.

## Donor and provenance audit

- Every other local worktree and branch examined still contains the original
  `render_sprite.c` skeleton; no completed local salvage exists.
- Git history contains only the initial skeleton for this translation unit.
- HCEA supplies useful, independently named topology for all ten functions,
  but its PPC codegen and evolved layouts cannot prove January XDK statement
  order.
- Focused upstream/fork searches found no readable January/XDK donor.
- The target itself and exact in-tree corpus were searched for the two small
  residual instruction motifs. Only the `choose_random_array_element` return
  donor justified a new experiment; that experiment failed as recorded above.

## Plain-English conclusion and park decision

In plain English: we know what this object does, and half of its functions can
already be expressed in C that compiles exactly like January. The remaining
half are not all "99 percent" compiler ties. Two small helpers are extremely
close, but the two large sprite builders still differ across many instruction
regions. Keeping the partial source would make the repository look more
complete than the strict evidence allows and would create a shared-header
collision with the newly integrated antenna work.

Therefore this lane is parked atomically. `source/render/render_sprite.c` and
all configuration are restored to the authoritative skeleton; only this log is
committed. No matching credit is granted.

## Reopen criteria

Reopen only when at least one of these becomes available:

1. January-local debug records or original source topology for the two large
   builders;
2. a readable-C donor that reproduces `build_sprites_end`'s already-live AL
   zero-extension without a reload;
3. a proven caller topology that anchors `code_0017c760`'s private ABI;
4. an exact cross-build/XDK donor for either large builder; or
5. a new corpus-derived source recipe that predicts one of the measured first
   divergences before editing.

Do not repeat declaration-order, byte-local, union, bitfield, static-helper,
return-type, named-short-result, or rotational clamp/scope experiments; they
are exhausted above.
