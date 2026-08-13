# `libcmt/strncmp.obj` provenance and park ledger

## Verdict

`strncmp.obj` is **not admitted**. It is a Microsoft vendor-assembly object,
and the project correctly prohibits replacing it with inline assembly or a
byte-forced C imitation. Production source and config remain absent/MISSING.

## Plain-English behavior

The routine compares at most a caller-supplied number of bytes from two
strings. The shipped implementation is a hand-tuned x86 scanner: it first
searches the left range for a null byte with `repne scasb`, then compares the
effective range with `repe cmpsb` and normalizes the unsigned-byte ordering to
`-1`, `0`, or `1`.

## Primary provenance

The actual XDK 3911 archive member was extracted from
`xbox/lib/libcmt.lib` as `obj\\i386\\strncmp.obj` (archive-member SHA-256
`b6550239dedf2a56c6da6bf850c9300110ee6c8e880a99defa52ac233cfdb339`).
Its COFF `.file` is `..\\i386\\strncmp.asm`, not C. It records an assembler
compiler identifier and owns a 16-byte-aligned ordinary `.text` section.

That archive member is strict runtime-exact to January:

- function: `_strncmp`
- size: 56/56 bytes
- relocations: 0/0
- normalized SHA-256:
  `e2f944563c7483cd4085f1878325de050424e2b192c3220c5e73a802f698e0da`
- section flags: `0x60500020` (code, execute/read, 16-byte alignment)
- target-owned runtime data: none

## Measured readable-C discriminator

| ID | Shape | Result |
| --- | --- | --- |
| S01 | Natural bounded byte loop, null stop, unsigned-byte subtraction | 52 bytes, 0 relocs, SHA `805211b3faa8f2f73d607bbc34f814300933a36824a3060b209ed68dc9cfdb1b`; structurally different and rejected |

The four-byte size gap understates the difference: normal VC7 C emits scalar
loads and branches, whereas the historical object explicitly uses string
instructions and 16-byte assembly-section alignment. Its provenance removes
the premise that another readable C spelling should recreate it.

## Do-not-repeat and reopen rule

- Do not search loop spelling, pointer-increment, or return-normalization
  variants; they cannot honestly recover an assembly-authored implementation.
- Do not copy the target bytes, use inline assembly, add `volatile`, force
  alignment, or grant a semantic/comparator exception.
- Reopen only if project policy later permits verbatim provenance-vetted
  Microsoft assembly or adopts an explicit vendored-object admission class.

## House/Berth audit

No production C, matching status, compiler option, ownership symbol, or credit
is changed. This documentation makes the limitation explicit in plain English
and retains the measured natural-C result so future work does not repeat it.
