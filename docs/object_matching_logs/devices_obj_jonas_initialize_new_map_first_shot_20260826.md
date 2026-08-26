# `devices.obj` Jonas new-map initialization first shot

## Result

This bounded source-only wave starts from clean cumulative commit
`9b21e117818b51806a747d6bb45a8cb231e2d919`. The sole requested body
matched in the first and only production candidate compile.
`devices.obj` advances from 9/30 to 10/30 strict functions and from
411/4,463 to 430/4,463 meaningful code bytes. It remains `NonMatching` and
claims none of the target's 208 data bytes.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_devices_initialize_for_new_map` | 19 | 32 | 3 | `eda44ef025945e0276efe79797f9a48655b34bcb1e9f23f373adba5b44ff2960` |

The three relocations are exact and ordered: DIR32 `_device_groups_data` at
`+1`, REL32 `_data_make_valid` at `+7`, and REL32 `_code_00084f70` at
`+15`. The target bytes are the natural lifecycle wrapper: load and push the
typed device-group data array, call `data_make_valid`, clean the argument,
then tail-call the existing private group-creation helper.

## Provenance and documentation audit

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/devices/devices.obj`: 13,148 bytes, SHA-256
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- HCEA evidence repository commit:
  `c89106c4964f2df3a98ef7b4035d6750372797cd`.
- HCEA exact-name body blob `src/devices_initialize_for_new_map.c`:
  `8fd1a3cc26eeb94bbd455369fabbf4d24c4d00d5`.
- HCEA private helper body blob `src/create_initial_device_groups.c`:
  `03c8fd95dff638c3b7ca4e890ca64cc61c9cc339`.

HCEA independently establishes that new-map initialization first makes
`device_groups_data` valid and then creates the scenario-authored device
groups. Its later-build body inlines both operations; January PC COFF proves
that the second operation is the external object-local
`_code_00084f70` helper. January symbol class/type, exact section extent,
bytes, and relocation identities remain authoritative.

The complete reachable Git history and registered-worktree census contain no
definition of `devices_initialize_for_new_map` or call to
`code_00084f70`. Exact-string `git log --all -S` searches are empty; existing
occurrences are the January symbol inventory, lifecycle declarations/callers,
or HCEA research. The four prior `devices.obj` ledgers cover typed leaves,
old-map disposal, position setters, and the group-get/new pair, not this
wrapper.

All available Claude documentation and machine-readable evidence was searched
for this exact function and unit before mutation. It contains no competing
production body or lawful codegen recipe. Historical combined reports only
classify the function as unresolved. Claude's current hardened relocation
identity, whole-TU regression, semantic-progress, admission, and park tooling
was used as validation infrastructure, never as matching authority or an
exception. The separate HCEA/Claude later-build tooling rules were treated as
research guidance only; no donor configuration, raw address, or completion
label was transplanted.

## Retained source

The baseline source blob is
`b20ab75aa491468b606583165c727cabd8f4c2a6`. The retained source blob is
`28c313e9741531798d4adbb70251ca62fa758eca`. Its CRLF worktree form is
5,693 bytes with 272 CRLF endings, zero bare LF endings, and SHA-256
`451f3601968d7fdf82a063f500fe4a60d237d6443f69a521b20d444669670d2b`.

Only the private helper declaration and public body were added:

```c
void code_00084f70(
	void);

