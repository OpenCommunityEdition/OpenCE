# `xbox_texture_cache.obj`: natural locked-callback boundary, 2026-09-05

## Disposition

Admit the one-shot natural source reconstruction of
`static boolean texture_cache_locked_block_proc(long)` as a fuzzy function and
park it `unclassified`. It restores 51 meaningful / 64 padded source bytes but
earns no strict code or data credit. The unit moves from 13 exact / 1 residual /
11 unwritten to 13 exact / 2 residual / 10 unwritten. All 13 inherited exact
functions and the existing `texture_cache_steal_memory` park are preserved.

This is not a new exact-match spelling attempt. It is the same ordinary boolean
source topology measured once in the 2026-08-30 wrapper wave, reconsidered under
the current policy that retains credible complete fuzzy functions. There was no
second source form, return-type experiment, declaration-order search, forced
emission, or compiler-control probe.

Do not mark the object `Matching`. Ten target code owners remain unwritten, the
mapper table/schema and debug-storage boundaries remain unresolved, and the
three inherited XDK select-any tables remain candidate-only data.

## Source and semantic evidence

The proposed source is:

```c
static boolean texture_cache_locked_block_proc(
	long block_index)
{
	struct xbox_texture_cache_texture *texture = datum_get(
		xbox_texture_cache_globals.textures,
		block_index);

	if (texture->loaded &&
		!IDirect3DBaseTexture8_IsBusy(&texture->hardware_format))
	{
		return FALSE;
	}

	return TRUE;
}
```

January independently fixes every operational access:

- the datum array is `xbox_texture_cache_globals.textures`;
- `loaded` is the byte at texture-entry offset `+0x4`;
- the embedded `D3DBaseTexture` receiver begins at `+0xC`;
- `IDirect3DBaseTexture8_IsBusy` naturally lowers to the target
  `_D3DResource_IsBusy@4` call;
- the result is false exactly when the entry is loaded and the resource is not
  busy, and true otherwise.

The complete 0x20-byte local texture-entry type and its offset/size assertions
already exist in `source/cache/xbox_texture_cache.c`; this packet adds no new
layout, header, cast, raw offset, or representation view. `texture_cache_new`
is the genuine existing caller and passes the callback to `lruv_new`. The
callback's semantic name and private linkage are authenticated by the HCEA PDB
and already represented by target metadata at January file offset 1763392 with
`static:true`. The later HCEA body uses a different lock-byte policy, so it is
used only for name, linkage, parameter, and byte-return corroboration—not as
Xbox body authority.

The source follows the current house rules: meaningful private name, ordinary
`boolean`/`long` types, declaration and initialization together, owning-TU
prototype, explicit returns, and no inline/assembly/pragma/barrier/volatile/
register trick, address-style identifier, fake branch, helper anchor, or
undefined behavior.

## Return ABI correction to the 2026-08-30 ledger

The old
`xbox_texture_cache_obj_jonas_xdk_device_wrapper_pair_20260830.md` conclusion
that January proved a full-width callback return contract was incorrect and is
explicitly retracted by this review. January's callback happens to materialize
the true case with `mov eax,1` and returns the known-zero IsBusy `EAX` directly
on its false path, but that emitted shape is not the callback ABI.

The owning declaration in `source/memory/lruv_cache.h` is:

```c
typedef boolean (*lruv_locked_block_proc)(
	long block_index);
```

`boolean` is the project's `byte` typedef. More importantly, every January
indirect use of the slot at `struct lruv_cache + 0x24` consumes only `AL`:

| January function | indirect call | result use |
| --- | ---: | ---: |
| `_lruv_cache_get_page_usage` | `+0x4F call eax` | `+0x54 test al,al` |
| `_lruv_debug_to_file` | `+0xED call eax` | `+0xF2 test al,al` |
| `_lruv_block_new` | `+0x1A7 call eax` | `+0x1AC test al,al` |
| `_lruv_block_new` | `+0x2EC call eax` | `+0x2F1 test al,al` |
| `_lruv_block_new` | `+0x39D call eax` | `+0x3A2 test al,al` |

The complete saved-target scan is
`scratch/lruv_locked_callback_abi_audit_20260905.py/.json`; it finds all five
slot loads/calls and confirms all five byte tests. The frozen January LRUV
object SHA-256 is
`e7a851cddf271f82ab6fc11a3e8872cbcd4c6965be15ca49ba507a594dcc8911`.
Thus the candidate's `xor al,al` / `mov al,1` is ABI-correct. The residual is
instruction selection in boolean materialization, not a public/private ABI
defect.

