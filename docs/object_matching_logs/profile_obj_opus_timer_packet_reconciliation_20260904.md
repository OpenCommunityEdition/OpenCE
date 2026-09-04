# `profile.obj`: Opus timer-packet reconciliation (2026-09-04)

## Disposition

Six target-owned timer functions from frozen donor commit
`8aed00b49` are retained on canonical base
`933b9df1d322b485d1421e1d7afc779676c5e0e0`. `profile.obj` advances from
18/44 to 24/44 strict-exact functions, with zero residual functions and 20
still unwritten. The accepted delta is 450 meaningful / 480 padded code
bytes, six functions, and zero data bytes. A fail-closed stable-verdict diff
reports all six gains and no regressions.

The donor's source-level timer semantics are retained, but its static
`profile_timebase` helper is not. That helper makes all six call sites exact
only by being automatically inlined and also emits an unnecessary candidate-
only code COMDAT. The final source instead uses the historically established
unit-local `QUERY_TIMEBASE` macro for the exact repeated January `rdtsc`
primitive. It emits no extra function or COMDAT.

The January split object is SHA-256
`f40acee3f68569049519a0f2c71775232cba27c6578e2bfc2aebfc52eca40587`.
The final rebuilt object is SHA-256
`be991dd9b28b6242d2859ca46c49635941798d5a47a198cbf51a548dd3f1b7ec`.
The final `profile.c` Git blob before commit is
`8b81f642af7829beab69037a63bfbcfe10ea48cc`.

## Evidence and source authenticity

The target itself supplies unusually strong source-shape evidence. Each of
the accepted start/end functions contains the same seven-instruction timing
primitive:

```text
push eax
push edx
rdtsc
mov dword ptr timebase, eax
mov dword ptr timebase+4, edx
pop edx
pop eax
```

The surrounding ordinary C then stores the 64-bit start/end value and, for
each end function, converts elapsed ticks to the project's `real` milliseconds
before accumulating `total` and `frame_total`. `profile_render_start` also
resets the current frame's render-window count. These operations account for
every target instruction and relocation and are meaningful for ordinary
profiler state.

The macro spelling is not claimed as a January PDB recovery. It is the
descriptive spelling already used by the project's older exact Profile
reconstruction (`8ce0ae40f`), and the repeated target shape independently
constrains its body. The compatible VC7/XDK toolchain has no cleaner C
primitive that produces this sequence. The official open-source Bungie
Marathon 2/Infinity tree was also searched for an analogous `rdtsc`/profile
primitive and supplied no stronger carrier or name evidence.

This is a sparse low-level timer primitive permitted by the helper/math
assembly exception, not an assembly transcription of gameplay logic. No
`volatile`, `register`, compiler barrier, pragma, forced-inline/noinline
attribute, raw byte emission, undefined behavior, dummy reachability, fake
dependency, or nonsensical branch is present. The packet introduces no
private address-derived function name and no address-derived global. Existing
unrecovered Profile names outside this selective donor packet are unchanged.

