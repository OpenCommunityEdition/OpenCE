# `trees.obj` Codex strict closeout

This Codex-owned ledger records the hardened reconstruction of
`source/memory/zlib/trees.obj`. The object remains `Matching`, now with direct
strict proof for every target function and every target-owned non-code
section. No pre-existing or Claude-authored Markdown was changed.

## Scope and donor census

- Integration base: `1c4ca0b90558440cf6da8ed283be8706a7dc3cae`.
- All 115 registered Claude/Jonas worktrees were checked for this translation
  unit. None had an uncommitted `trees.c` or `trees.h` delta.
- The only later committed source form was Jonas commit
  `45f8d684d520d33fef128c2448d184ca9294bb29`, inherited by the recent Claude
  branches. It recovers ten upstream zlib 1.1.3 private names: `send_bits`,
  `tr_static_init`, `init_block`, `pqdownheap`, `scan_tree`, `send_tree`,
  `set_data_type`, `bi_reverse`, `bi_flush`, and `bi_windup`.
- The retained change adapts that naming evidence to the current integration
  source and uses typed table aggregates to reproduce the January object's
  data-symbol ownership. No donor configuration or Markdown was imported.

## Hardened baseline and strict delta

The untouched integration source rebuilt with all 5,712 padded target code
bytes already byte-identical. Hardened comparison nevertheless accepted only
3/22 functions (`_code_00105dd0`, `__tr_tally`, and `_code_00106af0`), totaling
1,076 meaningful bytes, 1,104 padded bytes, and 27 relocations. The other 19
functions either had no same-name candidate symbol or referred to a private
callee through an address-derived name, so their relocation destinations
correctly failed closed.

After restoring the private zlib names, all 22/22 functions are strict exact:
5,511 meaningful bytes, 5,712 padded bytes, and 200 relocations. The strict
gain is 19 functions, 4,435 meaningful bytes, 4,608 padded bytes, and 173
relocations. All three baseline-exact functions are unchanged.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_send_bits` | 186 | 192 | 6 | `537ea1eafb37a40407b36ceb7a574d48cc05c3ee019203c28b3c99ba5a43ea72` |
| `_tr_static_init` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_init_block` | 100 | 112 | 0 | `0469e8666f29c844ce858363e08afeefb58ebb2d98fc808a1af82b1c47cf37aa` |
| `_pqdownheap` | 210 | 224 | 0 | `a90cac734e1745382132fce1797c8d9712393eb8e7237196c54994b2c1469efe` |
| `_code_00105dd0` | 563 | 576 | 8 | `c0e8d3b754538b7058145ce534cd9384ad729b2f562669773b8d006a10a383f7` |
| `_scan_tree` | 225 | 240 | 0 | `bebbe7f5118376ccd73748687c07210e07cb61467dd00d2f3cc3b2d11af1ee28` |
| `_send_tree` | 543 | 544 | 27 | `7c4b13aa2f718f562fd7a55fc0b5c81706a8f7bf3090aec7f2eb666d9f25e23b` |
| `_code_00106320` | 523 | 528 | 35 | `2e772f06f8062f2def93c789c4c84056b275c7306820a66279585ed40cbacb0e` |
| `__tr_tally` | 234 | 240 | 7 | `acdae421da92e1dccf32ea24641545c6feb1e73ca7de4f87ddb9f2e07586c370` |
| `_code_00106620` | 503 | 512 | 37 | `53579cc36f2aae92b43b32f2f1a1f6355224bbc3f35892e9b3381a486335533d` |
| `_set_data_type` | 129 | 144 | 0 | `f563d95871b5d675534d53bd4665ea2077728e44ee4edaa10b476b7377373427` |
| `_bi_reverse` | 23 | 32 | 0 | `7e0b053566f28ed47878c5e1758d81ef41bea081509892f74c1603f3842a055a` |
| `_bi_flush` | 127 | 128 | 0 | `5e9e24169bba81ed019e72d4f601572eefe4e7eb4dac74f86fe2c71d709aca1b` |
| `_bi_windup` | 108 | 112 | 0 | `02880f72ca316399221dd103b899fc478f527f1e78334ab923b78c23ee06d15e` |
| `_code_001069c0` | 168 | 176 | 1 | `13e020de757ba6d122e719f3b2f21384293c6ba821d3ac86502dbd2d881078e9` |
| `__tr_init` | 115 | 128 | 4 | `afdb039153e2058d6495339f3e4e7bde141b795a705fab04d4aff99b43d4a920` |
| `_code_00106af0` | 279 | 288 | 12 | `79a3a5f276bb35deb85f37d2b1c33a1d158c88eaa0b243e363759b926ad7a940` |
| `_code_00106c10` | 527 | 528 | 5 | `6f13530a5b94c0ed35e5de1289532477d40e0ef9fbd452bf7fda3e0d221743cf` |
| `_code_00106e20` | 146 | 160 | 8 | `2a37b905969edc381277389f0f919496fe53e1ba7446f9d4ec3d9bd186725021` |
| `__tr_stored_block` | 65 | 80 | 2 | `27afd3acfd5f3ae68e45cdc7c75183fee3ae53f2b60daccf1624c64f1d728ba3` |
| `__tr_align` | 198 | 208 | 14 | `3884036db9b259adf743716123d708af9d0cb787181abbab2540907b692f2134` |
| `__tr_flush_block` | 538 | 544 | 34 | `d4903b950cea3656819635346c53e161706976af5b5c8165f19976a1bc924870` |

