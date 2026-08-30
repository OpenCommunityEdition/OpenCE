# `rasterizer_frame_statistics.obj` FPS sampler fixed point (2026-08-29)

## Result

The complete typed HCEA FPS-window donor was mapped against the January COFF
after reading the two existing object ledgers and their pinned Claude/HCEA
documentation and tooling evidence. The January version omits the later
build's dropped-frame stream and accumulation-mode alternative, but otherwise
preserves the same typed timestamp-window algorithm.

One natural January-version candidate was compiled once. It reproduced all
21 relocation identities, including the frame-statistics pointer, BSS sample
array/count addends, `system_milliseconds`, and both floating constants. It
did not reproduce VC7's register and lifetime schedule:

| | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | 384 | 21 | `c6275f2bf09ccb0363daeeff9afcab28656ef7f51f2a73b4f0780f5fdb095a92` |
| typed candidate | 352 | 21 | `107238c3aa1f0dff108a477dfa8a33a05ff1f113301d5201319cee833f572c1f` |

The candidate kept the current timestamp in EBX, used a 12-byte local frame,
and held the zero/default sample count in EAX. January keeps the output pointer
in EBX, uses an 8-byte local frame, preserves the sample count in SI, and emits
separate zero/clamped/ordinary exits. These are compiler topology differences,
not missing semantic evidence.

The complete candidate and its temporary typed layout extensions were removed
with no production source delta. The baseline object was rebuilt and the gate
again reports 5 exact, zero residual, and five unwritten functions. Reopen only
with authentic January source or a distinct provenance-backed source topology;
do not tune declarations, expressions, or control flow around this rejected
shape.
