# `structure_visibility.obj` Jonas debug-PVS leaf

## Result

This bounded source-only wave starts from canonical commit
`8e3d82ed442155edb9e6f5221661f2bde4d52135`. The one requested body was
emitted in one natural candidate compile and is independently strict.
`structure_visibility.obj` advances from 0/21 to 1/21 exact functions and
from 0/7,530 to 18/7,530 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_debug_pvs` | 18 | 32 | 2 | `bf09dd7a693c7b03ee298be229006cd29be05c175fc932fe7e477868ac780b9e` |

The hardened COFF comparator proves identical padded bytes and identical
relocation addresses, types, destinations, and addends. Both relocations are
DIR32: `_debug_portals` at +7 and `_structures_use_pvs_for_vs` at +12. The
other twenty target functions and all 6,496 target non-code bytes stay outside
this wave's credit. No previously accepted function existed in this source
translation unit, so there is no inherited exact body to disturb.

## Provenance and signature

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `structure_visibility.obj` SHA-256:
  `dbe1a7580e7d22b6f01899073049db6cc2f6f556523ca148bb456e86be3f7697`.
  Its `_debug_pvs` code is at PDB-confirmed RVA `0x185E20`.
- Fresh candidate-object SHA-256:
  `b257bab9217ba6670f8acec9d3d615b8adee6005f7c7257494b49815df9c4376`.
- Clean HCEA semantic-oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its `src/debug_pvs.c`
  blob is `e73e0813147b5040a128f7b470b1f149d808e65e`, with payload SHA-256
  `91b0275274f5ffbd74e8ed4370fe094a32a991ff376a6b96b8676387a85b3b36`.
  It independently gives `void debug_pvs(uint8_t on)` and the same ordered
  assignments.
- HCEA's canonical globals header blob is
  `08c9ef3114bbe48222964cbec7bf3e252be5d5fc`, with payload SHA-256
  `44dc1f5470e8b19e8e5ebb4b8763a037cddf13e5baaabf70032cb842390add93`.
  It independently declares both destinations as external unsigned bytes;
  their separate data owners are not imported here.
- The repository's `hs.c` independently declares `void debug_pvs(boolean)`
  and routes the boolean script evaluator to it. `hs_globals_external.c`
  exposes both destination addresses through `byte` declarations and
  registers both names as `_hs_type_boolean`. In this repository, `boolean`
  is the one-byte `byte` alias, so the retained typed declaration agrees with
  all three sources without changing a shared header.
- January COFF reads the one-byte argument from `[ebp+8]`, writes it directly
  to the two undefined external byte objects in the same order, and returns.
  Its external cdecl signature, two relocation identities, and standard frame
  agree with the retained source.

## Defined-C and one-shot boundary

The retained body is the direct typed operation:

```c
extern boolean debug_portals;
extern boolean structures_use_pvs_for_vs;

void debug_pvs(
	boolean on)
{
	debug_portals = on;
	structures_use_pvs_for_vs = on;

	return;
}
```

The two globals remain undefined typed externs. This translation unit defines
no storage and claims no data credit. The body was compiled once as a
candidate; there were no rejected candidates, spelling retries, declaration
retries, control-flow experiments, or compiler-control devices.

The source is readable defined C with one parameter per line and an explicit
terminal return. It contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, raw address or byte-offset
access, pointer/integer reconstruction, pointer or union pun, inactive-union
access, undefined overflow, synthetic anchor, alignment trick, or object-byte
forcing.

## Scope and validation

Only `source/structures/structure_visibility.c` and this new Jonas-owned ledger
are changed. No header, configuration, semantic exception, parked-function
record, frozen source, pre-existing Markdown, Claude-owned file, or data owner
is changed.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: `_debug_pvs` is strict exact at 18/32 bytes
  with both required DIR32 relocations.
- Semantic audit: 470 units, 4,067 functions evaluated, 3,927 semantic exact,
  3,988 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,975/11,060 exact functions,
  476,443/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The pre-wave fail-closed regression check reports exactly `_debug_pvs` as
  `NEWLY_EXACT`, `changed_nonexact: []`, and no inherited exact functions.
  Its only hard findings are the expected `.debug$S` and symbol-set changes
  caused by adding the external function owner.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `structure_visibility.obj` is the final same-path
reproducibility proof. No push is performed.
