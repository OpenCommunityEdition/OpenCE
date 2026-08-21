# `recorded_animations.obj` Jonas old-map disposal leaf

## Result

This bounded source-only wave starts from exact canonical commit
`27f042fa86ea148e8a0ebccdb4f3b4e678cfa09b`. The sole requested body was
emitted in the first and only code-producing candidate compile and is
independently strict. `recorded_animations.obj` advances from 0/16 to 1/16
exact functions and from 0/2,684 to 13/2,684 meaningful code bytes. It
remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_recorded_animations_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends. The relocations are DIR32
`_bss_00435ca4` at +1 with addend zero and REL32 `_data_make_invalid` at +7.
The other 15 target functions and all 894 target non-code bytes stay outside
this wave's credit.

## Provenance and signature

- Baseline `source/cutscene/recorded_animations.c` blob:
  `77be7d528f1d648acfc62919cad74a816c75b998`.
- Retained source blob/SHA-256:
  `1da9a38950def1486c043e8d15c5a2bb773097fe` /
  `4a1eedf2a9ccddd34e77a7800cca3536b22413109308cea45c93bda04e61dca5`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `recorded_animations.obj` SHA-256:
  `cef157f5d4aa20f06b5e14dc659f44cc75ba29d11274faa4ec74166ee51990f0`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its tree-reachable exact-name
  `src/recorded_animations_dispose_from_old_map.c` body blob is
  `c4d06da3a2b1247546289979697f95bb1db8628a`, and its separate tree-reachable
  `src/data/animation_threads.c` owner blob is
  `3354d979935ab59303d1f984139e960ed970b592`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

HCEA authenticates the public name, `void` signature, ordinary invalidation
operation, and the typed `struct data_array *animation_threads` owner.
January COFF and disassembly independently authenticate the PC code extent,
bytes, padding, call, and grouped-BSS relocation. The source models only the
typed first-field prefix of that external target-owned BSS group:

```c
struct recorded_animations_globals_prefix
{
	struct data_array *animation_threads;
};

extern struct recorded_animations_globals_prefix bss_00435ca4;

void recorded_animations_dispose_from_old_map(
	void)
{
	data_make_invalid(bss_00435ca4.animation_threads);

	return;
}
```

No shared declaration, storage definition, or layout changed.

## Defined-C and one-shot boundary

The first compiler invocation stopped in parsing because `memory/data.h`
requires the repository's core `boolean` type. It emitted no OBJ. Adding the
ordinary prerequisite `cseries/cseries.h` include was a declaration-only
parser correction made before any code-producing candidate; the requested
body, typed external, function signature, and codegen-relevant expression did
not change. The next invocation produced the first and only candidate object,
and the leaf hit immediately. There was no source-shape retry, spelling retry,
control-flow experiment, or tuning pass. Later aggregate validation and the
committed forced replay use the unchanged retained source and are
reproducibility checks, not new candidates.

The retained body is readable typed C with `void` on its own parameter line
and an explicit terminal return. It contains no assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, raw-address or byte-
offset dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, synthetic anchor, or object-byte
forcing. The candidate defines no `.rdata`, `.data`, `.bss`, COMMON symbol,
or other runtime storage, so no data credit is claimed.

## Scope and validation

Only `source/cutscene/recorded_animations.c` and this new Jonas-owned ledger
are changed. No header, storage definition, configuration, data file, semantic
exception, parked record, protected-five source, pre-existing Markdown,
Claude-owned file, or deletion is in scope. The private-EAX-and-stack recorded
animation play wrappers remain deliberately excluded.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new leaf passes with exact padded bytes and
  relocation identities.
- Semantic audit: 470 units, 4,097 functions evaluated, 3,957 semantic exact,
  101 hidden exact, 36 ordinary-only, 4,018 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 4,005/11,060 exact functions,
  478,494/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- `recorded_animations.obj`: 1/16 exact functions and 13/2,684 meaningful
  code bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Park validation: three active, zero stale, and zero invalid records.
- Complete tooling suite: 179/179 tests pass.
- A clean committed regression snapshot/check and forced object deletion,
  rebuild, regression replay, and direct comparison preserve the exact leaf,
  both relocation identities, and the zero-runtime-data shape.

The five protected large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
or history rewrite is performed.
