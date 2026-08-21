# `xbox_sound_cache.obj` Jonas leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`9c260513b9c51ce9d6ad14d725159610ae5a1a96`. Six HCEA-backed public bodies
were emitted together in one natural code-producing compile. Five were
independently strict byte-and-relocation exact immediately. The nonexact
hardware-unlock body was removed fail-closed without a retry or code-generation
tune.

`xbox_sound_cache.obj` advances from 0/18 to 5/18 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_sound_cache_delete` | 37 | 48 | 5 | `85a33804560319ef928283fbf1c0b78f43b85da0d923a2a5bf9b3b4edef0c4ac` |
| `_sound_cache_open` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_sound_cache_idle` | 57 | 64 | 7 | `f5da87260c1d47120820c14b6a77baffb3010be638f316fd4addcae611447efd` |
| `_sound_cache_sound_hardware_lock` | 49 | 64 | 3 | `19693b189bbb97a998878732786d23ab6db87e59b83c42e68139bd6bb87d128f` |
| `_sound_cache_close` | 81 | 96 | 7 | `69581985a612c072b295a5f8849cf945583122f548d9dc7ca69c8486dd0766d7` |
| **Wave gain** | **237** | **288** | **24** | |

The hardened comparator reports identical normalized bytes, padded extent,
relocation address, type, symbolic destination, and addend for every retained
body.

## Provenance and signatures

- Baseline `source/cache/xbox_sound_cache.c` blob:
  `216035dcc62a79801b624e9a2758937558965fd3`.
- Retained source blob: `0c77a3a6c998d196d2d8fd119bc5b34efa62d6ec`.
- Retained source SHA-256:
  `7096d69d3ebafb788dbfdb7829e03172f8264dd721cc03893b7179bd0f059b2f`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `xbox_sound_cache.obj` SHA-256:
  `d023f0f71d3cc6592096ed895c00f69d137fe5c715b1abb8826ae24c0f5f568f`.
- The raw candidate-object hash is deliberately omitted because the XDK COFF
  container timestamp changes across forced builds. The normalized function
  hashes above are the reproducible admission identities.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The six screened donor
  source blobs are `1b6db3a20ac8dffa9cf98a8b8df026cfdea02e28`
  (delete), `f1305b24286cbd8be8dbfe086db4ce3a560fc2c9` (open),
  `b249bbaccc75af9858fd84859fe026ad4c49c349` (idle),
  `9984b404f2ae87fad667658de466c6893542978b` (hardware lock),
  `ff9b3e21758a43703f31c90d9e48ecef2cf296d6` (hardware unlock), and
  `b0ee4a0baf285b62a01f0fd9fa62fa7acbe60338` (close). Its globals,
  cache-datum, and sound-permutation layout-header blobs are
  `5fc877f9ecfd0a4c376e9b64b1c468ce0508679d`,
  `6300db55431df19c3ebbf447c6db3a5b48cfff9f`, and
  `ff374dc34732c4861169e493edf85e75a49d27a5`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and include paths.

HCEA supplies the readable typed behavior and these public signatures:

```c
void sound_cache_delete(void);
void sound_cache_open(void);
void sound_cache_idle(void);
void sound_cache_sound_hardware_lock(struct sound_permutation *sound);
void sound_cache_sound_hardware_unlock(struct sound_permutation *sound);
void sound_cache_close(void);
```

January PC COFF, disassembly, relocations, and callers remain authoritative
for the Xbox field identities, byte widths, call order, and exact admission.

## Typed Xbox layout and behavior

The target `_sound_cache_new` creates a 0x0C-byte, 512-element `xbox sound`
datum array and its assertion text independently names the target globals.
The translation unit therefore declares only an extern-backed prefix with
`cache_sounds` at `+0x100`, `base_address` at `+0x104`, and the LRU cache at
`+0x108`. Compile-time checks fix all three offsets and the 0x10C prefix
extent. `_bss_004d1088` remains undefined in the candidate; the target retains
the complete 0x110-byte storage owner.