void devices_initialize_for_new_map(
	void)
{
	data_make_valid(device_groups_data);
	code_00084f70();

	return;
}
```

The existing `devices.h` typed `struct data_array *device_groups_data`
declaration and `memory/data.h` API are reused. No header, storage definition,
layout, caller, or shared interface changed. The body is readable C89 with
`void` on its own parameter line and an explicit terminal return. It contains
no assembly, `volatile`, `register`, pragma, intrinsic, attribute, compiler
control, barrier, raw address or offset access, pointer/integer reconstruction,
cast or union pun, undefined behavior, synthetic anchor, byte forcing, or
comparator exception.

## First-shot discipline and strict comparison

Before source mutation, the isolated baseline object was rebuilt from the
clean cumulative commit. It is 2,712 bytes with SHA-256
`fb5ca03cd1b4982e0d918a3ac246c69afaf83f0003914c5de173cdf4211cdce6`.
The pre-wave whole-TU manifest is 293,051 bytes with SHA-256
`45152dc0effc50b50b85a5103ce169b6035fc1b36d72983133303d4cc8dcfdde`.

The frozen source passed XDK `/Zs` with the production include path and flags;
the baseline artifact's size and SHA-256 remained unchanged. Its resolved
absolute path was proved inside the isolated worktree, that literal file was
deleted, absence was verified, and the selected-object dry run named exactly
one edge. The unchanged ordinary edge ran once:

```text
[1/1] CL build\base\source\devices\devices.obj
devices.c
```

The preserved first artifact is 3,070 bytes with SHA-256
`98a9dc35fdf0e83f9ea4528030436040c12ca0f6697428f0011c2f560a310851`.
There was no spelling, declaration, control-flow, source-shape, or compiler
tuning retry. CRLF normalization after preservation changes no preprocessing
tokens or Git blob and is not a code-producing experiment.

Direct hardened comparison proves the new function and all nine inherited
functions independently strict:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_devices_initialize_for_new_map` | 19 | 32 | 3 | `eda44ef025945e0276efe79797f9a48655b34bcb1e9f23f373adba5b44ff2960` |
| `_devices_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_devices_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_device_new` | 69 | 80 | 2 | `a82077d5a860161cb67250de07a5a1a827b0c5a07883a4464672a38b8216a901` |
| `_device_set_never_appears_locked` | 64 | 64 | 1 | `e180e3b5196b04ad2c72967cdc93bdc207a23cc33b12d406864567ff38d55e36` |
| `_device_one_sided_set` | 59 | 64 | 1 | `37b297855afc1da1f38223444572043ab773a86d1831a3c828a24175f687bfb6` |
| `_device_group_get_value` | 28 | 32 | 2 | `14d2c7728226358605c598e4f2a5a78223c4ed901c5c313792e88985254fd65c` |
| `_device_set_actual_position` | 53 | 64 | 2 | `d0e53196194b9cab6dd3e748e0b9c4d81902b02b465dfd019ed632d338c9d836` |
| `_device_set_desired_position` | 57 | 64 | 2 | `46aed102030b8a7a6ca51498138ebba7ce8952c3f05d2b9ea73affce0ade5b4f` |
| `_device_set_power` | 67 | 80 | 2 | `77bb40070eb2a2a67b2d36bc1bae62ffce1cf94b7a3babc9cd3eacd37972b7be` |
| **Strict total** | **430** | **512** | **17** | |

The generic pre-wave regression check reports the new wrapper as
`NEWLY_EXACT`, an empty `changed_nonexact` list, and expected owner/debug/
section-index changes because the new external function was inserted in
January-relative order before the nine existing COMDATs. It consequently
cannot carry those inherited identities through its index-based manifest.
This is reviewed evidence, not a waiver: direct hardened comparison proves
every complete inherited padded section, normalized byte, and ordered
relocation address/type/destination/addend still exact. No adjudication file,
semantic exception, or comparator change was added.

The first artifact defines exactly the ten functions in the table. Its
undefined inventory is exactly `_code_00084f70`, `_data_make_valid`,
`_device_groups_data`, `_data_make_invalid`, `_tag_get`,
`_object_get_and_verify_type`, `_object_try_and_get_and_verify_type`,
`_datum_get`, `__fltused`, `_device_group_set_actual_value`, and
`_device_group_set_desired_value`. It defines no runtime non-code symbol,
string, BSS, COMMON owner, or data section.

## Validation

The complete Halo and libcmt object graph passes and then reports no pending
work. Canonical outputs are:

- `build/report.json`: 1,574,309 bytes, SHA-256
  `701f8d1959cfc6514ed67ed705008c2e664f0ce973214d39b2bb72756dcb4f62`;
- `build/semantic_report.json`: 2,976,794 bytes, SHA-256
  `4394653c7eb131584e94925ed6c74144d53370a540513b9c3a593570dffa1529`.

Campaign progress becomes:

- all categories: 375/833 complete objects, 4,145/11,060 exact functions,
  502,369/2,198,102 meaningful code bytes, and
  1,835,208/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,978/7,574 exact functions,
  489,455/1,770,166 meaningful code bytes, and
  1,830,016/3,923,451 data bytes;
- libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit scans 470 units and evaluates 4,252 functions:
4,110 semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36
ordinary-only, 35 structural, one rejected, 12 local skips, 3,477 missing,
4,171 accepted exact, and zero unit errors. Admission remains zero candidates
and zero revocations, with only the inherited unrelated
`source/shell/shell_xbox` completion-label contradiction. Parks remain three
active, zero stale, and zero invalid. All 179 tooling tests pass, and all 14
selected tracked/configuration/canonical-report JSON documents parse.

