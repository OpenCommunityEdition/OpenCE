# `error_geometry.obj` Jonas initialize first shot (2026-08-26)

## Result and fixed boundary

This bounded wave starts from clean cumulative commit
`7fe9098ef01e777b5303464c09cb34eb987e76e7`. The sole frozen production
candidate was `source/tool/error_geometry.obj:_error_geometry_initialize`
together with its exact same-TU initialized owner `_data_00307850`, its two
compiler-owned assertion literals, and the previously accepted
`_bss_004561b4` / `_error_geometry_dispose` sentinels. The ordinary typed C
source is strict exact on the first and only candidate-producing compile and
is retained without tuning.

The object advances from 1/17 to 2/17 exact functions, from 29/6,455 to
79/6,455 meaningful code bytes, and from 4/1,528 to 120/1,528 matched data
bytes. Campaign-wide progress advances by 50 meaningful code bytes, 116 data
bytes, and one function. `error_geometry.obj` remains `NonMatching`; no
whole-object completion or semantic exception is claimed.

| Retained owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_error_geometry_initialize` | `50 / 64` | 7 | `cee9cb4e0346cc4d777943545b1076d78675ec565f43b6a3f0b899bca88a73ee` |
| `_error_geometry_dispose` | `29 / 32` | 3 | `002f6d2344aa94af0039847531bace2e6179667baa3383e8634f17a707cdbb8c` |
| `_data_00307850` | `116 / 116` | 0 | `4f61d488466ad3c2101f591022495972265241bbe75195680f7296923e6c5ba2` |
| `_bss_004561b4` | `4 / 4` logical zero bytes | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |

The approval-gated Units package and its shared/protected neighbors remain
frozen. No Units approval phrase was supplied, and this wave does not edit or
stage any Units or protected path.

## Immutable inputs and first artifact

- January split object: 24,417 bytes, SHA-256
  `3fff59eb8ccafe28e46399032a19b4083d968b2b6aa2abc78d025f75ec8ee057`.
- Baseline source blob:
  `74e919f62df07122dfed4593b452d55bc114d546`.
- Frozen candidate source blob:
  `16d450c3a87a6184729acd2b91664902df90f8cb`.
- Frozen candidate source payload: 6,511 bytes, SHA-256
  `9111568e1293af96aebbd016e10d14be64e6c822fabe2b636eadc30d77851cb8`.
- Preserved baseline object: 883 bytes, SHA-256
  `7f51c4041e550fff928bb9bc5b2e31bff05558aff0f8520fca056fd28b53fb33`.
- Preserved first candidate object:
  `build/audit/error_geometry_initialize_first_shot_20260826/candidate-first.obj`,
  1,921 bytes, SHA-256
  `134faa5f1c0e8a6e3665f183ff4a9c53536c6338a3ba90ef3ac96f95c4130c42`.
- Frozen pre-wave regression manifest: 679,299 bytes, SHA-256
  `bef5b5f7d7181c270b5b5cc10fd3e0ca08c7cfb497ff113795f18f1356b2c16c`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 from
  XDK 3911, 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The ordinary object edge uses the repository's unchanged production flags,
compiler, include graph, and object path. No alternate compiler or codegen
control was used.

## Claude, HCEA, history, and tooling evidence

January COFF remains the admission authority. Every Claude/HCEA/later-source
artifact below was used only to establish provenance, semantics, or a
fail-closed tooling rule; none was treated as a byte oracle in place of the
target object.

Claude's duplicated residual inventories at
`C:/halo-worktrees/claude-near-complete-objects-20260816/research/residual_frontier.json`,
`C:/halo-worktrees/claude-finish-hs-20260816/research/residual_frontier.json`,
and `C:/halo-worktrees/claude-untried-objects/research/residual_frontier.json`
are byte-identical, with SHA-256
`db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
They classify all `error_geometry` functions, including initialize, as
target-only and provide no base/residual/error/owner-name recipe.

Their byte-identical `unwritten_string_synopsis.json` files have SHA-256
`9786012cf2daa5dda7b228ae0ecef0ee80e3e37760f21d4d9bd4e538beb5f1d1`.
The initialize record gives the 64-byte padded function and exactly the path
and predicate strings used here, but no body. Claude's near-complete
`source/tool/error_geometry.c` is an inventory-only empty skeleton (5,559
bytes, SHA-256
`28cd60f0a90fe7f6b9220732d3d1a586920df06ee4b81f890158f9b2e4b45c3d`).

