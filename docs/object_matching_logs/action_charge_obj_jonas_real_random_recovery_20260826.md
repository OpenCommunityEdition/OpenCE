# `action_charge.obj` Jonas real-random recovery

## Result

This bounded recovery starts from cumulative commit
`c07cbfb90b8bf67a86cb28bc6f3a1dffed70a486`. It adds the ordinary typed
`real_random` wrapper already authenticated by January COFF, the canonical
math interface, and the archived full-TU action-charge experiment.

`_real_random` is strict exact on the frozen recovery compile. The two
inherited lifecycle leaves remain strict exact, so `action_charge.obj`
advances from 2/22 to 3/22 exact functions. It remains `NonMatching` and
receives no data credit.

| Function | Meaningful / padded | Relocs | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_real_random` | `13 / 16` | 2 | `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c` | newly exact |
| `_action_charge_begin` | `80 / 80` | 3 | `c78c6feeb10fda8857834f5264614aa976498059590ee08304aef25f21af8bf3` | inherited exact |
| `_action_charge_update` | `71 / 80` | 2 | `f2591b2de350c20aefef42c38623875dca494bb46a51d06c6dcd5947b035976e` | inherited exact |

The accepted object total is 164 meaningful bytes, 176 padded bytes, and
seven relocations across three functions.

## Provenance and recovery status

This is not represented as a compiler-naive first shot. Archived commit
`677defb9d9b4dd0ac4f20bc2c5dbd53bf3e8d7f5` records a 20/22 full-TU
experiment in `docs/object_matching_logs/action_charge_obj.md`, blob
`bbb630a9fc673dab443948add44b8457e3b465e4`. That experiment independently
compiled `_real_random` strict exact, then restored the translation unit
because the large `action_charge_perform` and `action_charge_setup` bodies
remained non-exact. This wave recovers only the already authenticated wrapper;
none of the rejected large-function source, scheduling hypotheses, or
temporary data owners is reused.

The current two-leaf checkpoint is blob
`0ab1bc6655eda440815fd2c185160f8ba96de081`, last clarified by commit
`4053598caa60b40da0e660c138708f85e2a8550b`. The baseline source blob is
`d34d932679150399937475d915ade404f7f94ac9`; the frozen recovery source blob is
`3e76470cf8be066072239845cbf08e37aeffa458`.

The January split object has SHA-256
`76c4c0b62850ffaa42fe5027c3e68b2cc42f7640c1156342c9a9cc4d1125afea`.
The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with the unchanged XDK 3911 C command and flags.

## Typed interface and source shape

Current `source/math/real_math.h`, blob
`f14567675126171a4b1c2d3052e8058c68c029c2`, declares both callees and owns
the canonical inline behavior:

```c
real real_random(
	void)
{
	return real_seed_random(
		get_global_random_seed_address());
}
```

`actions.h` is the first transitive route to `real_math.h` in this TU. The
inline name is therefore narrowly macro-renamed to `real_random_inline` after
`cseries.h` and before `actions.h`, then undefined after the existing include
list. The explicit external definition above is retained in the public-code
section. No shared-header declaration, duplicate prototype, synthetic caller,
or emission anchor is introduced. Direct symbol inspection proves that no
`_real_random_inline` owner or reference is emitted.

Both dependencies are strict exact in complete
`source/math/random_math.obj`:

- `_get_global_random_seed_address`: 53 meaningful / 64 padded bytes, seven
  relocations, normalized SHA-256
  `978f6fa37835b338c2088717e09aa32a8e80f074c0c52043c3ee74f9e0c3e9a1`;
- `_real_seed_random`: 48 meaningful / 48 padded bytes, two relocations,
  normalized SHA-256
  `ad693d3107104928cec2113aa91e03214c37eced08553a417d7f90191ccec3b4`.

## Strict relocation and ownership evidence

The target and candidate padded bytes are identical. Both relocations are
zero-addend REL32 entries:

| Offset | Destination |
| ---: | --- |
| `+0x01` | `_get_global_random_seed_address` |
| `+0x07` | `_real_seed_random` |

The target-authoritative census is 3/22 strict-exact functions against 6,032
meaningful / 6,176 padded target code bytes. The candidate adds exactly one
NODUPLICATES code COMDAT owner `_real_random`, plus undefined references to
the two typed callees and VC7's ordinary compiler-generated `__fltused`
linker marker. The latter is an undefined external, not a runtime owner; the
same natural marker is present in the accepted `action_alert.obj` real wrapper.
There is no owner removal and no candidate `.rdata`, `.data`, BSS, COMMON, or
other runtime non-code section. All fifteen target read-only COMDATs, 243 raw
payload bytes / 256 progress bytes, remain uncredited.

The generic baseline regression gate reports exactly `_real_random` as
`NEWLY_EXACT`, both lifecycle functions as `still_exact`, and
`changed_nonexact: []`. Its nonzero result is confined to the expected new
symbol set plus compiler debug changes in `.debug$F` and `.debug$S`; no
adjudication or comparator exception is added.

## Compile and policy record

The frozen source passed the XDK `/Zs` parse-only check. The subsequent Ninja
dry run exposed exactly one
`CL build\base\source\ai\action_charge.obj` edge. That edge was invoked once,
reported exactly `[1/1] CL build\base\source\ai\action_charge.obj`, and its
output was immediately preserved as
`build/audit/action_charge_real_random_recovery_20260826.obj`, 1,405 bytes,
raw SHA-256
`365281ea87d2f74746742f68b8928223b05bee7abe16d5207734e761ba02ebb5`.
No source spelling, include position, declaration, retry, scheduling tune,
compiler flag, or second candidate compile followed.

The retained source is readable typed C with `void` on its own parameter line
and a direct explicit return. It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, forced inlining, barrier, raw offset or address,
pointer/integer reconstruction, cast or union pun, inactive-union access,
undefined behavior, synthetic caller or anchor, object-byte patch, comparator
exception, or alternate compiler flag.

Direct hardened comparison reports `all_equal: true` for the new wrapper and
both sentinels. The target object dry run reports no work after preservation,
`git diff --check` and the changed-path/protected-file review pass, and all 179
tooling tests pass. Whole-campaign reports and complete Halo/libcmt validation
are deferred to the corrected cumulative-HEAD closure so this isolated
worktree does not rebuild unrelated historical outputs.

Only `source/ai/action_charge.c` and this Jonas-owned ledger are changed. The
approval-gated Units package and its protected neighbors remain frozen. No
push is performed.