## Measurements

| | January target | natural candidate |
| --- | ---: | ---: |
| padded bytes | 64 | 64 |
| meaningful bytes | 51 | 51 |
| relocations | 3 | 3 |
| normalized SHA-256 | `9f883aeaa9d5a930f38b79b2960d634b647d971b5fcd92675174941d340b1f1d` | `ef0debbd170c841a4c89b83eb0363369278f09c33897019f50199a9b888efbf4` |

All relocation addresses, types, identities, and addends agree:

- `+0x08 DIR32 -> xbox_texture_cache_globals.textures`;
- `+0x0F REL32 -> datum_get`;
- `+0x22 REL32 -> D3DResource_IsBusy@4`.

Both owners have `.text` flags `0x60501020`, selection 1, static storage class
3, function type `0x20`, and value zero. Objdiff reports **95.0%**. The only
instruction differences are the candidate's explicit `xor al,al` and byte
`mov al,1` versus January's direct known-zero return and dword `mov eax,1`, plus
the resulting two-byte branch displacement. The fail-closed blocker classifier
returns `UNKNOWN / unclassified`; no recipe or exact credit is claimed.

Gate commands:

```text
python tools/campaign/gate.py source/cache/xbox_texture_cache --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/xbox-texture-cache-locked-callback-baseline-20260905.obj

python tools/campaign/gate.py source/cache/xbox_texture_cache \
  --edits scratch/xbox_texture_cache_locked_callback_natural_20260905.edits.json \
  --all --forbid-emitted-symbol _point_from_line3d \
  --out scratch/xbox-texture-cache-locked-callback-natural-20260905.obj
```

The first reports 13/1/11 and the second 13/2/10. The hard forbidden-symbol
guard passes both.

## Complete saved-object ownership audit

The frozen baseline has 14 code / 15 non-code / 0 COMMON owners. The candidate
has 15 code / 15 non-code / 0 COMMON owners. The sole added code owner is
`_texture_cache_locked_block_proc`; no code or non-code owner is lost, no
inherited owner changes, and no new non-code or COMMON owner appears. The
candidate has no code owner absent from January.

The existing exact `_IDirect3DBaseTexture8_IsBusy@4` helper is byte-,
relocation-, property-, and ownership-identical before and after. No
`_point_from_line3d` or other helper appears. The three known inherited
candidate-only SDK tables are unchanged:

- `_D3DTEXTUREDIRECTENCODE`;
- `_D3DSIMPLERENDERSTATEENCODE`;
- `_D3DPRIMITIVETOVERTEXCOUNT`.

The complete runtime fingerprint has exactly one inherited record change:
`_texture_cache_new` relocation `+0x3B`, type 6, addend zero, still names and
resolves to `_texture_cache_locked_block_proc`. Its target-symbol record changes
from an undefined external declaration (section null, storage 2) to the real
local definition (`.text|owners=_texture_cache_locked_block_proc`, storage 3).
Every other caller byte, relocation field, owner property, and local-label-
normalized runtime record is unchanged. This is the expected
undefined-to-static-defined transition, not caller drift.

Reproduce with:

- `scratch/xbox_texture_cache_locked_callback_owner_audit_20260905.py/.json`;
- `scratch/xbox_texture-cache-locked-callback-report-20260905/`;
- `scratch/xbox_texture_cache_locked_callback_classifier_20260905.json`.

Frozen artifact SHA-256 values:

- baseline object:
  `9458ea7a9d9c1afd3e6133059ed2109dd986783b8aa521fc6056eea16e1f956a`;
- natural candidate:
  `3f3ee662c0f5d3ae7282c37c92ccd60e2f3c66670574820dcf8c8e5f269aaac8`;
- frozen target:
  `fe0ee4fa6fb54805a7bab062d5e296c870c2feb3028ca86fd0b72ad95b2ced70`;
- edit manifest:
  `38ee03f8637cdc793acf3cd296b1802920cf18776d4d4b877ec88097c02628ec`;
- transformed source text:
  `79c11702bad4bc8b84878a321a9544f297b01cc331701e29a471d92cd45c3d8a`.

## Frozen behavior check

