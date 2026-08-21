# `bink_playback.obj` Jonas stop/render follow-up

## Result

This bounded source-only follow-up starts from authoritative integration commit
`be6b921be1e2b4db094fab70139f6ea7233d3b68`. Both requested bodies were
emitted together in one natural candidate compile, and both are independently
strict. `bink_playback.obj` advances from 6/24 to 8/24 exact functions and
from 161/3,803 to 349/3,803 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_bink_playback_stop` | 98 | 112 | 13 | `f81925c7f56891737b5aec985555545736944c5f4ab2009b620c50084910bf95` |
| `_bink_playback_render` | 90 | 96 | 12 | `0670afcbb0884c06cf5e67cc071cd1780b2cdb7e033669041926ed82bbf9e269` |
| **Wave gain** | **188** | **208** | **25** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends for both functions.
The six previously accepted Bink playback leaves remain independently strict.
The other sixteen target functions and all 1,701 non-code bytes remain outside
this wave's credit.

## Provenance and signatures

- Baseline `source/bink/bink_playback.c` blob:
  `05a29dc73419d1acd115fc67ebfc31c879fcba8a`.
- Retained source blob/SHA-256:
  `8b4d2948423243f98ce2a0a838a96276661d59b5` /
  `0809311fe2d118a1190f462b79d304f5d0be3aabf78535e803f6b26487ea30ed`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `6066221c0dd9bde77e2163643a063c7ff1ecd7d4225fea496d99d3d2274bf153`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name console
  stubs authenticate both public names and signatures, but not PC behavior.
- Stian's preserved PC reconstruction commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`. It supplies the stop and
  render call topology and state semantics. January disassembly and COFF
  relocations remain authoritative for target helper names and typed state.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, with the repository's unchanged XDK 3911 command and flags.

The retained signatures are:

```c
void bink_playback_stop(
	void);
void bink_playback_render(
	void);
```

## Authenticated behavior and one-shot record

The translation unit extends its typed, extern-only 0xD8 view of
`bink_globals`: `needs_decode` is at +1 and `rendered_frame_count` is at +0x30.
It also declares an extern-only typed view of the saved frame-throttle byte at
`_data_0031693c+4`. Compile-time layout checks fix those facts without defining
or claiming either target object.

`bink_playback_stop` is guarded by initialized state. It unsuppresses events
for playback flag bit 2, closes and clears an open Bink handle, releases the
texture cache through `_code_001b5cc0`, and reloads the main menu for flag bit
5. It then preserves the saved frame-throttle byte across the flags clear,
restores the live throttle byte, and tail-calls `attract_mode_reset_timer`.

`bink_playback_render` requires initialized state and an open handle. Pregame
frame throttling forces a decode; otherwise a frame with no pending decode
skips that work. A decoded frame clears `needs_decode`, the render helper runs,
the +0x30 frame counter increments, and non-pregame playback tail-calls the
existing check-stop helper.

Both candidates were compiled together exactly once. There are no rejected
candidates, spelling retries, declaration retries, control-flow retries, or
compiler-control experiments in this follow-up.

## Policy, scope, and validation

Only `source/bink/bink_playback.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, frozen-five
source, semantic exception, parked record, pre-existing Markdown, or
Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing. The final COFF defines only the eight accepted code
COMDATs plus debug metadata; it defines no `.rdata`, `.data`, `.bss`, COMMON
symbol, or global storage.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: both new functions and all six inherited Bink
  leaves pass, including all 42 combined relocations.
- Semantic audit: 470 units, 4,065 functions evaluated, 3,925 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,986 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,973/11,060 exact functions,
  476,401/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes. This wave adds two functions and 188 meaningful bytes with no
  data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `bink_playback.obj` is the final same-path reproducibility proof.
No push is performed.
