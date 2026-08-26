# `ui_widget.obj` Jonas seven-leaf first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`e825ed4ef91f7a461fadc249078335144ccb17ff`. All seven requested bodies
matched in the first and only production candidate compile. `ui_widget.obj`
advances from 4/102 to 11/102 strict functions and from 32/23,042 to
113/23,042 meaningful code bytes. It remains `NonMatching` and claims none
of the target's 26,804 data bytes.

| New retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ui_widgets_set_fade_value` | 13 | 16 | 1 | `3307464d0a4c3704ea054046b1a71de6ed4ab1c8c397d82e01d96e9777ce0123` |
| `_ui_widget_debug_show_path` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_main_menu_active` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_main_menu_is_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_ui_widget_load_progress_widget` | 16 | 16 | 2 | `fe882d09c800f4644f0bc8697f08b2787ad6c0fa8f285098947f4cd38b34ed3b` |
| `_filesystem_check_thread_is_active` | 14 | 16 | 1 | `e3d713882962f29c50f9529269c70d85d4e93d911caff2d69af551668dff0246` |
| `_ui_main_menu_music_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| **New strict total** | **81** | **112** | **8** | |

The BSS relocations are exact and ordered by function: DIR32
`_bss_00454240+0x82C` at `+7`, `+0x864` at `+7`, `+0x868` at `+7`,
`+0x868` at `+1`, `+0x85C` at `+2`, and `+0x866` at `+1`. The progress
stub has DIR32 relocation `+1` to its exact 68-byte string COMDAT and REL32
relocation `+8` to `_error`. The external BSS owner is declared but never
defined, so this wave emits no BSS and claims no storage ownership.

## Provenance and documentation audit

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/interface/ui_widget.obj`: 111,969 bytes, SHA-256
  `783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Current HCEA research cache commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its
  `src/halo/interface/ui_widget.c` blob is
  `3ebf5a20059a2dcea0db3591d9cb4eead9bd1572`.
- Current Claude/HCEA working repository commit:
  `afc14e5b1d016560ac5808a7da5aa9f2adb2b885`; its corresponding source
  blob is `9fad3cb54b67b2536d69c3139c275a5416009103`.

January COFF proves all seven symbols are external cdecl functions with the
current public signatures, complete section extents, normalized bytes, and
relocation identities shown above. HCEA bounds and function metadata
independently corroborate the names, parameter counts, and leaf semantics.
Its separate later-build source contains the same progress-stub message and
error priority, but uses forbidden raw addresses for neighboring state and
therefore was not transplanted. January evidence remains authoritative for
scope, layout, bytes, and addends.

The complete reachable campaign history and registered production-worktree
census contain no definition of these seven functions. The only earlier
same-body occurrence of `ui_widget_load_progress_widget` is separate
Claude/HaloRec research, not a January/XDK-3911 campaign attempt. Existing
campaign occurrences of the other names are declarations, callers, symbol
inventories, target metadata, or unresolved reports.

All available Claude documentation and machine-readable evidence relevant
to this object and its tooling was searched before mutation. The identical
`work/halo/CLAUDE.md` and `work/lanes/interface/CLAUDE.md` documents have
SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`;
the research-cache document has SHA-256
`97d4c63b9dd9d642cbe6774c5f3a578c60f2de002fce3ac888cd4ee635cc4c3d`.
Their binary-first, C89, ABI, and narrow-validation guidance was applied as
research discipline. Donor-only commands, hooks, raw-address practices,
configuration, and completion labels were not treated as production rules.
The current campaign's hardened relocation identity, whole-TU regression,
semantic-progress, admission, and park tools were used as validation
infrastructure, never as matching authority or an exception.

## Retained source

The baseline source blob is
`7b07bd7e1dd54672d8a0b8a13e512c883978d656`. The retained source blob is
`9d6207c85a9531b5092ef4808bb65d121d44cda9`. Its canonical CRLF worktree
form is 25,375 bytes with 800 CRLF endings, zero bare LF endings, and
SHA-256
`ab4b704ed6d8e019cd7819986c160932b190c13a1f71ff0472cb1e35495531ff`.

Only `source/interface/ui_widget.c` changed. It adds `errors.h`, two
translation-unit-only prefix structures, seven compile-time layout checks,
one declaration-only external BSS owner, and the seven public bodies. The
existing `ui_widget.h` declarations are reused unchanged. The authenticated
private layout is:

```c
struct ui_widget_runtime_globals_prefix
{
	byte reserved0000[0x2C];
	real fade_to_black;
	byte reserved0030[0x2C];
	void *initialization_thread;
	byte reserved0060[4];
	boolean debug_show_path;
	byte reserved0065;
	boolean main_menu_music_active;
	byte reserved0067;
};