## Data and ownership closeout

The target owns 29 non-code sections totaling 3,203 logical bytes. All 29 are
strict exact by ownership identity, logical size, flags, alignment, COMDAT
selection, normalized bytes, relocations, and complete symbol inventory.

The two aggregate sections are:

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rdata_0027c170` | 2,616 | 0 | `32bc7cbf8cc26537f4221726b893657f706833d65348075f217141ae9e2ddf2a` |
| `_data_00308b54` | 60 | 5 | `c63f9f44093d5f869420693610142c0a9d016cc2c42d366de48d01c8a7f3554f` |

The remaining 27 sections are exact assertion/trace string COMDATs totaling
527 bytes. At baseline, all payloads and relocation destinations were already
equal, but compiler-local labels inside the two aggregate sections made their
complete ownership fingerprints nonexact. The retained typed structures make
the static Huffman tables, length tables, and three descriptors explicit
subobjects of the target external owners, eliminating those extra labels
without raw-byte construction or aliasing tricks.

The candidate additionally emits ordinary compiler metadata only: one
`.drectve`, one `.debug$S`, and eight `.debug$F` sections. They are not target
ownership and do not replace any target-owned section.

## Source quality and configuration

The recovered private definitions use typed parameter lists. The no-argument
definition spells `void`, each retained private parameter is on its own line,
and every void function in this translation unit ends with an explicit
`return;`. The source contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, undefined type-punning, or
byte-forcing construct.

The translation unit explicitly undefines the repository-wide `MAX` macro
before restoring zlib's local `MAX` definition. This preserves the upstream
expression used by the strict build while avoiding a macro-redefinition
warning when the integrated header graph exposes `cseries.h` first.

Because code and target-owned data/ownership are complete, the existing
`Matching` status remains honest. No semantic exception, parked entry, or
configuration edit is needed. This hardens evidence without changing the
campaign's already-credited complete-object/function/byte totals.

## Reproducibility and gates

- Target object SHA-256:
  `763bfe6690fe6beff0897f0128880e2bbc5520e538e3568c2124ad6202aaed4b`.
- Rebuilt object SHA-256:
  `570c666b2bf3963c0af06abda25a4dea8765d15eaeeefb715fb9589aae35c3f6`.
- `trees.c` SHA-256:
  `235bd0c08a7705e7b27af5171e667488aaefc9bf98e8e8a2389cbb76ca3e2f3a`.
- `trees.h` SHA-256:
  `b4ca57bae95fd4cfdcd43b94f72d568a279df6c02ef85a60597e6fc2b709667b`.
- `deflate.h` SHA-256:
  `b82027ce710499b8c2abe150451c4aa736798a978fe049a1317aa2e52c56a9f6`.
- Full Halo and libcmt build graphs passed.
- Semantic audit: 470 units, 3,699 functions evaluated, 3,566 semantic exact,
  3,630 accepted exact, and zero unit errors.
- Campaign progress remains 374/833 complete objects, 3,624/11,060 exact
  functions, 438,106/2,198,102 exact code bytes, and
  1,802,644/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A clean post-commit snapshot and forced-object regression check are recorded
  in `build/regression_manifest_trees_final.json`.

The five user-frozen large objects are untouched, no tracked file is deleted,
and no Claude-authored Markdown is changed.
