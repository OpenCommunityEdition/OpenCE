# `draw_string.obj` Jonas HCEA one-shot — 2026-08-28

## Result

This bounded source-only wave starts from integration commit
`b1709807eb7aeda1f07e7e3c3f0d5e31e9b437d2`. It maps the sixteen residual
functions in `source/text/draw_string.c` from January call topology and the
readable HCEA corpus, then emits one natural typed-C batch without local
code-generation tuning. One function is strict exact and retained:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_draw_string_pick` | 80 | 7 | `35dca73f36d573e8e849cbba18b601fd57651369d043a266482e2516988a7089` |

The twelve inherited exact sentinels remain strict exact. `draw_string.obj`
advances from 12/28 to 13/28 exact functions and from 1,072/7,376 to
1,152/7,376 padded code bytes. It remains `NonMatching`, with fifteen
functions and 6,224 padded bytes unwritten.

The retained source Git blob is
`5940db49984e98c9fb42bd8ca891575581dd65ba`; its payload SHA-256 is
`6559c6cf8cc3b497e8062f7aa5d475fccff0a315d1948ad9f9cf1e1e44e935d2`.
The January split-object SHA-256 is
`17b4f9639bdfdf21b4e2eaa00fd8f51be6af3ebd23376ccfbbb39570bb05c11e`.

## Provenance and anonymous-function map

Before emission, the current source and headers, their complete reachable Git
history, every current/historical repository `CLAUDE.md` and `AGENTS.md`, and
all existing object-matching logs were audited. There was no earlier dedicated
`draw_string.obj` ledger. Preserved Pastudan and Stian PC source was used only
as an independent topology/disassembly cross-check because its raw-address,
`volatile`, and assembly fragments are not admissible production source.

The primary semantic oracle is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`. The requested donor set and the
adjacent helper donors needed to map anonymous January sections are:

| HCEA donor | Git blob | Payload SHA-256 |
| --- | --- | --- |
| `src/blam/text/draw_string.c` | `346c5104ec98791b80a5a8333b632749a1d02139` | `0c8808a13e51f70b2addf3129a6ab5cb198384f3ca9c3db495115d59bb9ba278` |
| `src/blam/text/draw_string_partial.c` | `dbebb882441676f6c38a45fa8e63747e269c6134` | `fd16e5a7aeb58f325f63e6ebb3e4effff99428fc7ee8573a0e51a58d1deee514` |
| `src/blam/text/draw_string_and_hack_in_icons.c` | `8dcf0970c261b78f46b095f2cff820b459d03b56` | `c60b20427b860910bfa5d981a4c808c53297a6ca47387ae13a2844c21ebc7bda` |
| `src/blam/text/draw_string_compute_bounds.c` | `2fab013064ba489ea6adc83ce190c6e47ebcc494` | `08a7d9c1cd23f5c406855a0620791e1115a7716231b1ea6f4627edeeb2f36e37` |
| `src/blam/text/draw_string_pick.c` | `57de8b19595f14b9009541ef9060255260206a82` | `c9711f29ea6b064b950e9d212c32ac2c1bfa58c444dd744915a3e65339e51bdc` |
| `src/blam/text/draw_string_set_tab_stops.c` | `c128d2674bc2b5ac5b6c97b9c8f2d1a8dbd707f2` | `d57fa81c1fbdb21fbc6c98b2e41a083243d9153941c1702de6e28238ff4db3b8` |
| `src/blam/bitmaps/bitmap_draw_string.c` | `77fd26cc7952ce62312794c33ddcf1d6b8c88c34` | `e798a2c9ec05f74a478956ac8daf87a6f27fdcebef3e60aea2431ce22eb3b1e0` |
| `src/blam/text/draw_unicode_string.c` | `b0f5949389a1a1349fd572a6c1565261154e0cdc` | `5fa165152452bd5d4f9dc4a82e94d793656f24c2fe4cdb81231515891dc95142` |
| `src/blam/text/draw_unicode_string_partial.c` | `172e4d91d232d2df0fcad418d8f3dd0bf04fb15e` | `2be0b43b34b088106bbe965caa837f8534106c27b36abfbf0a9c97410c7172a7` |
| `src/blam/text/draw_unicode_string_compute_bounds.c` | `671cffee987d2c9f0afb9267ca04dfab5ffe660f` | `672073c84d137625f342be65d9359461259db6d83509c8f870108524f43c334a` |
| `src/blam/text/text_bounds_draw_character.c` | `352b54274cd1d089e1b4acfb28cf96f2acf7628a` | `640711b4d1e254156e26ea0177d51141c447ddcc7488b88e1ca4130d6bab0696` |
| `src/blam/text/text_pick_draw_character.c` | `254523e02a718c31e92607bd71ff4595f765b105` | `64fb8779fd7f6f38056883f2a6c8c9b1740e9ad9dc48ac8ae952b647e80b5e7d` |
| `src/blam/text/parse_string_new.c` | `edc21a66e58050502270780993f3e2f5739b3074` | `a6915730e6a34b6b1a653a6f5636e28b2ecfcc8a8a4e45db2c77069b44df9262` |
| `src/blam/text/parse_string.c` | `5db8b02de7773f0ec90cf149b7db65b4ff34c339` | `b9b9ae1675c72313abbfc17b17eb09177a178afd603963c2569b29cd18ee398b` |
| `src/blam/text/parse_unicode_string.c` | `0c457c0a90be1fccb1132422c3059f100e0be3c9` | `09702d70b70ab016a7e4173644bb31e7bfe282f0caea99cf4efe72f065fa57ec` |
| `src/blam/text/styled_font_get.c` | `09b8d32fe3edf65db6f8e8e59adccb557a25a6ff` | `d0837dca1c7f9d7d15e656588455114ed2ef28b9fe9e422efc8fc5639f3631b5` |
| `src/blam/bitmaps/bitmap_draw_character.c` | `9e1210a281b83dbb357eea926b90d51cc64af76f` | `81f04293e14791d86a9afdcd2a8c8ae3f79a10248f8d60841e6d381da415595e` |