The HCEA January correspondence file
`C:/halo-worktrees/fable-crossbuild-bridge/research/crossbuild_bridge/hcea_january_correspondence.json`
has SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
It records cdecl, January meaningful size 50, `january_only`, and
`no-hcea-name`. The clean HCEA snapshot at
`c168af2e747d3095d9a29418ae401f3a39544863` supplies no exact-name or body
donor.

The later HSMAP/Stian reconstruction at
`C:/halo-worktrees/claude-hsmap-2/src/halo/main/main.c` is 106,114 bytes with
SHA-256
`9e6334963de42a88caa2835810c8d748b1e8a51adee15be8c6ad53269fbfa3fb`.
It corroborates the `debug.wrl` state, cached file pointer, set-name
semantics, and assertion path/predicate, but is later-binary lift provenance,
not a January compiler-body donor.

All relevant inspected `CLAUDE.md` copies are byte-identical, 47,280 bytes,
SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`.
Their applicable guidance was followed: MSVC-era C89-compatible source,
binary-first ABI/call-site/disassembly verification, small source changes,
and no inline assembly. Their `rtk`, lift, knowledge-base patching, VC71 LCS,
and runtime tooling concern a different later-XBE project and were not used
as admission or mutation mechanisms here. Claude's campaign README warning
about escaping `c:\\halo...` paths was also applied by verifying the literal
bytes after compilation rather than editing through JSON.

Claude's matching methodology is byte-identical to the current repository
copy (11,329 bytes, SHA-256
`3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`).
Its padded-byte, normalized-relocation, ABI, ownership-before-tuning, and
whole-TU regression rules are the direct gates used below. The authenticated
current comparator and regression-gate payloads are respectively SHA-256
`411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`
and
`d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`.
The production shape-transfer executor is inapplicable because there is no
authenticated atlas donor/recipient source pair; no transfer or synthetic
body was attempted.

Twenty-seven preserved historical base objects plus the current baseline
contain no initialize definition. Reachable Git `-G`/`-S` history likewise
contains no body, and no prior candidate compile was found. The existing
dispose ledger froze only `_error_geometry_dispose` and `_bss_004561b4`.
This is therefore a genuine first shot under all available candidate
evidence.

## Exact January packet and closure

January requires one external type-`0x20` cdecl `void(void)` owner with 50
meaningful and 64 padded bytes. Its normalized instruction stream loads and
tests `_bss_004561b4`; on failure it calls `display_assert` with the exact
path, line `0x44`, and exact predicate, calls `system_exit(-1)`, then calls
`remove` on `_data_00307850.filename` and returns. The remaining 14 bytes are
NOP padding.

The exact ordered relocations are:

| Offset | COFF type | Destination |
| ---: | --- | --- |
| `+0x01` | `DIR32` | defined non-code `_bss_004561b4 + 0` |
| `+0x0E` | `DIR32` | exact path literal |
| `+0x13` | `DIR32` | exact predicate literal |
| `+0x18` | `REL32` | `_display_assert + 0` |
| `+0x1F` | `REL32` | `_system_exit + 0` |
| `+0x27` | `DIR32` | defined non-code `_data_00307850 + 0` |
| `+0x2C` | `REL32` | CRT `_remove + 0` |

The exact literal payloads are:

- `c:\\halo\\SOURCE\\tool\\error_geometry.c\0`, 37 bytes, SHA-256
  `44156e74dad2019ffb44f2e3b88db505fad3d8574f5da533efccc15b74aec633`;
- `error_geometry_file==NULL\0`, 26 bytes, SHA-256
  `1d8b25a8709d6e6134a9c4a706c16ca9017bc0b9818d03cfc624fa85ee13dd2`.

`_data_00307850` is one external storage-class-2 owner at offset zero in a
116-byte initialized read/write `.data` section with 8-byte alignment and no
relocations. Bytes 0 through 63 are `debug.wrl` plus zero fill. Bytes 64
through 115 are the identity `real_matrix4x3`, byte-identical to the accepted
current `source/math/real_math.obj:_rdata_0027afb0`; that matrix span has
SHA-256
`be9ab0782f2000d0dcf4b705d4f6fad3826100554cab76f708c1acefcb5ae986`.

All external callees are already exact and link-owned:

- `libs/libcmt/unlink:_remove`, 40/40 bytes, three relocations;
- `source/cseries/cseries:_display_assert`, 77/80 bytes, six relocations;
- `source/cseries/cseries_windows:_system_exit`, 5/16 bytes, one relocation;
- CRT `_fclose`, inherited by the retained dispose function.

The candidate's runtime defined-owner inventory is exactly
`_bss_004561b4`, `_error_geometry_dispose`,
`_error_geometry_initialize`, `_data_00307850`, and the two expected string
COMDATs. Its undefined inventory is exactly `_fclose`, `_remove`,
`_display_assert`, and `_system_exit`. No absent same-TU helper or unowned
storage is introduced, so the wave is fully link-closed.

## Retained typed source

The retained implementation uses the repository's normal `match_assert`
macro so the stringized token sequence remains exactly
`error_geometry_file==NULL` while the evaluated expression expands through
the readable alias to the already-owned file pointer. `real_math.h` supplies
the established `real_matrix4x3` type and initializer shape. Compile-time
C89 typedef checks prove the complete structure size and transform offset;
they emit no runtime owner.

```c
#include "cseries.h"
#include "math/real_math.h"