## Exact inventory

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_profile_render_start` | 48 | 48 | 3 | `7c22415d901cfdf571139101fc7ad24a814e5bb41fbb7c50aed7edb8294f6532` |
| `_profile_render_end` | 108 | 112 | 10 | `f3d008280cfe6f370840a84c53f908d2b4887e24f759eb067963e512a807f580` |
| `_profile_texture_start` | 39 | 48 | 2 | `e6a2159339ce6d6d2d684aa67e7f30a9e365cdbd0afad038e4fa8a6010b98534` |
| `_profile_texture_end` | 108 | 112 | 10 | `f3d008280cfe6f370840a84c53f908d2b4887e24f759eb067963e512a807f580` |
| `_profile_idle_start` | 39 | 48 | 2 | `e6a2159339ce6d6d2d684aa67e7f30a9e365cdbd0afad038e4fa8a6010b98534` |
| `_profile_idle_end` | 108 | 112 | 10 | `f3d008280cfe6f370840a84c53f908d2b4887e24f759eb067963e512a807f580` |

## Rejected static-helper carrier

The donor's `static __int64 profile_timebase(void)` is automatically inlined
at all six sites, but VC7 also emits `_profile_timebase` as a 32-byte,
zero-relocation `.text` COMDAT. Its section flags are `0x60501020`, its
selection is 1 (`IMAGE_COMDAT_SELECT_NODUPLICATES`), and the rebuilt object
contains no incoming relocation to it. The split target has no corresponding
section.

An isolated XDK 3911 `Link.Exe` probe rooted at `_profile_render_start`, with
`/OPT:REF /INCREMENTAL:NO`, produced an image and map without
`_profile_timebase`; the linker can discard this unreferenced copy. That is
useful ownership evidence, but it does not satisfy the packet's stricter
"accept only if no cleaner source exists" condition. The unit-local macro
produces the same exact target functions, does not require forced inlining,
and emits no `_profile_timebase` symbol or surplus code section. The helper
carrier is therefore rejected rather than admitted through a linker-only
exception.

The final candidate has 24 `.text` sections, no `_profile_timebase` symbol,
and no `point_from_line3d` symbol or COMDAT.

## Prototype blast-radius result

The existing render and texture declarations already reside in their owning
`source/cseries/profile.h`. The two idle functions have no current caller or
foreign translation-unit declaration. Adding their declarations to the owner
header was nevertheless tested in both the adjacent timer block and at the
end of the header. Each otherwise harmless placement rebuilt the header's 25
includers and reopened `_unit_preprocess_node_orientations`, moving
`units.obj` from 189/189 to 188/189 exact because of VC7 definition-position
sensitivity. Both experiments were reverted; `profile.h` is byte-identical to
the base.

No prototype was placed in an unrelated `.c` file. If an idle-timer caller is
later reconstructed, its declaration belongs in `profile.h`, but that shared-
header change must first preserve the January Units inline schedule.

## Canonical replay

The isolated packet was replayed after the admitted `render_debug` geometry
packet at canonical commit `87fbeb758`.  The current-tree stable diff again
finds exactly the same six gains and zero regressions.  Current canonical
progress is therefore **850,394 / 2,198,102 meaningful code bytes** and
**5,934 / 11,060 exact functions**, with 2,007,666 matched data bytes and
391/833 linked objects.  The combined semantic report contains 6,003 accepted
exact functions and zero unit errors.  Full-object SHA-256 values differ
between rebuilds because the COFF header carries build metadata; normalized
per-section equality and the stable function verdicts are the admission gate.

## Validation

- Full Ninja build: pass.
- `tools/campaign/gate.py source/cseries/profile`: 24 exact, zero residual,
  20 unwritten.
- `tools/campaign/gate.py source/units/units`: 189 exact, zero residual, zero
  unwritten.
- Stable verdicts: six gains / 480 padded bytes, zero regressions across
  8,245 compared functions.
- Meaningful progress: 848,913 to 849,363 code bytes; 5,921 to 5,927 exact
  functions; data remains 2,007,666 bytes.
- Semantic report: 5,979 semantic exact, 5,996 accepted exact, zero unit
  errors.
- Park validation: 196 active, zero stale, zero invalid.
- Object admission: zero candidates, zero contradictions, zero revocations;
  the two inherited rejections remain `player_rumble` and `models`.
- Scoped fake-match scan: seven review leads, all seven lines of the single
  target-proven `QUERY_TIMEBASE` primitive; zero unreviewed/prohibited
  constructs.
- Tool tests: 261 passed.
- Canonical protected working-tree hashes were unchanged:
  `bitmap_utilities.c` =
  `ab6002bf2baecdbc18c7b308bb7846c58fbcf1c7c72f586621d2b1f271d49140`,
  `bitmaps.h` =
  `b435f38e406c7dcdb60b4df12e78bcf98d1e4b0e57fe2ab2240bc78f799eba93`.

The source remains formatted with one parameter per line, `void` on its own
line for no-argument functions, project `real` rather than raw `float`, and an
explicit terminal `return;` in every accepted function. No completion label,
park, admission exception, build flag, or comparator rule is added.
