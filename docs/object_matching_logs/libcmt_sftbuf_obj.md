# `libs/libcmt/_sftbuf.obj` exact-match ledger

## Plain-English scope

This object temporarily buffers `stdout` or `stderr` when the stream refers to
a terminal. `_stbuf` allocates or reuses one of two 4096-byte buffers, falling
back to the stream's two-byte internal storage when allocation fails. `_ftbuf`
flushes and removes that temporary buffer state. The eight-byte `_stdbuf`
array owns the two persistent buffer pointers.

The admission unit is atomic: two functions plus the complete BSS symbol.

## Authoritative provenance

- Initial campaign base: `037de80f42ede0554b537056942afe00e94e4533`.
- Final revalidation base: `b831acac297ea8af5f1b02e1ff321ab70bb9b793`.
- XDK 3911 CL `13.00.9254.1`; unchanged `/O1 /Gy` flags.
- XDK `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- CodeView source:
  `d:/xbox-finalfre/private/sdktools/crt/crtw32/stdio/_sftbuf.c`.
- Microsoft source mirror commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `209c6fb70cc72ccde747a568e76801c4c553f6f3`.

The authentic archive is independently strict-identical to January before
source reconstruction: `_stbuf` is 136 bytes with six relocations, `_ftbuf`
is 42 bytes with one relocation, and `_stdbuf` owns eight zeroed BSS bytes.

## Experiment table

| ID | Readable source shape | Result | Disposition |
|---|---|---|---|
| P0 | Authentic Microsoft topology, public XDK `FILE`, named private flags, two-element zero-initialized `_stdbuf` | **EXACT**: `_stbuf` 136B/6 relocs, `_ftbuf` 42B/1 reloc, `_stdbuf` 8B/0 relocs and external ownership exact | Accepted |

## Original-bug audit

No original defect was found. Retaining the allocated pointer after `_ftbuf`
is intentional: later `_stbuf` calls reuse the persistent `_stdbuf` entry.

## House/Berth audit

- Typed readable C and named constants; no raw structure offsets.
- Vertically formatted parameters and explicit return in the `void` routine.
- No assembly, `volatile`, forced inline, UB, byte forcing, pragmas, flags, or
  comparator exceptions.
- Original behavior is preserved.
- Credit requires strict code, symbolic relocations, data bytes/storage class,
  full builds, semantic/admission, regression, and test gates.

## Final gates

- Forced build under unchanged `/O1 /Gy`: passed.
- `_stbuf`: exact normalized SHA-256
  `6744ed55bdf8efd5d94fb02ef65a8743349da4a9714fd3764902c9ccc9058b88`.
- `_ftbuf`: exact normalized SHA-256
  `ae9b3b801289a758903c5e991dce030e62f1e927fc284d3e4406d2a4926e1a81`.
- `_stdbuf`: exact eight zero bytes, no relocations, normalized SHA-256
  `af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`,
  external storage class in both objects.
- Full `halobetacache_build`: passed.
- Full `libcmt_build`: passed.
- Full progress/semantic report after rebase: passed with zero unit errors;
  the campaign is 356/833 Matching overall and libcmt is 89/212. The latter
  includes another object integrated into canonical during this lane.
- Semantic-match audit after rebase: 457 units scanned, 3,572 functions
  evaluated, 3,488 accepted exact, zero unit errors.
- Object-admission audit: zero new candidates, zero revocations; the sole
  pre-existing contradicted unit is `source/shell/shell_xbox`.
- Test suite: 179/179 passed.
- `git diff --check` and prohibited-construct scan: passed.
- Clean-tree regression snapshot/check: both functions `still_exact`, no
  failures, warnings, changed non-exact siblings, or ownership warnings.
- Every decisive gate above was repeated after rebasing onto the final base.
