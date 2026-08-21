# `devices.obj` Jonas old-map disposal leaf

## Result

This bounded source-only wave starts from exact canonical commit
`5c14f9595e860b598a2dfc49631cc6f3c4534664`. The sole requested body was
emitted in the first and only natural candidate compile and is independently
strict. `devices.obj` advances from 4/30 to 5/30 exact functions and from
191/4,463 to 204/4,463 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_devices_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends. The relocations are DIR32
`_device_groups_data` at +1 and REL32 `_data_make_invalid` at +7. All four
previously accepted functions remain independently strict. The other 25
target functions and all 208 target non-code bytes stay outside this wave's
credit.

## Provenance and signature

- Baseline `source/devices/devices.c` blob:
  `04613ddc1f1aa7fd2a06a616728870fd2214b70e`.
- Retained source blob/SHA-256:
  `55ac691dd3a5372a7dce264183e43d8029ba1aa3` /
  `58deb2d3e929ef2e8430b18019d23b0c4b78bf737f74aff30c6bafa29677487f`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `devices.obj` SHA-256:
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name
  `src/devices_dispose_from_old_map.c` body blob is
  `9bfb43330c841fd44890face99b55cba503fa375` and independently supplies the
  same per-map data-array invalidation behavior.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

HCEA authenticates the public name and natural typed operation. The existing
`devices.h` declaration fixes `device_groups_data` as
`struct data_array *`, and `memory/data.h` supplies the ordinary typed
`data_make_invalid(struct data_array *)` API. January COFF and disassembly
independently authenticate the PC signature, code extent, bytes, padding, and
relocation ownership:

```c
void devices_dispose_from_old_map(
	void)
{
	data_make_invalid(device_groups_data);

	return;
}
```

No shared declaration or layout changed.

## Defined-C and one-shot boundary

The single natural candidate compile covered exactly this one requested
function and hit immediately. There was no rejected candidate, source-shape
retry, spelling retry, declaration retry, control-flow experiment, or tuning
pass. Later aggregate validation and the committed forced replay use the
unchanged retained source and are reproducibility checks, not new candidates.

The retained body is readable typed C with `void` on its own parameter line
and an explicit terminal return. It contains no assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, raw address or byte-
offset dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, synthetic anchor, or object-byte
forcing. The candidate defines no `.rdata`, `.data`, `.bss`, COMMON symbol,
or other runtime storage, so no data credit is claimed.

## Preserved exact bodies

Direct hardened comparison records all five accepted bodies independently:

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_devices_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_device_set_never_appears_locked` | 64 | 64 | 1 | `e180e3b5196b04ad2c72967cdc93bdc207a23cc33b12d406864567ff38d55e36` |
| `_device_one_sided_set` | 59 | 64 | 1 | `37b297855afc1da1f38223444572043ab773a86d1831a3c828a24175f687bfb6` |
| `_device_set_power` | 67 | 80 | 2 | `77bb40070eb2a2a67b2d36bc1bae62ffce1cf94b7a3babc9cd3eacd37972b7be` |
| `_devices_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| **Final strict total** | **204** | **240** | **6** | |

The source edit does not touch any pre-existing exact body.

## Scope and validation

Only `source/devices/devices.c` and this new Jonas-owned ledger are changed.
No header, storage definition, configuration, data file, semantic exception,
parked record, protected-five source, pre-existing Markdown, Claude-owned file,
or deletion is in scope.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new leaf and all four inherited exact
  functions pass with exact relocation identities.
- Semantic audit: 470 units, 4,089 functions evaluated, 3,949 semantic exact,
  101 hidden exact, 36 ordinary-only, 4,010 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 3,997/11,060 exact functions,
  477,934/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- `devices.obj`: 5/30 exact functions and 204/4,463 meaningful code bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Park validation: three active, zero stale, and zero invalid records.
- Complete tooling suite: 179/179 tests pass.
- A clean committed regression snapshot/check and forced object deletion,
  rebuild, regression replay, and direct comparison preserve all five exact
  functions, their relocation identities, and the zero-runtime-data shape.

The five protected large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
or history rewrite is performed.