The January request path increments datum byte `+0x04` as the software
reference count. The functions named hardware lock/unlock exclusively update
byte `+0x05`, and the target delete path tests `+0x04` and `+0x05`
independently. This proves the Xbox hardware-reference field at `+0x05`
despite HCEA's cross-build field at `+0x06`. The close loop loads the pointer
at datum `+0x08` and passes it directly to `sound_cache_sound_delete`; the
callee consumes the sound-permutation fields, and HCEA independently names
the datum member `sound`. Compile-time checks fix both consumed datum offsets
and the 0x0C datum extent.

The repository's already typed `sound_permutation.unknown0` member is at
`+0x2C`. January sound-cache bodies and HCEA independently identify this
member as `cache_block_index`; the source records that evidence locally and
does not edit the shared header. The retained lock resolves the datum with
ordinary `datum_get`, increments the byte below `UNSIGNED_CHAR_MAX`, and
increments the existing short `assertion_count` only on overflow.

Delete disposes the sound data array, deletes the LRU cache, then clears the
base address. Open makes the array valid. Idle services the LRU cache and
retains the January line-148 fatal assertion that the short global assertion
count is zero. Close uses the ordinary 0x10-byte `data_iterator`, deletes each
typed sound pointer, then invalidates the array. Every void body has an
explicit return.

## Fail-closed omission and ownership boundary

The first candidate `_sound_cache_sound_hardware_unlock` had the target's
48-byte padded extent and the same three relocation destinations and offsets,
but its normalized SHA-256 was
`cc8b42295bd02556f29d2e0665d6bdd38037d8d574126ede11addbe5ed4150c2`
rather than target
`82a5dae842a13d9586fb9b94da9ce18faf476589dc9d487237f36426e37b47f8`.
Its body is absent from final source. No alternate branch spelling,
declaration shuffle, or code-generation adjustment was attempted.

The other twelve target functions were never added. Only
`source/cache/xbox_sound_cache.c` and this new Jonas ledger change. No shared
header, configuration, semantic exception, parked record, completion label,
frozen source, or existing Markdown is edited.

The candidate defines no `.bss`, `.data`, COMMON symbol, or aggregate owner.
The two compiler-owned assertion-string COMDATs required by the exact idle
body are not granted data credit; all 1,224 target non-code bytes remain
unclaimed. The retained source is readable typed defined C with no assembly,
raw address access, pointer arithmetic, cast-based object access, union pun,
inactive-union read, undefined behavior, volatile scheduling device,
force-inline annotation, optimizer pragma or barrier, synthetic anchor, or
byte-forcing expression.

## Validation

- The one natural candidate compile retained five bodies; the rejected body
  was removed without another candidate experiment.
- Complete 573-action Halo/libcmt build and generated semantic reports: pass.
- Direct hardened comparison: all five retained functions are exact and the
  rejected hardware-unlock symbol is absent.
- `ninja progress`: pass. The semantic audit scans 470 units and 4,040
  functions, reports 3,900 semantic exact, 101 hidden exact / 61,876 hidden
  code bytes, 3,961 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,948/11,060 functions,
  475,204/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,781/7,574 functions and
  462,290/1,770,166 code bytes.
- Object admission, parked validation, the complete 179-test tooling suite,
  `git diff --check`, policy-token, deleted-path, and frozen-scope audits:
  pass.
- The pre-wave translation-unit comparison identifies exactly the five
  retained additions as `NEWLY_EXACT`, reports `changed_nonexact: []`, and
  shows only the expected symbol/debug and assertion-literal section changes.
- Clean committed-state snapshot, deletion-plus-forced single-unit rebuild,
  and regression check: pass. All five accepted functions remain exact, with
  no changed nonexact sibling, failure, or warning.

Nothing is pushed.