January relocation and call topology gives the following stable anonymous map:

| January symbol | HCEA semantic body |
| --- | --- |
| `_code_0018ac10` | `text_bounds_draw_character` |
| `_code_0018ac80` | `text_pick_draw_character` |
| `_code_0018b160` | `bitmap_draw_character` |
| `_code_0018b510` | `styled_font_get` |
| `_code_0018b580` | `parse_string_new` |
| `_code_0018b680` | `parse_string` |
| `_code_0018b8f0` | `parse_unicode_string` |
| `_code_0018ba00` | `draw_string_partial` |
| `_code_0018bc10` | `draw_unicode_string_partial` |

The accepted `_draw_string_pick` body initializes the existing BSS-owned pick
point, distance, best index, and last index, then invokes `_draw_string` with
the mapped `_code_0018ac80` callback. Its ordered relocation identities are
strictly equal to January: BSS addends 12, 16, 8, 14, and 12 at offsets 19,
25, 40, 47, and 60; `_code_0018ac80` at offset 34; and `_draw_string` at
offset 54.

## Frozen one-shot candidate

The only code-producing batch compiler input is preserved as
`scratch/_gate_28180.c`, Git blob
`9751b55a78a6190c5617cca58d108b76635daacf`, payload SHA-256
`1aa2c11eb0011e21003fa44017bfa36213265311a92033ba43d66043f1555019`.
Its object is `scratch/_gate_28180.obj`, raw SHA-256
`dd8070de786febd14fe6c861a43c156608ff7604db007d46f82fb1c4ffae9360`.
The gate reported 13 exact, six residual, and nine unwritten functions. The
full January/candidate evidence is frozen below; `not emitted` means that the
batch object contains no section for that function, not that an all-zero stub
was compared.

