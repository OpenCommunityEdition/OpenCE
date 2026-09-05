# `input_xbox.obj` public leaves: independent review, 2026-09-04

## Scope and verdict

This is an independent source, ABI, COFF-owner, and house-rule review of the
public-leaf packet following canonical commit
`d556ba5304eeb4ee663b7c81998925b36f2d5cd7`. January split COFF is the byte and
ABI authority; later HCEA input source is semantic corroboration only.

**Approve the final packet for integration**, subject to root's normal full
regeneration and stable sweep. It adds two strict public functions:

- `input_key_is_down`: 192 meaningful / 192 padded bytes;
- `input_get_key`: 97 meaningful / 112 padded bytes.

The exact gain is therefore **289 meaningful / 304 padded bytes**. All 18
inherited exact functions remain exact. `input_get_raw_data_string` is one
honest new fuzzy reconstruction; together with the three inherited poller
parks and three still-unwritten lifecycle functions, `input_xbox.obj` remains
partial and must not be marked wholly `Matching`.

Final reviewed source and artifact:

- `source/input/input_xbox.c`, SHA-256
  `b8195cdc8bb4e07791182c036e35ef96e6fd753d57f72f1f853e2e6c7f34cbd3`;
- `scratch/input-xbox-root-terminal-return-20260904.obj`, SHA-256
  `a85da70887081eca1c01d2db7fb7cbb082ad87ff2c7b96efe1aa561905751510`.

## Public ABI and semantics

The existing owner `source/input/input.h` declares all three functions with
their actual i386 cdecl ABI and project types. No caller-local declaration or
shared-header guess was added.

`input_key_is_down` returns the held-tick byte, not merely normalized true or
false. Suppression returns zero. The four generic modifier enum values return
the maximum tick count of their real left/right keys, while the default path
uses January's exact assertion and reads the requested key. The final source
uses a natural initialized result, assigns it in the switch, and ends in one
explicit return.

`input_get_key` checks the authenticated read/write indices, performs
January's bounded ring assertion, copies one four-byte `key_stroke`, advances
the read index, and returns the natural `boolean` result. Its nine relocations
authenticate the `input_globals` ring fields and assertion owners.

`input_get_raw_data_string` preserves January's two assertions, signed-short
size contract, literal pipe-delimited debug strings, `_snprintf` calls, four
gamepad iteration, genuine `HANDLE` array, and raw two-stick values. The
candidate and target have equal 240-byte extent and 14 relocations; the only
identified boundary is the reversed order in which VC7 materializes the
handles and raw-state global bases. No declaration-order or register search is
justified for a debug formatter, so the natural body is correctly parked.

All functions use semantic fields and enum constants, parameter-per-line
formatting, and explicit terminal returns. The packet contains no raw address,
offset cast, fake helper, representation pun, volatile/register steering,
inline or assembly control, optimizer barrier/pragma, synthetic anchor, or
nonsensical branch.

## House-rule correction that exposed an exact source

The worker's initial `input_key_is_down` candidate returned directly from every
switch arm and had no lexical terminal return. An unreachable fallback return
would have been formatting-only fake source and was correctly rejected during
review. The retained source instead expresses the routine naturally:

```c
boolean result = FALSE;

if (!input_globals.suppressed)
{
	switch (key_code)
	{
		/* each arm assigns result and breaks */
	}
}

return result;
```

This is ordinary single-exit source with meaningful suppression semantics, not
a byte-forcing variant. It unexpectedly reproduces January exactly at 192
bytes and 19 relocations. No further spelling, declaration, or branch-order
experiment was needed.

## Strict and fuzzy evidence

| Owner | Target / candidate padded bytes | Relocations | Normalized SHA-256 | Disposition |
| --- | ---: | ---: | --- | --- |
| `_input_key_is_down` | `192 / 192` | `19 / 19` | `c1ab3b0043e411c7e6695fb129837501c3bca12a2477d9497b5123f3eaf17805` | strict exact |
| `_input_get_key` | `112 / 112` | `9 / 9` | `a3b48c1ac1818bbd2c937a5f56fa988454af93d4a023201c03682be5fd73f436` | strict exact |
| `_input_get_raw_data_string` | `240 / 240` | `14 / 14` | target `3a95b06caa2d47b995d9c24ba0a7450d1ccbc091e6a4dc288cc6a986d8f76c5a`; candidate `1032cde83ef635472499ea3ef854f7bbd2877bc5fb0ef29aa4dc4f41c631557e` | honest fuzzy park |

Target and candidate relocation destinations/addends for the raw formatter are
the same. Its two `input_globals` base relocations occur at the same offsets
but point to raw-state/handle fields in the opposite order, matching the
documented materialization difference rather than a type or owner error.

## Owner and lifecycle boundary

The final scratch object has no candidate-only function or helper. Its natural
784-byte `.rdata` tables and sole 1,036-byte file-static `input_globals` BSS
owner remain unchanged from the already-published packet; the public leaves add
only January-owned assertion/format literals. The `point_from_line3d` guard
passes.

The private rumble flusher, input thread callback, and `input_initialize`
remain unwritten as a coherent lifecycle cluster. Ordinary static helper
drafts were correctly pruned when they had no real emitted caller. The DWORD at
`input_globals + 0x230` still lacks an authenticated semantic owner; no raw
offset, fake member purpose, external-linkage substitution, or synthetic caller
is used to bypass that blocker.

The focused result is **20 exact / four residual / three unwritten**. Root must
still reproduce the new raw-formatter park measurement in the production
object, validate all inherited parks, run the full stable exact-set comparison
and tooling suite, and inspect the final emitted owner census before publishing.
