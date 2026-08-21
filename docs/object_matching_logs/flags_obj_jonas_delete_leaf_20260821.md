# `flags.obj` Jonas delete-leaf wave

## Result

This bounded source-only wave starts from canonical commit
`4a2a61d41143d530d193fe46fb4df0c2a4714ffa`. The one requested body was
emitted in one natural compile and is independently strict. `flags.obj`
advances from 4/16 to 5/16 exact functions and from 90/5,752 to 114/5,752
meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_flag_delete` | 24 | 32 | 2 | `cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa` |

The hardened COFF comparator proves identical padded bytes and identical
relocation addresses, types, destinations, and addends. The relocations are
DIR32 `_flag_data` at +8 and REL32 `_datum_delete` at +15. All four previously
accepted flag lifecycle functions remain independently strict. The other
eleven target functions and all 252 target non-code bytes stay outside this
wave's credit.

## Provenance and signature

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `flags.obj` SHA-256:
  `8bf8e0959be8366ab347ccbefbf24a037d9639ec1c57f779c1b9923d91c47c5d`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its
  `src/flag_delete.c` blob is
  `127e7249af571bb786a02e9f1b441b8895fe7d88`; its payload has SHA-256
  `f923edf8c0810ebf76f708c31946007697ee69615f3330ff4e09b73c91595ef6`
  and independently defines the same `void flag_delete(int)` operation.
- HCEA's typed widget callback table independently assigns `flag_delete` to
  the flag datum delete slot.
- January COFF labels the external cdecl function, reads its sole argument as
  a full 32-bit value, and passes it with `_flag_data` to `_datum_delete`.
  This agrees with the existing repository declaration:

```c
void flag_delete(
	long flag_index);
```

On the target compiler, HCEA `int` and repository `long` are both signed
32-bit types. No shared declaration or layout changed.

## Defined-C and one-shot boundary

The retained body is the direct typed operation:

```c
void flag_delete(
	long flag_index)
{
	datum_delete(flag_data, flag_index);

	return;
}
```

It uses the existing typed `struct data_array *flag_data` owner and the
existing `datum_delete(struct data_array *, long)` declaration. The body was
compiled once; there were no rejected candidates, spelling retries,
declaration retries, control-flow experiments, or compiler-control devices.

The source is readable typed C with an explicit terminal return. It contains
no assembly, volatile scheduling device, force-inline annotation, optimizer
pragma/barrier, raw address or byte-offset dereference, pointer/integer
reconstruction, pointer or union pun, inactive-union access, undefined
overflow, synthetic anchor, or object-byte forcing. The wave neither defines
new storage nor claims data credit; the pre-existing `_flag_data` ownership is
unchanged.

## Scope and validation

Only `source/objects/widgets/flags.c` and this new Jonas-owned ledger are
changed. No header, configuration, frozen-five source, semantic exception,
parked record, pre-existing Markdown, Claude-owned file, or deletion is in
scope.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new leaf and all four inherited exact
  functions pass.
- Semantic audit: 470 units, 4,066 functions evaluated, 3,926 semantic exact,
  3,987 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,974/11,060 exact functions,
  476,425/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The pre-wave regression gate reports exactly `_flag_delete` as
  `NEWLY_EXACT`, all four inherited bodies as `still_exact`, and
  `changed_nonexact: []`; its expected symbol-set finding is solely the new
  external function owner.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `flags.obj` is the final same-path reproducibility proof. No push
is performed.
