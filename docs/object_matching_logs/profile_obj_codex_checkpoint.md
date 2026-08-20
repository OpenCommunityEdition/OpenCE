# `profile.obj` Codex checkpoint

This Codex-owned ledger records the bounded, header-free recovery of
`source/cseries/profile.obj`. The unit remains `NonMatching`: 30 functions
and most initialized data are still open. No pre-existing or Claude-authored
Markdown was edited or deleted.

## Scope and donor boundary

- Integration base: `664ce784eae6bbeded627097d9627fcc11091328`.
- Safety-correction base: `472fe0e0b272d6da7e7c289a97ac80fca56c0b0a`.
- The historical donor chain runs from `be2e5bc2` through `3c2a9168`; its
  final `profile.c` blob is `50f0976a1c647b9137e4bc798877453322f9c14c`.
- The donor measured 30/44 exact functions. Thirteen of those bodies depend
  on the donor's inline `rdtsc` assembly macro and were excluded. The
  section-history helpers `_code_0007dd20` and `_find_profile_section` were
  also excluded because their typed implementation requires broadening the
  shared `profile_section` layout in `profile.h`.
- The retained frame, timer, iterator, and global layouts are private to
  `profile.c`. There is no `profile.h`, render caller, shared-header,
  configuration, semantic-ledger, or park delta.
- The user-frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`
  objects are outside this lane and untouched.

## Strict code result

The untouched base had 0/44 strict-exact functions and zero credited code
bytes. A forced candidate build accepts 14/44 functions totaling 1,117
meaningful bytes, 1,232 padded bytes, and 85 relocations. Every accepted body
is ordinary objdiff exact and independently semantic-COFF exact; no exception
ledger is used.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_profile_initialize` | 120 | 128 | 14 | `36a04f11c4190f78706ef5efa96cd67818c2e3a448e03eee6c21be3a54b4403c` |
| `_profile_rasterizer_stats` | 22 | 32 | 2 | `d2a88f26be45877e903a87316d6f952be36c8f04e11a7facf5062f7e448856e4` |
| `_profile_seconds_elapsed` | 13 | 16 | 1 | `3307464d0a4c3704ea054046b1a71de6ed4ab1c8c397d82e01d96e9777ce0123` |
| `_profile_lapsed_frames` | 62 | 64 | 5 | `ca4109f0a47a4e1615b5acbe7ce2395ae95093eaf4623b63aa4322c81e28ad4f` |
| `_profile_lapsed_msec` | 23 | 32 | 2 | `5bf0f905711f7dd660ef2cb05c51d2397e521d52bac526bbe8720e3ea8a18052` |
| `_code_0007ee30` | 158 | 160 | 6 | `c66e82fa92c13d13181e344026e596f60a55b1140e86644acad55433ad598e00` |
| `_profile_sections_activate` | 20 | 32 | 1 | `e7f3fd1fde128eb17a46853feb1d41f3d7c7cc1aa969ce290690f1cf964ba22e` |
| `_profile_sections_deactivate` | 20 | 32 | 1 | `f474d33b63b8b8a0bd493b57aa9c2ff259c18455d50cc748005b7d066869368b` |
| `_profile_find_game_value` | 62 | 64 | 4 | `0ec982b2f6b7608c4f9c184d4b40bdada7ab8e01cbe347345fbf02a564b4720a` |
| `_profile_frame_iterator_new` | 81 | 96 | 5 | `1aea8b55afa0d28e715b266efe08ca74f9f7cae0be9faf817d3160359c1d1676` |
| `_profile_frame_iterator_next` | 115 | 128 | 4 | `aa9283d20fee8a1b68fb2cfe9c18a9c0ee18e126fda57f2a414dc69009a71dd3` |
| `_profile_frame_get_messages` | 139 | 144 | 14 | `37014e7926aa92604c2941c07048a93f67c24f6130833906c31271b712309b31` |
| `_profile_frame_get_stalls` | 150 | 160 | 11 | `83423f3a3dca3dca554517e99af0c77514e603776046f06af5a7d2f32782c93d` |
| `_profile_rasterizer_stalls` | 132 | 144 | 15 | `78d28326869a333768f8f1cdfe9126251575b2c3e92bfd0dd845d4197598e848` |

Thirteen exact public functions have matching whole-symbol function type and
external storage. The private `_code_0007ee30` code section is exact, but the
target split symbol has external storage class 2 while the legal source-static
definition has class 3. This checkpoint does not claim whole-symbol ownership
for that helper.

`profile_dump_to_file` remains a necessary public body because `hs.c` calls
it. The target-shaped form called `fclose` even when `fopen` returned `NULL`.
The retained implementation closes the stream only in the successful-open
branch, so it is ordinary defined C but deliberately receives zero strict
credit: it is 128 padded bytes with nine relocations and normalized SHA-256
`c1c2e7ef1e9547f36d70b457fe9f1201fbd3cb5c4f4f6cebb277fd009dafaa21`
rather than the target hash.

One bounded advisory test moved the `profile_frame_get_stalls` range assertions
ahead of forming the frame pointer. That changed the normalized function hash
to `305f28e627e0d6e72a83a17ff25535dd1fd2c52187ee6ded07d46d0df6807dd0`,
so the experiment was reverted. The exact body retains the original caller
precondition that the iterator and its current index are valid on entry; its
assertions reproduce the target diagnostics and are not a safety guard.

## Strict data and ownership boundary

The target reports 1,131,932 data bytes: the 1,129,812-byte BSS aggregate is
byte-exact at both the untouched base and candidate, while 2,120 aligned bytes
of initialized/RDATA ownership remain uncredited by ordinary progress. The
candidate adds seven independently strict target-owned RDATA COMDATs totaling
285 logical bytes and zero relocations:

| Exact payload | Logical bytes |
| --- | ---: |
| `*` | 2 |
| `%s\r\n` | 5 |
| `d:\\profile.txt` | 15 |
| game-value argument assertion | 32 |
| January source path | 33 |
| frame-history current-index assertion | 78 |
| frame-history range assertion | 120 |

The untouched base owned none of those seven complete sections. The private
`profile_globals` BSS owner has the same target layout and zero payload, but
the target split symbol uses storage class 2 while the natural source-static
owner uses class 3; whole-symbol ownership is therefore not claimed for that
aggregate. Candidate/target symbol inventories are 75/298 entries. Extra
compiler/debug/directive sections receive no target runtime credit.

## Source quality and gates

The retained code is readable typed C. It uses no assembly, volatile
scheduling device, forced inline, optimizer pragma/barrier, undefined
type-punning, byte forcing, or repeated raw layout cast. Parameters follow the
one-per-line convention, no-argument functions use the project layout, and
void functions end with explicit `return;`.

The full Halo and libcmt build graphs pass. Semantic audit reports 470 units,
3,865 functions evaluated, 3,724 semantic exact, 3,788 accepted exact, and
zero unit errors. Campaign progress is 375/833 complete objects,
3,777/11,060 exact functions, 460,855/2,198,102 exact code bytes, and
1,803,160/4,176,062 matched data bytes. Admission reports zero candidates and
zero revocations, with only the inherited `source/shell/shell_xbox`
contradiction. Parked audit reports three active, zero stale, and zero
invalid. The complete tool suite passes 179/179 tests.

The unit remains `NonMatching`; no GitHub push is performed.
