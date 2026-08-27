# `weapons.obj`: tail-placed random wrapper rejection (2026-08-26)

## Outcome

The already authenticated `word random(void)` body again emitted January's
exact `_random` function on its first and only candidate compile. Placing the
definition at physical end of `source/items/weapons.c` reduced the earlier
section-index disturbance, but did not eliminate it: VC7 emits the new
external function before the trailing private/static COMDAT cluster. The
frozen whole-TU gate therefore still failed closed.

The candidate source was reverted exactly and the accepted baseline was
rebuilt. No implementation, matching credit, configuration change, semantic
exception, adjudication, or comparator recipe survives this wave.

## Frozen baseline and candidate

The clean baseline was commit
`9bfdbedcbb8113b95db19c8cb8e66db607f0ab5a` with source blob
`f6abe256f6b4becb78c6db398fd577e98a8cc6e3`. Its selected-unit manifest is:

- `build/audit/weapons_random_tail_baseline_20260826.json`;
- SHA-256
  `ff579542d345cc4930830b79c59f4f4e7bd3a653addfad591a7ad095f780f596`.

The immutable split target SHA-256 is
`5dd414f3f77e67aa7b6609f9131debfa8a9b3b23afab6a57fe4e790410cff2fe`.
The one candidate object is preserved at
`build/audit/weapons_random_tail_first_current_tu_20260826.obj`, raw SHA-256
`931a7c24bb118f5533a8d4c0de1b67ec98dd49403dbbd1ad403c6b028779dc7f`.

The source used the same narrow header-inline rename as the prior reviewed
wave, but placed the external wrapper after the final existing definition:

```c
word random(
	void)
{
	return seed_random(get_global_random_seed_address());
}
```

There was one normal Ninja candidate compile and no spelling, placement,
declaration, flag, or source-shape retry.

## Exact function proof

Direct hardened comparison reports `_random` strict exact:

- 15 meaningful / 16 padded bytes;
- normalized SHA-256
  `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0`;
- zero-addend `IMAGE_REL_I386_REL32` at `+0x01` to
  `_get_global_random_seed_address`;
- zero-addend `IMAGE_REL_I386_REL32` at `+0x07` to `_seed_random`.

The body is independently authenticated by the canonical inline definition in
`source/math/real_math.h` and exact ABI-compatible donor
`source/cseries/debug_memory:_local_random`. This experiment did not retest
the body; it tested only the new physical placement hypothesis.

## Why EOF placement still fails

The earlier January-order placement shifted 24 accepted owners. Physical EOF
placement reduces that set to the eight private/static owners VC7 emits after
the external-function group:

- `_code_000ec1a0`;
- `_code_000ec960`;
- `_weapon_put_away`;
- `_weapon_ready`;
- `_weapon_reloading`;
- `_weapon_reset`;
- `_weapon_set_state`;
- `_weapon_stop_reload`.

Direct target/candidate comparison proves all eight remain strict exact in
padded bytes, normalized bytes, relocation count/order/type, destinations, and
addends. The two previously nonexact owners `_code_000ec300` and
`_weapon_update` retain their prior code fingerprints but are also reported as
changed because their object-local section numbers move. Two anonymous
`.debug$F` records move with the same insertion, and the symbol inventory gains
the expected wrapper and callees.

The general regression gate intentionally fingerprints those numeric ownership
relations. Its only lawful transition recipe is the narrow stock-XDK D3DINLINE
selection change; there is no recipe for a general external COMDAT insertion.
The campaign forbids adding a comparator exception merely to admit this
function. Exact function bytes are therefore necessary but not sufficient, and
the patch is rejected.

## Restoration and stop rule

After the exact inverse source patch, the source blob is again
`f6abe256f6b4becb78c6db398fd577e98a8cc6e3`. The generated baseline object was
rebuilt once; its phase-local raw SHA-256 is
`1ee787449932583eae2cf2bc0b0b94d22ad9cd0992540464117bfbf4bc3ed5be`.
Raw VC7 object hashes are nondeterministic and are not the equality oracle.

The frozen manifest check is fully green after restoration: all 37 accepted
functions are `still_exact`, `changed_nonexact: []`, with no failures or
warnings. `git diff --check` passes and no source change remains.

Do not repeat physical placements of the same external definition. Reopen
only with a lawful source/ownership mechanism that adds the target owner
without renumbering existing private COMDATs, or with a repository-wide,
fail-closed ownership model designed and validated independently of this
recipient. Do not add a one-off comparator/adjudication exception. No push was
performed.
