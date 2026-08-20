# `files_windows.obj` Codex checkpoint

This Codex-owned ledger records the bounded recovery of
`source/tag_files/files_windows.obj`. The unit remains `NonMatching`: seven
functions, the 624-byte file-search aggregate, and eleven residual strings are
still absent. No pre-existing or Claude-authored Markdown was edited or
deleted.

## Scope and donor

- Integration base: `9584608e7d09cbe8cb126fb73886eba4740b585e`.
- The source-only donor chain is `7dfc5550`, `20cd95ea`, `9e7a38c0`,
  `93c8cf48`, and `62466e86`. Its final source blob is
  `62c2b361e6707cd64c39440aa57479c553542010`.
- Only `source/tag_files/files_windows.c` is implemented. No shared header,
  configuration, semantic ledger, or park entry changes.
- The user-frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`
  objects are outside this lane and untouched.

## Strict code result

The untouched base had 8/27 strict-exact functions, 580 meaningful bytes,
640 padded bytes, and 29 relocations. The forced donor build has 20/27
strict-exact functions, 2,217 meaningful bytes, 2,368 padded bytes, and 131
relocations. The strict gain is therefore 12 functions, 1,637 meaningful
bytes, 1,728 padded bytes, and 102 relocations. All eight baseline-exact
functions remain exact.

| Newly exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_00189ca0` | 54 | 64 | 5 | `2eaf2ef2e40e300a0d5a28a93034b9a08e23442973c8ed6d6d195337ee937cfc` |
| `_file_close` | 110 | 112 | 8 | `26e68877c869dc34a50ad4bb69a7a405d7519ad221483db669f76032a45708b2` |
| `_file_create` | 198 | 208 | 11 | `c22e9fd564daa86108ee8e050a98e229821987b39ea870e4c2d35f2ff5a6677a` |
| `_file_delete` | 209 | 224 | 11 | `d66c78171570e3175824965f394f709a3724522a598fe71875c5ed2a0f40c1e2` |
| `_file_exists` | 143 | 144 | 7 | `32bffecc4b13164a902ca8b124c4b5d35a93e5437d16b7f956681e165e9aaa54` |
| `_file_get_eof` | 92 | 96 | 8 | `86112ad5154f2a4c664f21a075b04b2c878df0aadb20af10c21b4c3af715c605` |
| `_file_get_last_modification_date` | 190 | 192 | 11 | `ead1799722db9f2fab1e5a2b114f3e9b9ee952ce5ccbbaa228396fef51db9be4` |
| `_file_get_position` | 96 | 96 | 8 | `6889a379aad45e3032f56a996e0d10a94a23c9265d812ce5702728fedc703dff` |
| `_file_location_get_full_path` | 130 | 144 | 8 | `6716ae42a80c92d4cb90e0c60c19307a0f4beca0fafe78e433af31793e45738c` |
| `_file_rename` | 201 | 208 | 8 | `4ad09ff6f79a04709ad187840864b5b4ed9dbbcd897ec518bee3e1821392296e` |
| `_file_set_eof` | 113 | 128 | 9 | `1d3bf10f787012c9e0085441f552cd8e535d108542b72a0744f322d2073ffe8a` |
| `_file_set_position` | 101 | 112 | 8 | `ad9eaeb718b98ec84d734d38b23b8b8b470191c95b5b2382d720424502ef7636` |

The seven residual functions are fail-closed and absent from the candidate:
`_find_files_start` (204/208 bytes, 17 relocations), `_file_path_split`
(213/224, 2), `_file_open` (391/400, 23), `_file_read` (168/176, 13),
`_file_write` (161/176, 12), `_file_get_size` (209/224, 13), and
`_find_files_next` (499/512, 49). No residual body is retained or credited.

Nineteen exact functions have matching whole-symbol type and storage. The
private `_code_00189ca0` section has exact bytes and relocations, but the
target split symbol has external storage class 2 while the legal source-static
definition has class 3. This checkpoint does not claim whole-symbol ownership
for that helper.

## Strict data and ownership boundary

The target owns 27 runtime non-code sections: a 624-byte DATA aggregate and
26 RDATA strings, totaling 1,299 logical bytes. The baseline owned 3 exact
strings totaling 152 bytes. The donor owns 15 exact strings totaling 337
bytes and zero relocations, a gain of 12 sections and 185 logical bytes.

| Exact payload | Bytes |
| --- | ---: |
| `d:\\` | 4 |
| `path && full_path` | 18 |
| `file_set_position` | 18 |
| `file_get_position` | 18 |
| shared file-error format | 22 |
| `file_get_last_modification_date` | 32 |
| January source path | 41 |
| path/name assertion | 53 |
| path/extension assertion | 58 |
| `file_close` | 11 |
| `file_delete` | 12 |
| `file_create` | 12 |
| `file_exists` | 12 |
| `file_get_eof` | 13 |
| `file_set_eof` | 13 |

The 962-byte open boundary is the 624-byte aggregate plus eleven strings
owned only by omitted residuals. Candidate debug/directive sections receive no
runtime credit. Three unchanged XDK `SELECT_ANY` constants total 432 bytes;
they are the same unreferenced duplicate COMDATs present at the baseline and
also receive no target ownership credit. Target/candidate symbol inventories
are 148/111 entries.

## Regression and gates

The baseline regression manifest was captured from a forced clean build at
`9584608e`. The donor makes twelve functions newly exact and adds their exact
strings and symbols. Three baseline callers now relocate to same-name local
definitions instead of unresolved externals: `_file_read_only` calls
`_file_location_get_full_path`, while `_file_read_from_position` and
`_file_write_to_position` call `_file_set_position`. Their code, padded size,
relocation addresses/types/destinations, and strict target equality are
unchanged. The other five baseline functions are unchanged under the complete
regression fingerprint. The expected additive data/symbol transition was
reviewed rather than waived; a new clean post-commit manifest freezes the
accepted checkpoint.

Full `halobetacache_build` and `libcmt_build` graphs pass. Semantic audit
reports 470 units, 3,775 functions evaluated, 3,641 semantic exact, 3,705
accepted exact, and zero unit errors. Campaign progress is 375/833 objects,
3,698/11,060 functions, 450,589/2,198,102 code bytes, and
1,803,124/4,176,062 data bytes. Admission reports zero candidates and zero
revocations, with only the inherited `source/shell/shell_xbox`
contradiction. Parked audit reports three active, zero stale, and zero
invalid. The complete tool suite passes 179/179 tests.

The retained source is ordinary typed C: no assembly, volatile scheduling
device, forced inline, optimizer pragma/barrier, undefined type-punning, raw
byte access, or byte forcing. Parameter formatting and explicit void returns
follow the project rules. The unit remains `NonMatching` and no GitHub push is
performed.
