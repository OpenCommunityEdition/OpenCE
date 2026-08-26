# `rasterizer_cinematics.obj` setter ABI repair one-shot (2026-08-25)

## Frozen scope and provenance

This bounded wave starts from clean cumulative integration commit
`12504289e36754a6ab9b000fdbe1c0eba365bfb3` in isolated branch and worktree
`jonas/rasterizer-setter-abi-repair-20260825`. The only permitted production
source is `source/rasterizer/rasterizer_cinematics.c`; this new Jonas ledger is
the only other permitted tracked path. `source/hs/hs.c`, every header,
configuration, semantic exception, parked record, object option, and Units
path are frozen. Exactly one fixed candidate may receive one ordinary
code-producing compile. There is no post-emission source spelling change,
retry, tuning pass, alternate candidate, compiler change, or flag change.

The pinned compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`. Baseline source blob
`081ef6b17ab7655ff6ef0980a00279052fbc4449`, configuration blob
`8825927921a7abf9ad1a30521c3c80df265cdb3a`, January split-object raw SHA-256
`257b38976236eb2fb5913f4bda88186c6b4de1d498c5677eddded99b5b7e6400`,
and cumulative base-object raw SHA-256
`432b0aad744e4215dc3b0b431d6f3589fe3bf5ede4d700b05fb379b52e8840b9`
are frozen before source emission.

## January target and historical exact evidence

The single requested function has this authoritative fingerprint:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_rasterizer_script_screen_effect_set_value` | 40 | 48 | 1 | `9a2d886587d1d61f101850006fcf0d8cbaae24f8ac516e75d9219302667a7852` |

The clean HCEA semantic donor is commit
`c168af2e747d3095d9a29418ae401f3a39544863`, function-source blob
`bd038cc3eadac312a85083fc480f3933a1a2727f`. The earlier bounded leaf wave,
recorded in
`docs/object_matching_logs/rasterizer_cinematics_obj_jonas_leaf_wave_20260821.md`,
compiled the natural donor body with a `short` formal and obtained strict
equality on its first compile. It was withdrawn only because the sole current
cross-TU declaration and exact caller in `source/hs/hs.c` use `word` and pass
`arguments[0].unsigned_short_value`. No code-generation miss was recorded.

The current object has five protected exact siblings. Their strict baseline is:

| Function | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_rasterizer_screen_effects_dispose_from_old_map` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_screen_effects_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_script_screen_effect_get_value` | 48 | 2 | `6caf0056daa07a5059b4224a33807b0c6011a81752dcffb205ba97ae5a7a99f7` |
| `_rasterizer_screen_effect_stop` | 16 | 1 | `ea017ca1be35d28d1d1bb50f5d53c0823d3b2872c9534121b9a83b32ef17ccee` |
| `_rasterizer_set_near_clip_distance` | 32 | 1 | `4ec7094d49d4302f2eac5a399d692b8fe71af721516a26c4941d42536dd35c05` |

## Claude documentation and tooling reconciliation

All Claude object and tooling material was screened before this shot. The HCEA
PDB/donor is used only for semantic names, types, and natural control flow;
January PC COFF, disassembly, callers, and strict relocation identities remain
authoritative. The campaign's corrected frontier material rules out hidden
clean whole-object claims and prohibits importing external assembly, pinned
bytes, object surgery, `register`, raw address arithmetic, intrinsics/puns,
compiler barriers, optimizer pragmas, undefined behavior, or alternate
toolchains. The typed IR's width-class field is not treated as a physical
register claim, and the unvalidated dirty debugger/tool branch is not used.

The Claude connected-component warning for file-local custom conventions does
not apply here: this setter has external linkage and an already exact in-repo
caller. The hardened `tools/coff_compare.py` result, including relocation
destination and ownership semantics, is the admission authority; no raw-byte,
fuzzy, semantic-only, or comparator-exception credit is permitted.

## Frozen candidate C1

C1 preserves the caller-visible `word` ABI and performs one intentional
unsigned-to-signed conversion to the domain required by January's lower-bound
test:

```c
void rasterizer_script_screen_effect_set_value(
	word effect_index,
	real value)
{
	short signed_effect_index = (short)effect_index;

	if (bss_004662f4 &&
		signed_effect_index >= 0 &&
		signed_effect_index < 4)
	{
		bss_004662f4->script_values[signed_effect_index] = value;
	}

	return;
}
```

VC7 is expected to fold the local conversion into the same signed 16-bit
load, bounds checks, and array index emitted for the historically exact
`short` formal. ISO C classifies an out-of-range unsigned-to-signed conversion
as implementation-defined, not undefined behavior. Its low-word two's-complement
result is fixed for the pinned XDK 3911 Microsoft implementation and is the
behavior authenticated by the January callee. No incompatible declaration is
introduced.

C1 is readable typed C with explicit return. It uses no assembly, `volatile`,
barrier, pragma, intrinsic, attribute, forced/no inline, `register`, raw
offset/address, pointer or union pun, inactive-union access, undefined
behavior, synthetic anchor, object-byte patch, or flag/tool change.

