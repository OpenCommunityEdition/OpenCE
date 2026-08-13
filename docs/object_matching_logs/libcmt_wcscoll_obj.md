# `libs/libcmt/wcscoll.obj` exact-match ledger

## Plain-English behavior

This January CRT routine does not implement a separate locale-aware sorting
algorithm. It passes both wide-string pointers straight to `wcscmp` and
returns that result. The compiler turns the return call into a tail jump, so
the complete runtime object is only five bytes long.

## Target inventory

- Runtime ownership: one external `/Gy` `.text` COMDAT, `_wcscoll`
- Runtime size: 5 bytes
- Runtime relocations: one `IMAGE_REL_I386_REL32` relocation at `+0x01`
  to `_wcscmp`, addend zero
- Owned `.data`, `.bss`, or `.rdata`: none
- Runtime aliases or duplicate entry points: none

The candidate object's ordinary compiler bookkeeping sections (`.drectve`,
`.debug$S`, and associative `.debug$F`) are not linked runtime ownership and
are deliberately excluded from runtime credit by the project comparator.

## Recovered source and experiment record

The only source shape tested was the natural CRT wrapper:

```c
return wcscmp(left, right);
```

Under the unchanged libcmt flags, XDK 3911 VC7 emits the January tail jump on
the first build. An inline comparison loop or a wrapper through any other
symbol would be structurally different and would fail the five-byte size or
the relocation-identity gate, so no speculative source-shape sweep was
warranted.

An older Claude commit contained the same source, but it was treated only as
an untrusted lead. This lane regenerated the target from the current January
executable, rebuilt the candidate independently, and repeated the strict
comparison from the current campaign baseline.

## Strict object result

- Compiler: XDK 3911 CL 13.00.9254.1
- Flags: `/O1 /Gy /I"xbox/include"`
- Size: `5/5`
- Relocations: `1/1`
- Normalized SHA-256, target and candidate:
  `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449`
- Relocation address, type, destination, and addend: identical
- `section_infos_equal = true`
- Runtime non-code ownership: absent on both sides

## House/Berth and readability audit

The source is readable C with parameters formatted one per line and an
explicit return. A single return is both natural and January-authoritative.
There are no raw Halo datum/tag accesses in this CRT wrapper. It uses no
assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment coercion, compiler-flag change, or comparator exception. No original
bug is present or needs a preservation comment.