| Function | January bytes / relocs / normalized SHA-256 | Candidate bytes / relocs / normalized SHA-256 | Decision |
| --- | --- | --- | --- |
| `_bitmap_draw_string` | 304 / 9 / `4e8747543c903cdbcfac7e2ebca7b5ed667af293165012efd03b1262c00fe2aa` | not emitted | omitted: donor's null-bounds path requires forbidden absolute-VA-zero reads |
| `_code_0018ac10` | 112 / 10 / `8497c0497bf3d1c1f59bb22c46ddb237ab9338a5f37d0b97114eba3c10e92c6e` | 112 / 10 / `c3167bdbc49b82da6f5511137d759477b40fed134c5c5926c3f813108258291b` | normalized-byte residual |
| `_code_0018ac80` | 224 / 11 / `a0b5c3c509e22da9390fc71d49e8296440382e1d4b706b5762c320a7a779a7c8` | 224 / 11 / `3303e9e102ea8c8202165866a30684e35f826109cbd1b3937c9b97f10633988f` | normalized-byte residual |
| `_code_0018b160` | 944 / 25 / `1fbab7db0314ed562e1e6908a77d5570fd513dbc8a3e27e261710ed051f3682a` | 912 / 23 / `452ff3905a294856144d2a5f7c6e08eced577d8876ac2a4c5966ee3694f3c3cd` | size, relocation-count, and byte residual |
| `_code_0018b510` | 112 / 6 / `85f0a63276b854d6854915b2099aacdcd0ba5741d1366b55f4e7cf2c2fa9fc81` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_code_0018b580` | 256 / 17 / `e257eb07009cc417d63fadc3f316a2a9a7ab537df9f77409c22e2f2ac7e8d60e` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_code_0018b680` | 624 / 30 / `f80225b7f49eeafe4a68d6094b4ec238747e5af312947588c11ab8529dfcb304` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_code_0018b8f0` | 272 / 7 / `933bc5d77f06ac0cdd924f9e0b97eb09f817bf5250929087f0329c80ac8f8219` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_code_0018ba00` | 528 / 9 / `e55f2a4aa9ebcd849324eb7c083e8a2f03fee150ddddf12284fa28d2c5e11c5a` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_code_0018bc10` | 528 / 9 / `a5bfb710d5d9d7d0231206c83cfd1f931e2aab1bd0188203fde7c9c0e0ecce07` | not emitted | internal helper dead-stripped without a retained top-level caller |
| `_draw_string` | 912 / 46 / `8a16ea28aab4bb4839d80221e1c7b793a2180e9a3ec7c50a0b6d2c6a008c48d3` | not emitted | deliberately omitted from this bounded batch |
| `_draw_string_compute_bounds` | 192 / 14 / `1546e737cb0c9365d2894beae56d2890e6af2028be8d00e26b8e0bab67264628` | 240 / 15 / `31067f971eaaab2cc12d973b5ab9f88dc5d1114345eac1897d8e01afb42c9614` | size, relocation-count, and byte residual |
| `_draw_string_pick` | 80 / 7 / `35dca73f36d573e8e849cbba18b601fd57651369d043a266482e2516988a7089` | 80 / 7 / same normalized hash | **strict exact; retained** |
| `_draw_string_set_tab_stops` | 112 / 8 / `093c317d014b2d059db64033de044e6fc0853997564a94f6fa3458b3500e30e6` | 112 / 7 / `d266e60a74db46ccbe29a4ecf8b2e67fd7aade5e2fc0fa5bf0f43ecafb1e2ddd` | relocation-count and byte residual |
| `_draw_unicode_string` | 912 / 46 / `a9959a0469b03fcbff5ec70bf6749e20a761755c4e6e192fec80f5dca8e28e70` | not emitted | deliberately omitted from this bounded batch |
| `_draw_unicode_string_compute_bounds` | 192 / 14 / `1546e737cb0c9365d2894beae56d2890e6af2028be8d00e26b8e0bab67264628` | 240 / 15 / `31067f971eaaab2cc12d973b5ab9f88dc5d1114345eac1897d8e01afb42c9614` | size, relocation-count, and byte residual |

All six code-producing rejects and all dead-stripped helper transcriptions were
removed from production source immediately after the first gate. There was no
spelling, declaration, branch, scope, or compiler-control tuning and no second
code-producing candidate. Reopen the six measured rejects only with new
authentic January source/debug-scope evidence. The dead-stripped parser and
partial helpers were not measured candidates; screen them only in a new
bounded wave that also has an authentic source-backed top-level caller
topology. Do not reproduce the forbidden absolute-address behavior in
`_bitmap_draw_string`.

## Retained source, ownership, and validation

The retained diff is limited to 55 insertions and one deletion in
`source/text/draw_string.c`: it includes `math/integer_math.h`, gives the
already-owned first 0x20 bytes of `_bss_004c1908` typed pick fields while
preserving every offset and the aggregate size, adds forward declarations and
typed prototypes for the two still-unwritten callees, and adds the exact
`draw_string_pick` body. It creates no new storage owner and changes no shared
header.

The retained-only compiler input is `scratch/_gate_42500.c`, identical to the
retained source blob. The replay object `scratch/_gate_42500.obj` has raw
SHA-256
`b5af8c980b4aade4419bf990c1dac739f8e7bbdb30b47480efeec684132da6df`.
The exact replay command was:

```text
python tools\campaign\gate.py source/text/draw_string --all
```

It reported:

```text
exact 13  residual 0  unwritten 15
```

The retained source is ordinary typed C. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, optimizer barrier, raw address or byte-offset
access, pointer reconstruction, punning, undefined behavior, synthetic anchor,
forced emission, byte forcing, comparator exception, configuration change, or
alternate compiler flag. `git diff --check` is clean, the policy scan has no
findings, and every inherited exact sentinel remains exact.

Only `source/text/draw_string.c` and this Jonas-owned ledger are intended
tracked changes; the frozen gate inputs and objects remain untracked under
`scratch/`. Units, Vehicles, Claude-active source, shared headers, storage
ownership, build configuration, matching policy, parked records, comparator
code, and object status labels are untouched. No commit or push was performed.