## Acceptance, rejection, and closeout gates

After this ledger and exact C1 source exist, the real compiler receives one
`/Zs` parse-only invocation, proven not to modify the baseline object. Exactly
one ordinary Ninja edge may then compile
`build/base/source/rasterizer/rasterizer_cinematics.obj`. The first-shot object
is copied unchanged under ignored `build/audit/` before inspection.

Acceptance requires the requested function's exact meaningful/padded size,
normalized hash, symbol properties, and sole relocation address, type,
destination, and addend. All five protected exact siblings must remain strict
exact, and the object may gain no unexpected data/storage ownership. Any
difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning, and only this ledger records the rejection.

If C1 is exact, the source and initial ledger are committed, followed by full
Halo and libcmt builds, fresh ordinary/semantic/progress reporting, semantic,
admission and parked audits, the complete tooling test suite, policy/scope and
JSON checks, and a clean committed regression snapshot. A literal one-object
delete/rebuild must reproduce the first-shot result. The replay evidence is
then appended in a ledger-only commit and replayed once more from corrected
HEAD. No amend, push, rebase, history rewrite, or worktree removal is
authorized.

## Production result

The real XDK `/Zs` invocation passed and left the baseline object raw SHA-256
`432b0aad744e4215dc3b0b431d6f3589fe3bf5ede4d700b05fb379b52e8840b9`
unchanged. The source then remained frozen at Git blob
`97f5af1b4597a8f3e3a4b079435a0c5e1fda28d0`. A Ninja dry run exposed exactly
one pending CL action, and exactly one ordinary production edge compiled
`build/base/source/rasterizer/rasterizer_cinematics.obj`. There was no direct
code-producing CL command and no second candidate compile.

The unchanged first-shot object is preserved at ignored path
`build/audit/rasterizer-setter-abi-repair-first-shot/rasterizer_cinematics.obj`,
raw SHA-256
`8f7e37f679669cb553e7c6ae2e2949938e1081c20a65358b29bb291a29e95298`.
The hardened six-function comparison returned `all_equal: true`:

- the setter is strict exact at 40 meaningful / 48 padded bytes, one `DIR32`
  relocation at `+0x05` to `_bss_004662f4` with zero addend, and normalized
  SHA-256 `9a2d886587d1d61f101850006fcf0d8cbaae24f8ac516e75d9219302667a7852`;
- all five protected siblings retain their frozen sizes, relocations, and
  normalized hashes; and
- no new constant or storage owner is emitted. The setter's only runtime
  dependency is the expected undefined target-owned `_bss_004662f4` pointer.

The exact result also authenticates the ABI repair. `word` is the repository's
`unsigned short`. The sole `hs.c` caller and January caller both zero-extend the
16-bit argument and push one 32-bit cdecl slot. VC7 eliminates the local and
emits January's signed `AX` lower/upper checks followed by the same `movsx`
array index. Thus the source preserves the exact caller while recovering the
callee's signed-domain behavior. The conversion remains ISO C
implementation-defined for out-of-range inputs, but is deterministic for the
pinned Microsoft implementation and is not undefined behavior.

The ordinary object census advances from 5/16 to 6/16 exact functions and from
81 to 121 accepted meaningful code bytes out of 1,454. No data credit is
claimed. The complete full-build/report closeout records:

- `halobetacache_build libcmt_build`: pass; 568 copied-build catch-up edges
  completed after first-shot adjudication without changing its raw object;
- direct authenticated objdiff report: 6/16 and 121/1,454 for this object;
- semantic audit: 470 units, 4,239 functions evaluated, 4,097 semantic exact,
  114 hidden exact / 64,705 hidden code bytes, 4,158 accepted exact, and zero
  unit errors;
- campaign progress: 375/833 objects, 4,132/11,060 functions, 501,809 accepted
  code bytes, and 1,835,088 accepted data bytes;
- Halo progress: 273/468 objects, 3,965/7,574 functions, 488,895 accepted code
  bytes, and 1,829,896 accepted data bytes;
- object admission: zero candidates and zero revocations, with only the
  inherited unrelated `source/shell/shell_xbox` completion contradiction;
- parked validation: three active, zero stale, zero invalid;
- complete tooling suite: 179/179 tests pass; and
- source scope, prohibited-construct scan, `git diff --check`, direct
  fingerprints, and final full-build dry run: pass.

The generated report edge would have refreshed already authenticated tool
downloads because copied Ninja timestamps are worktree-specific. The existing
local objdiff binary was invoked directly instead, exactly as in the prior
Cinematics closeout; no network, tool replacement, source change, candidate
rebuild, or acceptance exception occurred.

No committed-state replay is claimed in this initial record. After the source
and this ledger are committed from a clean index, a no-build one-unit
regression snapshot will pin the implementation commit. The verified generated
object will then be removed with literal-path containment checks, rebuilt once
through its ordinary edge, and checked immediately with the regression gate
and hardened six-function comparator. Replay facts will be added only in a
second ledger-only commit, followed by the same corrected-HEAD replay.
