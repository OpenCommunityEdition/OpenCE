# `libs/libcmt/timeset.obj` — data-only CRT reconstruction

`timeset.obj` has no executable code. It supplies abbreviated English day and
month names and the initial Pacific timezone values. The timezone-name buffers
are writable because the CRT can replace their contents.

The January split target is index 660. Its runtime ownership is exactly
61 bytes of `.rdata` and 152 bytes of `.data`, with two pointer relocations.
The corresponding XDK 3911 `libcmt.lib` member (SHA-256
`8cbc84af96e6fccf96da2d883e34b655559020c50d1e46641afe6bc85c13281a`)
records the original Microsoft C compiland ending in `timeset.obj`
and the private names `_tzstd` and `_tzdst`. The archive SHA-256 is
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
No archive code or binary has been copied into this repository.

The reconstructed `timeset.c` compiles under the existing `/O1 /Gy` libcmt
flags. Its `.rdata` bytes match January and the authentic member exactly
(61 bytes, SHA-256 `bdd75065889fd924d17915160c883c4a711e971b0d2cd7a21663c77e07cbf107`).
Its `.data` content other than relocation fields likewise matches: the
measured section is **152 bytes**, flags `0xc0400040`, with the
same public names, offsets, storage classes, and two `DIR32` pointer sites.
The authentic archive member and our rebuild have identical raw `.data` bytes
(SHA-256 `018f624ae93a3eb78aa47a3b4806c5bb6ddf54ce9fe06bf93fb973749f724f4a`).

The original split misattributed the two pointer relocations to `__dstbias`
with addends 8 and 72. The authentic XDK member proves they reference the
private buffers `_tzstd` and `_tzdst` at data offsets 16 and 80. The two
CodeView-confirmed names were added to `config/symbols.json` at executable
file offsets 3249536 and 3249600, both static, and January was re-split.
The refreshed target and rebuilt C now have the same symbol identities and
addends. The hardened `section_infos_equal` check returns true for both
runtime sections: `.rdata` normalized SHA-256
`bdd75065889fd924d17915160c883c4a711e971b0d2cd7a21663c77e07cbf107`
and `.data` normalized SHA-256
`018f624ae93a3eb78aa47a3b4806c5bb6ddf54ce9fe06bf93fb973749f724f4a`.
The two `.data` relocations resolve to internal offsets 16 and 80 on both
sides. No semantic exception or comparator waiver was used.

House-rule review: all globals have provenance-backed names and natural CRT
types, corroborated by the project's authentic XDK `<Time.h>`; the 64-byte
buffers and array extents follow the measured XDK member.
No inline assembly, optimizer pragma, forced alignment, invented global,
manual byte patch, or semantic bug was added. The full build, board sweep,
park validation, and maintained tests passed; this data-only object is marked
`Matching`.