The tracked scope is exactly `source/devices/devices.c` plus this new ledger.
There are no deletions. The complete `source/units/**` tree and the eight-path
protected guard have zero intersection with the wave. No push, amend, rebase,
history rewrite, or worktree removal is performed.

## Clean committed-state replay

The implementation-and-ledger commit is
`ec3faec3e537190fb2a14ab5c261cdc09b24ff5e`, authored and committed by
Jonas Volman. It retains source blob
`28c313e9741531798d4adbb70251ca62fa758eca` and was tracked-clean before
replay.

The first artifact was preserved again as `pre-committed-replay.obj`. The
resolved production object path was proved inside the isolated worktree, that
literal file was deleted, absence was verified, and the selected-object dry
run again named one edge. The unchanged committed source then rebuilt through
exactly the ordinary one-object action. This is a reproducibility replay, not
a candidate or tuning pass.

The 3,070-byte committed replay object has phase-specific whole-file SHA-256
`aae25827050e960dff9f2e97b8c84c0c0cd80a7493f6e46d9f0d937c1a76b1d8`.
Direct hardened comparison again proves all ten complete padded functions,
the 17 ordered relocations, and every normalized hash in the table strict.
Its defined-function, undefined-external, and zero-runtime-data inventories
are unchanged.

A clean accepted-state manifest was captured at the committed source. It is
299,637 bytes with SHA-256
`edd35e28e63f5a3ea0ec1aa43895ad17819333ea598d3c4227883ebee02a67e0`.
Its immediate no-build check has no failures or warnings, empty
`newly_exact` and `changed_nonexact` arrays, and exactly the ten table entries
under `still_exact`.

The canonical report and semantic-report sizes and hashes reproduce the
first-shot values, as do all object, function, byte, and semantic counts.
Admission remains zero candidates / zero revocations plus the inherited
unrelated `shell_xbox` contradiction. Parks remain three active / zero stale /
zero invalid. All 179 tooling tests and all 14 selected JSON parses pass. The
complete Halo-plus-libcmt graph reports no work, and the tracked worktree is
otherwise clean before this additive replay record.

## Corrected cumulative-HEAD closure

The authoritative `jonas/units-integration-20260824` branch was cleanly
fast-forwarded from `9b21e117818b51806a747d6bb45a8cb231e2d919` through the
implementation commit and isolated replay record
`67b517f0664ed4f62c96058174415af051916462`. The committed source blob and
scope are unchanged.

Before corrected replay, the cumulative worktree still held its 2,703-byte
wrapper-free `devices.obj`, SHA-256
`1399338d41f1af83e7d19475eaffae9fa4f78723074cbc3448fe6878ac45246b`.
That artifact was preserved. Its resolved path was proved inside the
cumulative worktree, the literal file was deleted, absence was verified, and
the selected-object dry run named one edge. The ordinary edge rebuilt the
committed source once.

The corrected cumulative object is 3,061 bytes with phase-specific SHA-256
`e7eabe6e69d47f91f38d27f454527567589284676ce136f8efcb4716b1828ff7`.
Direct hardened comparison again proves exactly the ten table functions,
their complete padded sections, normalized bytes, and all 17 ordered
relocations strict. The external function/data inventories remain identical
to the isolated replay.

The cumulative clean manifest is 299,637 bytes with SHA-256
`0c6b56c9bc71777f66c34c50feabef631dd9ccf84b840e3284795f61a10f7536`.
Its immediate check is clean with the ten functions under `still_exact` and
empty failure, warning, `newly_exact`, and `changed_nonexact` lists.

The cumulative canonical reports reproduce the isolated 1,574,309-byte
`build/report.json` hash
`701f8d1959cfc6514ed67ed705008c2e664f0ce973214d39b2bb72756dcb4f62`
and 2,976,794-byte `build/semantic_report.json` hash
`4394653c7eb131584e94925ed6c74144d53370a540513b9c3a593570dffa1529`.
Every campaign, object, semantic, admission, park, test, JSON, source-policy,
protected-path, and no-deletion result above is reproduced at the cumulative
head. No push, amend, rebase, history rewrite, or worktree removal occurs.