struct ui_widget_bss_prefix
{
	byte reserved0000[0x800];
	struct ui_widget_runtime_globals_prefix widget_globals;
	boolean we_are_at_the_main_menu;
};

extern struct ui_widget_bss_prefix bss_00454240;
```

The checks prove offsets `0x2C`, `0x5C`, `0x64`, and `0x66`, inner size
`0x68`, inner placement `0x800`, and the main-menu byte at `0x868`. The
outer size and unconsumed trailing target storage remain intentionally
unclaimed. The bodies use direct typed members, preserve byte returns without
normalization, compare the thread pointer against `NULL`, and call
`error(_error_silent, ...)` with the exact literal.

The source is readable C89 with every parameter on its own line, `void` on
its own no-argument line, and explicit terminal returns for all void bodies.
It contains no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, compiler control, barrier, raw address or raw offset access,
pointer/integer reconstruction, cast or union pun, undefined behavior,
synthetic anchor, byte forcing, data definition, or comparator exception.

## First-shot discipline and strict comparison

The clean cumulative baseline object is 1,443 bytes with SHA-256
`5892dc80666e51c2a1b225c3d64630ab82747183b05a29111598f391eb2030da`.
The pre-wave whole-TU manifest is 1,970,050 bytes with SHA-256
`fd1119fd1b5c178b35e34d57aa8f25ae32ad004d096149380ef6728646f64e22`.

The frozen source passed XDK `/Zs` with the production include path and flags;
the preserved baseline artifact's size and SHA-256 remained unchanged. The
ordinary selected-object edge then ran exactly once:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
ui_widget.c
```

The preserved first artifact is 3,193 bytes with SHA-256
`ea55c5d0ee9d8209bdc01ed3f409fde76c6c64fada3f543649fc332cdd7289e2`.
There was no spelling, declaration, layout, control-flow, source-shape, or
compiler-tuning retry. CRLF normalization after preservation changes no
preprocessing token or Git blob and is not a code-producing experiment.

Direct hardened comparison proves the seven new functions and all four
inherited functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_ui_widgets_safe_to_load` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_ui_widgets_set_fade_value` | 13 | 16 | 1 | `3307464d0a4c3704ea054046b1a71de6ed4ab1c8c397d82e01d96e9777ce0123` |
| `_ui_widget_debug_show_path` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_widget_instance_count_children` | 29 | 32 | 0 | `ae15477fb628a00d4f51a719438f430179585e7069e2e464dee76e2b0d39b60f` |
| `_main_menu_active` | 13 | 16 | 1 | `de8c4439b030245988663f8f08b89f6a3ab735129d5467f31f6d3f0bde83c561` |
| `_main_menu_is_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_ui_widget_load_progress_widget` | 16 | 16 | 2 | `fe882d09c800f4644f0bc8697f08b2787ad6c0fa8f285098947f4cd38b34ed3b` |
| `_filesystem_check_thread_is_active` | 14 | 16 | 1 | `e3d713882962f29c50f9529269c70d85d4e93d911caff2d69af551668dff0246` |
| `_ui_main_menu_music_active` | 6 | 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| `_code_000d4680` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_code_000d4690` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| **Strict total** | **113** | **192** | **8** | |

