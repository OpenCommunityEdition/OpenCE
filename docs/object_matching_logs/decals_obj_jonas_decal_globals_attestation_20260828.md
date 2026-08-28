# `decals.obj`: attested `decal_globals` owner (2026-08-28)

## Outcome

The January BSS name used by `_decals_dispose_from_old_map` is now recovered
at image offset `0x435D70`. Regenerating the split object with that attested
name turns the function's sole relocation-identity residual into a strict
match. `source/effects/decals.obj` advances from 5/33 to 6/33 exact functions
and gains 112 padded bytes. The object remains incomplete.

No C source, compiler option, comparator, semantic exception, or admission
rule changed. The retained source blob remains
`774d1477c6fba42a3a4ecb91f9335c883f584f5d`.

## Attestation evidence

The provisional January owner `_bss_00435cb0` begins at `0x435CB0`.
`tools/campaign/attest_scan.py` isolated one naming gap:

```text
target _bss_00435cb0 + 192 <-> ours _decal_globals
source/effects/decals::_decals_dispose_from_old_map
```

The corresponding address is `0x435D70` (decimal `4414832`). This is not a
shape inference: January's same function relocates to that address while its
assertion string says `decal_globals`, and the source declares the matching
file-static owner `static void *decal_globals`. The frozen metadata change adds
only `_decal_globals` at that exact address in `config/symbols.json`.

After csplit regeneration, direct hardened comparison proves the complete
112-byte function exact: 13/13 relocation entries agree in address, type,
destination identity, and addend, and the normalized SHA-256 on both sides is
`0861c1b04c9be6538d2985f8487877932a77f95f98b361d32b14b862dca3dd3f`.
The strict gate reports 6 exact, 0 residual, and 27 unwritten functions.

Artifact sentinels for this replay are:

- regenerated January split object SHA-256:
  `f7f7b96b03e0e25328fc51a1780d7673cf82d78923122c58f91e8a7517186bfa`;
- rebuilt VC7 object SHA-256:
  `94f361d3e010f6bf948f8a6ce84a73c32385616ab737e9f5f7cadb826ae574bd`.

The surrounding January BSS is still an aggregate provisional owner, so this
wave claims function credit only: no standalone BSS-owner, data, or object
completion credit is taken. The prior plane-helper and color-packing ledgers
were read before admission. No protected Units, Vehicles, or Claude-active
path was modified.
