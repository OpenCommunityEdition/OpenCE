# `source/cseries/errors.obj` exact-match strategy ledger

## Plain-English outcome

The object is complete. All ten functions, all thirteen owned read-only string
sections, its one-byte initialized global, and its sixteen-byte uninitialized
state block reproduce the January 2002 object exactly under the strict COFF
comparator.

The final mismatch was not a mysterious compiler choice. The reconstructed
source had invented a local copy of `error_globals.message_buffer_size` and a
ternary expression for choosing the retained message-buffer suffix. Those two
choices gave VC7 a different value graph: it kept `newline` in `EBX` and
`new_size` in `ESI`. January instead spills `newline`, keeps the raw
`prefix_size+copy_size` sum in `EBX`, and reads the global size directly.
Later Halo source independently preserves that same explicit null-first branch
and raw sum subscript. Restoring that topology made the function byte-exact.

## Scope and verification baseline

- Translation unit: `source/cseries/errors.c`
- January target: `build/split/source/cseries/errors.obj`
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`
- Starting campaign commit: `063ec49e403216ee9bfb880f0b7d87ae44600c0d`
- Starting state: 9/10 functions exact; 483/483 target-owned non-code bytes exact
- Sole starting residual: `_error`, target/base `0x270`, 50/50 relocations,
  target SHA `61bf8aad11f19fd443093f2eba0bae2db27d90af0bc156ec78a530246ae47fd7`,
  base SHA `09377b04504056b8f2278b92bf682d5bd8be32a924e9e87c53cddd954f11ce53`

The starting relocation count and destination sequence agreed, but only 27 of
50 relocation addresses agreed. The first normalized-byte divergence at
`+0xA8` was a downstream branch displacement; the first structural divergence
was at `+0x111`.

## New experiment matrix

| ID | One changed source factor | Strict result | Disposition |
| --- | --- | --- | --- |
| E01 | Load `message_buffer_size` before calling `csstrlen`, instead of after it. | `0x270`, 50 relocs, SHA `51789f02...`; the call and global-load relocations moved and the previously exact prefix regressed. | Rejected and reverted. Call/load statement order is not January's topology. |
| E02 | Replace only the ternary suffix selection with an explicit null-first `if (!newline) ... else ...`. | `0x270`, 50 relocs, SHA `2f3d855d...`; branch polarity moved toward January but `newline` remained in `EBX` and the persistent-value allocation stayed wrong. | Rejected alone. Retained as provenance for the combined topology. |
| E03 | Combine the explicit null-first branch with indexing the terminating zero by raw `prefix_size+copy_size`. | `0x270`, 50 relocs, SHA `7ef8e31f...`; it preserved the raw sum but the invented cached short local still constrained the wrong allocation. | Rejected alone. |
| E04 | Use only the raw `prefix_size+copy_size` zero-store subscript. | `0x270`, 50 relocs, SHA `ea61f690...`; this successfully spilled `newline` and preserved the raw sum in `EBX`, proving the lvalue topology control, but the cached short local still changed the early allocation. | Rejected alone; key diagnostic step. |
| E05 | Restore the broader source-provenance topology: remove the cached `message_buffer_size` local, use the global member directly, select the suffix with explicit null-first `if/else`, terminate using raw `prefix_size+copy_size`, and publish the cast sum directly. | **STRICT EXACT:** `_error` `624/624`, 50/50 relocations with identical addresses, types, destinations and addends, normalized SHA `61bf8aad...`. All nine exact siblings remain exact. | **Accepted and retained.** |

Historical work before this ledger also covered declaration order, local scope,
`register` hints, and a now-prohibited `volatile` experiment. Those families
either emitted the old baseline or regressed relocation addresses. They were
not repeated.

## Final strict evidence

- Functions: 10/10 strict exact.
- `_error`: `624/624`, 50/50 exact relocation records, SHA
  `61bf8aad11f19fd443093f2eba0bae2db27d90af0bc156ec78a530246ae47fd7`.
- Owned `.rdata`: 13/13 sections, 466/466 bytes exact.
- Owned `.data`: 1/1 byte exact.
- Owned `.bss`: 16/16 logical bytes and symbol layout exact.
- Total target-owned non-code data: 483/483 bytes exact.

No assembly, volatility, undefined behavior, force-inlining, byte patches,
compiler-flag changes, or fuzzy credit were used.

## General lesson

When the machine code shows a persistent global value in a register but the
reconstruction creates a local cached copy, do not assume the local is
original. Compare the value graph against later-source provenance. Reading the
member directly can be the legal-C register-lifetime control. Likewise, raw
long-expression lvalue provenance can make VC7 preserve a sum independently
from a narrowed stored value even when both are mathematically equal.