The generic pre-wave regression check reports all seven new leaves as
`NEWLY_EXACT`, an empty `changed_nonexact` list, and expected owner/debug/
section-index changes because new external COMDATs and one string COMDAT were
inserted in January-relative order. It consequently carries only
`_ui_widgets_safe_to_load` through its original indices. This is reviewed
evidence, not a waiver: direct hardened comparison proves every complete
inherited padded section, normalized byte, and ordered relocation
address/type/destination/addend still exact. No adjudication file, semantic
exception, or comparator change was added.

The first artifact defines exactly the eleven table functions. Its undefined
inventory is exactly `_bss_00454240`, `__fltused`, and `_error`. Its only
defined runtime non-code externals are the inherited
`_dashboard_abort_error` data symbol and the exact progress-message string
COMDAT. It defines no BSS or COMMON owner. Neither non-code symbol receives a
semantic-data exception or matched-data credit.

## Validation

The complete Halo and libcmt object graph passes and reports no pending work.
Canonical outputs are:

- `build/report.json`: 1,574,535 bytes, SHA-256
  `501ab948b85afb0c702a0a1d08637ffb8e2a01ec6b9d246fbd49669563cf9602`;
- `build/semantic_report.json`: 2,981,518 bytes, SHA-256
  `e84969333cc632f30d09ce903395c01866a8552d8bb3dfd2eaf3b910c8881734`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,152/11,060 exact functions,
  502,450/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,985/7,574 exact functions,
  489,536/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,259 functions:
4,117 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,470 missing,
4,178 accepted exact, and zero unit errors. Admission remains zero candidates
and zero revocations, with only the inherited unrelated
`source/shell/shell_xbox` completion-label contradiction. Parks remain three
active, zero stale, and zero invalid. All 179 tooling tests pass, and all 14
selected tracked/configuration/canonical-report JSON documents parse.

The tracked scope is exactly `source/interface/ui_widget.c` plus this new
ledger. There are no deletions. The complete `source/units/**` tree and the
eight-path protected guard have zero intersection with the wave. No push,
amend, rebase, history rewrite, or worktree removal is performed.

## Clean committed-state replay

The implementation-and-ledger commit is
`87222a56dbbf4df52274773e3befc59df4375486`, authored and committed by
Jonas Volman. It retains source blob
`9d6207c85a9531b5092ef4808bb65d121d44cda9` and was tracked-clean before
replay.

The first artifact was preserved again as `pre-committed-replay.obj`. The
resolved production object path was proved inside the isolated worktree, that
literal file was deleted, and absence was verified. The selected-object dry
run named exactly one edge. The unchanged committed source then rebuilt
through exactly the ordinary one-object action. This is a reproducibility
replay, not a candidate or tuning pass.

The 3,193-byte committed replay object has phase-specific whole-file SHA-256
`4c774c20ab6203a8f3f63506e27e20c846fd906606701dc9c96b49d1365370b1`.
Direct hardened comparison again proves all eleven complete padded functions,
the eight ordered relocations, and every normalized hash in the table strict.
Its defined-function, undefined-external, inherited data, exact-string, and
zero-BSS inventories are unchanged.

A clean accepted-state manifest was captured at the committed source. It is
1,999,649 bytes with SHA-256
`f31762184e84a464c04d547a252dfe1f3cc3eccd9bfb0c1dd3141531e634a39b`.
Its immediate no-build check has no failures or warnings, empty `newly_exact`
and `changed_nonexact` arrays, and exactly the eleven table entries under
`still_exact`.

The canonical report and semantic-report sizes and hashes reproduce the
first-shot values, as do all object, function, byte, and semantic counts.
Admission remains zero candidates / zero revocations plus the inherited
unrelated `shell_xbox` contradiction. Parks remain three active / zero stale /
zero invalid. All 179 tooling tests and all 14 selected JSON parses pass. The
complete Halo-plus-libcmt graph reports no work, and the tracked worktree is
otherwise clean before this additive replay record.
