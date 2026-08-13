# widgets.obj exact-match log

## Target and standard

- January 14, 2002 Xbox target, XDK 3911 CL 13.00.9254.1.
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`.
- Admission requires strict function bytes, relocation identity, target-owned data, symbol ownership, whole-project build, and regression gates.
- HCEA/PDB material is used only as a semantic and naming hypothesis. January disassembly and COFF metadata are authoritative.

## Recovered ownership and layouts

- Eleven functions, 1,632 padded code bytes.
- Seven target-owned string COMDATs, 239 bytes total.
- One external writable table, `_data_0030b2b0`, exactly 200 bytes and 37 DIR32 relocations.
- No target-owned BSS.
- `struct widget_type_definition` is 0x28 bytes: group tag, lighting byte, padding, then eight callback pointers.
- `struct widget_datum` is 0x0C bytes: datum header, type, type datum index, and next link.
- `struct object_definition_widget` is 0x20 bytes and begins with a tag reference.
- The five descriptor records are flag, antenna, glow, light volume, and lightning. Glow, light volume, and lightning have null update callbacks.

## Proven source architecture

- `_code_001257a0` is the private `widget_type_definition_get(short)` helper. It follows the exact `actor_types.c` pattern: a source-static helper retained out of line with a private register ABI and inlined into callers.
- Its exact assertion is from lowercase `c:\halo\source\objects\widgets\widget_types.h`, line 150.
- Direct widgets assertions use uppercase `c:\halo\SOURCE\objects\widgets\widgets.c` at lines 46, 55, 190, and 241.
- Typed `object_get`, `object_definition_get`, `TAG_BLOCK_GET_ELEMENT`, and `widget_get` accessors preserve Berth's type-access rules.

## Experiment 1: faithful January/HCEA topology

The first implementation reconstructed all functions and the table as readable C. Strict results:

| Function | Target/base size | Relocs | Result |
| --- | ---: | ---: | --- |
| `code_001257a0` | 64/64 | 5/5 | exact |
| `widgets_initialize` | 176/176 | 16/16 | exact |
| `widgets_initialize_for_new_map` | 96/96 | 7/7 | exact |
| `widgets_dispose_from_old_map` | 96/96 | 7/7 | exact |
| `widgets_dispose` | 80/80 | 5/5 | exact |
| `widgets_delete` | 256/256 | 14/14 | exact |
| `widgets_update` | 96/96 | 5/5 | exact |
| `tag_group_to_widget_type` | 48/48 | 1/1 | non-exact return-local shape |
| `widgets_new` | 384/368 | 15/15 | non-exact loop width/branch shape |
| `widgets_need_lighting` | 128/128 | 7/7 | non-exact result-live-range shape |
| `widgets_render` | 208/224 | 13/15 | duplicate `widget_get` in loop update |

Accepted lessons: the helper architecture, assertion spelling, lifecycle loops, delete topology, update loop, aggregate table, and callback prototypes are all proven exact.

Rejected/changed shapes:

- Direct early returns in `tag_group_to_widget_type` did not retain the target's separate `NONE` result register.
- A `long` outer widget-reference index omitted the target's `movsx` short-loop behavior.
- A `for` update expression that called `widget_get` repeated the datum lookup and added two forbidden relocations in `widgets_render`.
- Failure-first source ordering in `widgets_new` inverted the target branch; success-first source order is the next measured shape.

## Experiment 2: result locals and loop topology

- A separate `result = NONE` local closed `tag_group_to_widget_type` exactly.
- A separate boolean result local closed `widgets_need_lighting` exactly.
- Reusing the already-loaded widget datum for the linked-list advance removed the second `datum_get` and closed `widgets_render` exactly.
- A short outer counter and success-first callback branch brought `widgets_new` to the exact 384-byte size and 15-relocation set. Its residual was traced to a `for` loop initializing the counter before the empty-block test. January tests `count` first, then enters a short-index `do/while`; that topology is measured next.

## Experiment 3: shrink the widget-block pointer lifetime

- Replacing the `for` loop with an explicit nonempty guard and short-index `do/while` retained the exact 384-byte size and 15-relocation identity set, but the named `widget_references` pointer remained live in EDI.
- January instead retains the object-definition result and repeatedly derives its `widgets` block. This shortens the derived pointer lifetime, permits VC7 to spill it at `[ebp-0x14]`, and shrink-wraps EDI to the nonempty path.
- The HCEA topology independently corroborates an object-definition local, direct block expressions, and an explicitly narrowed short increment. These are hypotheses only until the January strict comparator accepts them.

## Experiment 4: initialize the index before the nonempty guard

- After removing the named block pointer, bytes from the loop body at `+0x50` through the return became instruction-identical. The sole residual was the guard preheader at `+0x34..+0x4f`.
- January stores the zero index before the empty-block branch. Moving only that initialization before the guard removed the candidate's entry jump and made `_widgets_new` exact: 384/384 bytes, 15/15 relocation identities, normalized SHA-256 `0ae3ea3a2443...`.

## Final strict result

- All 11 functions are exact: 1,632/1,632 padded code bytes.
- `_data_0030b2b0` is exact: 200/200 bytes, 37/37 relocations, normalized SHA-256 `106dfc2b8ac744be4212214fb0dd59f4b7d371210130d3b4769837d04cb53c66`.
- All seven target-owned string COMDATs are exact: 239/239 bytes and zero relocations.
- No target-owned BSS exists.
- The table and string owner symbols retain external linkage. The source-static private helper follows the already-proven `actor_types.c` pattern and produces exact function bytes plus relocation identity.
- The full 467-object `halobetacache_build` completed successfully after admission.
- Semantic reporting accepted 3,160 exact functions with zero unit errors; project progress reported `widgets.obj` as the 268th matching Halo object.
- All 149 tooling unit tests pass, `config.json` parses successfully, the admission audit reports no new candidate or revocation involving this object, and `git diff --check` is clean.
- A final house-rule audit confirmed multiline parameters, typed accessors, explicit final returns, one-return control flow, and no forbidden assembly, volatile, undefined behavior, forced bytes, or alignment directives.

## Reopen discipline

Every subsequent source-shape change is measured against all eleven functions and the data table. An exact sibling regression is rejected immediately. No asm, volatile, byte forcing, undefined behavior, compiler-flag changes, or speculative casts are permitted.

## Independent current-campaign admission audit (2026-08-13)

This result was transplanted onto the then-current campaign tip `a489a651`
and treated as untrusted.  The lane rebuilt the object from source using XDK
3911 `CL.Exe` 13.00.9254.1 with the unchanged `/O2 /Oy- /DDEBUG /Dxbox`
flags, enumerated every external target-owned COFF section, and compared each
with `tools/coff_compare.py::section_infos_equal`.

All **19/19 owned sections** pass strictly: eleven functions, seven string
COMDATs, and the writable descriptor table.  Function evidence from the fresh
build is:

| function | padded bytes | relocations | normalized SHA-256 |
|---|---:|---:|---|
| `_tag_group_to_widget_type` | 48 | 1 | `c089e765e2a986e1d8ac5dfae2d280e1cd04dcec2e07dde023ddc022f1be0f07` |
| `_code_001257a0` | 64 | 5 | `afe03eb995375857cdb5f1b85ba80cdc3b75af37179de4862878a79d3b4e58a7` |
| `_widgets_initialize` | 176 | 16 | `affe7b6f639b587c4de2df2e146f54462d999bd7966f76662aebeab9a8b03964` |
| `_widgets_initialize_for_new_map` | 96 | 7 | `e69d20511ff0ea54b864a578b9d5c7b04914f9dbdc35ad5290696b4fe5aa981b` |
| `_widgets_dispose_from_old_map` | 96 | 7 | `31193c6b1563f03260791fdf314871227affe7b3cf5d6f28dc789615dea02b06` |
| `_widgets_dispose` | 80 | 5 | `3af29e1353d3ccff3397275468220fe952200cd230b114f9dec646abcbee7f29` |
| `_widgets_new` | 384 | 15 | `0ae3ea3a2443cfee8d983a83b0072fc3ff258192e9ed026f5ef527370ed1715e` |
| `_widgets_delete` | 256 | 14 | `9153a10bbabd49f69b75dd1851b16af843a624f9bd0603192b25e9944a9a831e` |
| `_widgets_need_lighting` | 128 | 7 | `22378cfaec31c2370ca7ccddc4d8e4775c0d172b29b9d48dc45a50ceb91e32b4` |
| `_widgets_render` | 208 | 13 | `4d10821c50de57264e12b10b750a9ceff104eb4c98587db2db5c93523b7a06f3` |
| `_widgets_update` | 96 | 5 | `e6ac57dbe6e8956068013d021080f4a5e52a900209bd03244ee43cabf237331a` |

The seven strings total 239 bytes and have no relocations.  The externally
owned `_data_0030b2b0` is 200 bytes with 37 relocations and normalized SHA-256
`106dfc2b8ac744be4212214fb0dd59f4b7d371210130d3b4769837d04cb53c66`.
There is no target-owned BSS.

### Shared-header blast radius

The reconstruction adds layouts and prototypes to eight shared headers.  A
transitive include-graph walk found **78 affected C translation units**, much
more than the obvious widget family.  To make the regression test independent
of the old report, the lane created and fully built a clean pre-change worktree
at `a489a651`, then fully built the candidate worktree.  For every affected
object it compared every target-owned section that was strict-exact before the
change against the January target again after the change, and separately
compared owner storage/type/value metadata.

The result was **1,710 prior-exact owned sections retained, zero strict
regressions, and zero symbol-metadata drifts**.  The only gains were the 19
widgets sections listed above.  Raw whole-object hashes were deliberately not
used: the absolute worktree paths stored in debug information make raw COFF
files differ without changing target-owned program sections.

Plainly: the headers provide names and layouts to the compiler, but do not
alter any previously accepted machine code or data.  The widgets object itself
now reproduces every instruction, callback-table pointer, string, relocation
destination/addend, size, and owner in the January object.

### Current house/Berth and original-bug audit

- parameters and no-argument `void` lists use one item per line;
- every void function ends in an explicit `return;`, and each function has one
  return where the measured control flow permits it;
- object access uses the established typed `object_get` macro, widget access
  uses `widget_get`, tag-block access uses `TAG_BLOCK_GET_ELEMENT`, and object
  definition access uses `object_definition_get`; there are no raw repeated
  casts around `object_get` or `tag_get`;
- structures and named constants replace raw offsets and unexplained values;
- no assembly, `volatile`, `__forceinline`, undefined behavior, byte forcing,
  compiler-flag change, alignment pragma, or direct-offset field access is
  present;
- the audit found no preserved original bug requiring a warning/fix comment.
  No behavior was silently corrected or modernized.

### Final current-campaign gates

- full `ninja halobetacache_build`: passed (466 build edges);
- `ninja progress`: exactly the expected one-object gain over the clean
  pre-change baseline, from 257 to **258/468** Halo objects; **3203/7574**
  functions and **360534/1770166** code bytes credited, with no revocation;
- `tools.audit_semantic_matches`: 11/11 widgets functions accepted from both
  ordinary objdiff and independent `semantic-coff` evidence, zero unit errors;
- transitive-header audit: 78 translation units, 1,710 previously exact owned
  sections retained, zero strict regressions, zero ownership drifts;
- `python -m unittest discover -s tools -p 'test_*.py' -v`: **173/173** passed;
- `git diff --check` and JSON parsing: clean.