`scratch/xbox_texture_cache_locked_callback_behavior_20260905.py` executes the
immutable January and candidate callback sections in Unicorn with only the two
genuine callees stubbed: `_datum_get` as cdecl and
`_D3DResource_IsBusy@4` as stdcall. It patches relocations only in emulator
memory and never changes either input object.

For each object the harness runs every possible loaded byte (`0..255`) against
seven busy results: zero, one, and five noncanonical full-width nonzero values,
including values whose low byte is zero. That is 1,792 cases per object and
3,584 total. Every case passes the byte-return contract:

- loaded zero always returns `AL=1` and never calls resource-busy;
- every nonzero loaded byte calls resource-busy exactly once;
- loaded nonzero with busy zero returns `AL=0`;
- loaded nonzero with any tested nonzero 32-bit value returns `AL=1`.

The stubs assert the exact datum-array pointer, signed-long argument bit
pattern, and `&texture->hardware_format` receiver. Each run also proves the
complete 16-byte global and 32-byte entry images are unchanged, the return
address and argument bytes remain intact, `ESP` is balanced, and
`EBX`/`ESI`/`EDI`/`EBP` are preserved.

The harness deliberately records rather than equates full `EAX`: January
canonicalizes it to zero/one, while the byte-returning candidate leaves
non-contract high bits in 1,282 cases. All five real January LRUV consumers
test only `AL`, so this is expected ABI-safe evidence rather than a hidden
equivalence assumption.

- harness SHA-256:
  `a8bae6d0a6294b1981b07dee77835577651955adea558450a25541693aee2d16`;
- result SHA-256:
  `7be920ba281838a414e729efbc8b52b88374fead8bb3a1c928757969e891db4e`;
- result:
  `scratch/xbox_texture_cache_locked_callback_behavior_20260905.json`.

## Integration boundary

Apply only
`scratch/xbox_texture_cache_locked_callback_natural_20260905.edits.json`.
No header or target-symbol edit is required. Add the measured park from
`scratch/xbox_texture_cache_locked_callback_park_proposal_20260905.json`, then
run root-owned full Ninja, stable-owner sweep, park validation, and tests. The
minimal handoff is
`scratch/xbox_texture_cache_locked_callback_production_20260905.manifest.json`.

Expected accounting is **+51 meaningful fuzzy source bytes, +0 strict bytes,
+0 data bytes, and no object-admission change**.

## Canonical integration verification

Root applied the minimal source edit and95.0% park. The actual production
object SHA-256 is
`f778b414d312336da0b1572a4c4a1bab27eb916b10f3c8ef5bd69986e5fdaf23`.

All14 inherited code and15 non-code owners are unchanged, with no lost owner,
new data, COMMON, or helper. The complete runtime audit finds only the expected
callback-definition transition in texture_cache_new's DIR32 at+0x3B: unchanged
name/addend/receiver identity, undefined external storage2 becoming the genuine
static definition storage3. No other inherited record changes.

The unchanged frozen behavior harness passes all3,584 target/actual-object
runs, including every loaded-byte value and the noncanonical nonzero busy
results. It still compares AL, the independently established callback contract,
and records high-EAX differences rather than silently discarding them.

| Actual integration evidence | SHA-256 |
| --- | --- |
| `scratch/xbox_texture_cache_locked_canonical_owner_audit_20260905.json` | `6acbfaa0b035bda00fcd77bf023b23d873c66796f83ebb2b03d90047636de643` |
| `scratch/xbox_texture_cache_locked_canonical_runtime_audit_20260905.json` | `f21c80b63d83329354a88b522fc1efa726f21fd1d44122e719adede095d31600` |
| `scratch/xbox_texture_cache_locked_canonical_behavior_20260905.json` | `e2843da9bcb15d56023cad16f56acc8ec56c59c5db1efde3da788039e59a5988` |

Full Ninja passes. The stable sweep `scratch/main-texture-canonical-first-20260905.json`
retains6,217/8,245 strict owners with zero gains or regressions versus the
restored baseline. Units remains189/189 exact; both protected Bitmap hashes
are unchanged. All280 parks validate (zero stale/invalid); all288 tests and26
subtests pass. Fresh admission finds0 candidates/0 contradictions/5 recorded
rejections/0 revocations. The production meaningful totals remain906,391 code
bytes,6,171 functions,2,030,868 data bytes,391/833 Matching objects. No exact
code, data, or whole-object gain is claimed for this batch.
