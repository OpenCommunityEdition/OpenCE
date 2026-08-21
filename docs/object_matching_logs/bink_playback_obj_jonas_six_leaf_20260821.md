# `bink_playback.obj` Jonas six-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`d3453bd46972f3c24148b2de14f495286fa3637c`. All six requested Bink playback
bodies were emitted together in one natural candidate compile, and every body
is independently strict. `bink_playback.obj` advances from 0/24 to 6/24 exact
functions and remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_bink_playback_active` | 27 | 32 | 2 | `4ebd087225d3e39311134215393b553b8fdac987b5cd9f62deb0f90a948851ab` |
| `_bink_playback_ui_rendering_inhibited` | 27 | 32 | 2 | `ccd797d507a0953292d48cc166f94d038072b29168dbb4152732432fb5462962` |
| `_bink_playback_in_progress` | 14 | 16 | 1 | `e3d713882962f29c50f9529269c70d85d4e93d911caff2d69af551668dff0246` |
| `_bink_playback_initialize` | 43 | 48 | 6 | `0946048562578bbf5c22fff37df8532829eac3b0af18a0d8e65b05989bf29ef9` |
| `_bink_playback_dispose` | 35 | 48 | 4 | `ff76873293322413bbd4a98ee38679cfd938a412ea8e9397bf13fcda18f9032a` |
| `_bink_playback_update` | 15 | 16 | 2 | `dac8a7de77356d5fc0d225152b500fa42898f7c82b15864f36acfe8920714548` |
| **Wave gain** | **161** | **192** | **17** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends for all six functions.
The object now reports 161/3,803 meaningful code bytes and six exact functions.
The target's remaining eighteen functions and all 1,701 non-code bytes stay
outside this wave's credit.

## Provenance and signatures

- Baseline `source/bink/bink_playback.c` blob:
  `1c430c68536282aa5c89914154ad400cedfef4d6`.
- Retained source blob/SHA-256:
  `05a29dc73419d1acd115fc67ebfc31c879fcba8a` /
  `4bb471b652e103568d5fff442f7cfa6b492810910848d4bc0957078375999577`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `6066221c0dd9bde77e2163643a063c7ff1ecd7d4225fea496d99d3d2274bf153`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its six exact-name stubs
  authenticate the public names and signatures, but its console no-op bodies
  are not used for PC behavior.
- Stian's preserved PC reconstruction commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`. It supplies the handle,
  initialized-state, UI-inhibit, lifecycle, and per-frame check semantics.
  January disassembly and relocations remain authoritative where its raw
  reconstruction lacks types or names the SDK call incorrectly.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, with the repository's unchanged XDK 3911 command and flags.

The retained signatures are:

```c
boolean bink_playback_active(
	void);
boolean bink_playback_ui_rendering_inhibited(
	void);
boolean bink_playback_in_progress(
	void);
void bink_playback_initialize(
	void);
void bink_playback_dispose(
	void);
void bink_playback_update(
	void);
```

## Authenticated behavior and one-shot record

The translation unit declares a typed, extern-only 0xD8 view of
`bink_globals`: `initialized` is at +0, playback flags are at +4, and the Bink
handle is at +8. Compile-time layout checks fix all three facts without owning
the target's storage.

`bink_playback_active` tests the handle before initialized state.
`bink_playback_ui_rendering_inhibited` tests initialized state before flag bit
3. `bink_playback_in_progress` tests only the handle. Initialization clears the
complete 0xD8 state, passes the target's allocation and release callbacks to
`RADSetMemory`, then sets initialized. The `_RADSetMemory@8` relocation and
callback order are fixed directly by January COFF; no donor SDK-call label is
substituted. Disposal is guarded by initialized state, stops playback, then
clears the complete state. Update checks the PC frame-rate-throttle/pregame
byte and tail-calls the target's playback check-stop helper. Every void body
has an explicit terminal return.

All six candidates were compiled together exactly once. There are no rejected
candidates, spelling retries, declaration retries, control-flow retries, or
compiler-control experiments in this wave.

## Policy, scope, and validation

Only `source/bink/bink_playback.c` and this new Jonas-owned ledger are changed.
No shared header, storage definition, data section, configuration, frozen-five
source, semantic exception, parked record, pre-existing Markdown, or
Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, pointer or union
pun, inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing. The final COFF defines only the six requested code
COMDATs plus debug metadata; it defines no `.rdata`, `.data`, `.bss`, COMMON
symbol, or global storage.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all six retained functions pass, including all
  17 relocations.
- Semantic audit: 470 units, 4,058 functions evaluated, 3,918 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,979 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,966/11,060 exact functions,
  475,799/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes. This wave adds six functions and 161 meaningful bytes with no
  data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `bink_playback.obj` is the final same-path reproducibility proof.
No push is performed.