#define error_geometry_file bss_004561b4

struct error_geometry_globals
{
	char filename[64];
	real_matrix4x3 transform;
};

typedef char verify_error_geometry_globals_size[
	sizeof(struct error_geometry_globals) == 0x74 ? 1 : -1];
typedef char verify_error_geometry_transform_offset[
	offsetof(struct error_geometry_globals, transform) == 0x40 ? 1 : -1];

extern struct error_geometry_globals data_00307850;

void error_geometry_initialize(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\tool\\error_geometry.c",
		0x44,
		error_geometry_file==NULL);
	remove(data_00307850.filename);
	return;
}

struct error_geometry_globals data_00307850 =
{
	"debug.wrl",
	{
		1.f,
		{
			{ 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f }
		}
	}
};
```

The previously accepted BSS declaration and dispose body remain physically
unchanged.

## First-shot compile and strict gates

The frozen source first passed XDK `/Zs` with the exact production arguments.
The existing object hash and timestamp remained unchanged. The baseline
object was preserved, its resolved absolute path was proved inside the
isolated worktree, exactly that literal file was removed, and absence was
verified.

The copied support graph required one non-production configuration refresh
before the selected object edge. `build.ninja` reproduced its authenticated
625,957-byte payload with SHA-256
`6123636fac2ddfe09196d95f7e8618a32db0603eac1d59f3771c0e78fc1bcff2`.
The complete sole candidate-producing output was:

```text
[1/1] RUN configure.py
[1/2] CL build\base\source\tool\error_geometry.obj
error_geometry.c
```

There was exactly one CL invocation and no tuned or retry candidate. The
emitted object was preserved immediately, and a selected-object Ninja dry run
now reports `ninja: no work to do`.

Direct hardened comparisons return strict section equality for both code
owners, all padded bytes, and every ordered relocation. Ownership-aware data
comparisons return strict equality for the 116-byte initialized owner and the
four-byte BSS. The initialized owner has zero relocations, so no alias or
semantic exception is involved. Direct owner-section comparisons also return
strict equality for both assertion COMDATs: 26 and 37 bytes respectively,
zero relocations, and the exact literal hashes recorded above.

The frozen pre-wave whole-TU manifest reports only
`_error_geometry_initialize` as newly exact and `changed_nonexact: []`.
Generic structural failures are limited to the reviewed new data/literal/
debug/symbol identities and compiler identity renumbering. No adjudication,
exception, configuration change, or comparator relaxation was added. A clean
accepted-state snapshot and replay remain mandatory before integration.

## Deferred neighboring work

`error_geometry_set_name` is a larger state transition and was not bundled
with this bounded owner. Point/comment/bounded-point follow-ons require absent
helpers and are not full-link closed. The closeout audit also rejected
nominal near-complete `wind.obj`, `player_rumble.obj`,
`stack_memory_pool.obj`, and `light_volumes.obj` production shots because
their remaining source/ABI fixed points have no new lawful exact recipe.
Those objects were neither edited nor compiled in this wave.

## Source-policy and scope record

The selected source uses typed structures, readable ordinary C, the required
no-argument formatting, and explicit returns. It uses no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, force-inline control,
barrier, raw address or byte offset, pointer/integer reconstruction, cast or
union pun, inactive-union access, undefined behavior, synthetic caller or
anchor, byte patch, semantic/comparator exception, or alternate compiler
flag.

Only `source/tool/error_geometry.c` and this Jonas-owned ledger are selected
for the implementation commit. No header, configuration, semantic ledger,
parked record, completion label, protected source, Units source, or tracked
deletion changes. No push, amend, rebase, history rewrite, or worktree removal
is performed.

## First validation before committed replay

The full `halobetacache_build` plus `libcmt_build` graph completed all 568
required actions with exit zero; the already-current frozen candidate object
was not rebuilt in that graph. The canonical no-build report is 1,574,220
bytes with SHA-256
`b8a1e632fcc38eb5f968404086d8c586b6ee7bd33548b0c0cf71e1e8e156bfb6`.
The semantic report is 2,974,776 bytes with SHA-256
`926f5872b4512ac9b51133b2e751907f996d0b1c2215a0e553866b802f1d01ca`.

The strict semantic audit reports 470 units, 4,249 functions evaluated,
3,480 missing, 4,107 semantic exact, 114 hidden exact / 64,705 hidden code
bytes, 36 ordinary-only, 35 structurally accepted, one rejected, zero unit
errors, 12 local skips, and 4,168 accepted exact functions.

Current campaign progress is:

- all categories: 375/833 matched objects, 4,142/11,060 functions,
  502,302/2,198,102 meaningful code bytes, and 1,835,208/4,176,062 data
  bytes;
- Halo: 273/468 matched objects, 3,975/7,574 functions,
  489,388/1,770,166 meaningful code bytes, and 1,830,016/3,923,451 data
  bytes;
- libcmt: 102/212 matched objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

Object admission reports zero candidates and zero revocations, with only the
inherited unrelated `shell_xbox` contradiction. Park validation reports
three active, zero stale, and zero invalid entries. All 179 tooling tests
pass, all 55 relevant configuration/report/audit JSON files parse, and the
tracked-scope/deletion/diff checks are clean before adding this ledger.

## Clean committed-state replay

Implementation-and-ledger commit
`4c361d7f1d218775ebedcf194e25c856fb72d5dc` is clean and authored by
Jonas Volman. Its committed source blob is exactly the frozen candidate blob
`16d450c3a87a6184729acd2b91664902df90f8cb`; no C token or Git source blob
changed after the first production compile.

The final physical-format audit identified mixed line endings left by patch
application. The source and new ledger were mechanically normalized to the
repository-required CRLF convention before replay: the source now has 223
CRLF and zero bare-LF endings, and the implementation-phase ledger had 339
CRLF and zero bare-LF endings before this additive section. Git's normal text
filter reproduced the same committed
source and ledger blobs, the index remained content-clean, and the C token
stream was unchanged. This was a house-format correction, not a candidate,
codegen, or source-shape retry.

A one-unit accepted-state snapshot was captured without building from that
exact clean commit at
`build/audit/error_geometry_initialize_first_shot_20260826/committed-manifest.json`.
It is 696,503 bytes with SHA-256
`d030471af94e680cdc4faa8c79c1ade520ad58b1d6e42d69295d539edc1f0c47`.

The resolved `build/base/source/tool/error_geometry.obj` path was proved
inside the isolated worktree. The exact 1,921-byte first object was preserved,
exactly that literal path was removed, and absence was verified. The unchanged
normal selected-object edge then reported exactly:

```text
[1/1] CL build\base\source\tool\error_geometry.obj
error_geometry.c
```

The replay is preserved at
`build/audit/error_geometry_initialize_first_shot_20260826/committed-replay.obj`,
1,921 bytes, phase-specific raw SHA-256
`1b858211ccaee070c9caf61462df41a62a5a72eea28da9a7d9cf5ba9e27eb83f`.
Its raw object hash differs from the first artifact only in compiler-debug
records. The admitted runtime evidence is identical.

Direct hardened comparison again returns `all_equal: true` for both complete
code sections: 64/7 and 32/3 padded bytes/relocations with their frozen
hashes. Direct owner comparisons again return equality for `_data_00307850`
(116/0), `_bss_004561b4` (4/0), and both string COMDATs (26/0 and 37/0),
including every normalized hash. The clean manifest check has no failure or
warning, no newly exact or changed-nonexact entry, and exactly
`_error_geometry_dispose` and `_error_geometry_initialize` in `still_exact`.
The selected-object Ninja dry run reports `ninja: no work to do`.

The canonical report and semantic audit were regenerated directly from the
replay object and reproduce the pre-commit hashes and counts exactly:

- `build/report.json`: 1,574,220 bytes, SHA-256
  `b8a1e632fcc38eb5f968404086d8c586b6ee7bd33548b0c0cf71e1e8e156bfb6`;
- `build/semantic_report.json`: 2,974,776 bytes, SHA-256
  `926f5872b4512ac9b51133b2e751907f996d0b1c2215a0e553866b802f1d01ca`.

Progress reproduces 4,142 accepted campaign functions and 502,302 meaningful
code / 1,835,208 data bytes. Admission remains zero candidates and zero
revocations with the one inherited `shell_xbox` contradiction. Parks remain
three active, zero stale, and zero invalid. The complete tooling suite again
passes 179/179 tests. The implementation commit is otherwise clean, and this
replay record is additive documentation only.

## Corrected cumulative-HEAD closure

The authoritative `jonas/units-integration-20260824` branch was
fast-forwarded from `7fe9098ef01e777b5303464c09cb34eb987e76e7` through the
two isolated commits to
`a8e5a00eaa3fae4091ff46e38cc81ddcbd72bcfd`. Integration changed only the
error-geometry source and this ledger. The committed source blob remains
`16d450c3a87a6184729acd2b91664902df90f8cb`; both files check out with CRLF,
and no Units source, protected neighbor, Units/game-engine interface,
configuration, semantic ledger, or parked record changed.

At that clean cumulative HEAD, the prior 883-byte dispose-only object was
proved inside the cumulative worktree, preserved, deleted literally, and
confirmed absent. The normal selected-object edge ran once and reported
exactly:

```text
[1/1] CL build\base\source\tool\error_geometry.obj
error_geometry.c
```

The corrected replay is preserved at
`build/audit/error_geometry_initialize_corrected_head_replay_20260826.obj`,
1,913 bytes, with phase-specific raw SHA-256
`de42ee6a1001d93713033a2fbdb7bca2d20cb5192528221537131c831728feb5`.

Direct hardened comparison again proves both complete code sections, all ten
ordered relocations, and their normalized hashes exact. Direct owner
comparisons again prove the 116-byte data owner, four-byte BSS, and both
assertion COMDATs exact. The selected-object dry run reports no work.

The cumulative canonical outputs reproduce the isolated hashes exactly:

- `build/report.json`:
  `b8a1e632fcc38eb5f968404086d8c586b6ee7bd33548b0c0cf71e1e8e156bfb6`;
- `build/semantic_report.json`:
  `926f5872b4512ac9b51133b2e751907f996d0b1c2215a0e553866b802f1d01ca`.

The corrected clean snapshot is
`build/audit/error_geometry_initialize_corrected_head_manifest_20260826.json`,
696,503 bytes, SHA-256
`83c9e4a6868496d93b7efac9c8f1b6d57989ab82361d7d24b692e2c152e2021d`.
Its immediate no-build check is fully clean: dispose and initialize are the
two `still_exact` functions, `changed_nonexact` and `newly_exact` are empty,
and there are no failures or warnings.

The complete Halo-plus-libcmt graph is current and returns exit zero with no
work after the corrected replay. Semantic audit and progress reproduce the
full-validation counts above. Object admission remains zero candidates / zero
revocations, with only the inherited unrelated `shell_xbox` contradiction.
Parked validation remains three active / zero stale / zero invalid, all 179
tooling tests pass, and all 55 relevant JSON files parse.

Final tracked scope from the pre-wave base is exactly this source plus this
ledger, with no deletion. The exact eight-path Units/shared/protected guard,
the complete `source/units/**` tree, and all headers have zero intersection.
The cumulative worktree is otherwise clean before this additive closure
record. No push, amend, rebase, history rewrite, or worktree removal is
performed.
