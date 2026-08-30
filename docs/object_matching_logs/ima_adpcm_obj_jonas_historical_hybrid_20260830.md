# `ima_adpcm.obj` historical-hybrid closeout (2026-08-30)

## Scope and authority

This documentation-only closeout starts from canonical commit
`d7ecde9dfa40e140323c932d9eaaddcd297f9d95` in isolated branch
`jonas/ima-adpcm-fresh-packet-20260830`. The production scope was limited to
`source/sound/ima_adpcm.c` and one genuinely fresh compressor spelling. No
decompressor spelling was authorized without new source or artifact evidence.

The baseline source blob is
`66244dc35af5020848642afb014d8f7939305d85`. The immutable split target has
phase-local raw SHA-256
`2d0ce87b2d0342a01359022b4719f9bb8825375083919c53c0b671dbe4099fa1`.
Strict authority is the per-function section fingerprint produced by
`tools/coff_compare.py`; raw object hashes are only local artifact sentinels.
Compilation used the repository's pinned XDK 3911 `CL.Exe` edge unchanged:
`/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the generated include set.

The inherited baseline remains one strict-exact function out of three and
`32 / 800` padded code bytes:

| Function | Target | Restored baseline | Relocations target / baseline | Target normalized SHA-256 | Restored normalized SHA-256 |
| --- | ---: | ---: | --- | --- | --- |
| `_compress_ima_adpcm_audio_data` | `346 / 352` | `346 / 352` | `+108,+219 / +108,+219` | `97e166836a7a6f05810477e6d722d209a4c07f5fb2e604756915f8b3de58fff3` | `9ef202fbd4db1aced4543636bb9f85efdcaca79b0edbb7b8d912535d3fc6eaf6` |
| `_decompress_ima_adpcm_audio_data` | `404 / 416` | `410 / 416` | `+178,+275 / +181,+277` | `c308b60091d47921aeb30f63188ec5317c64aa9fa52b9e53721774ecd34176f9` | `92f27a5e493fd0e401a7ee0c7c823e3040bddfdd6100cccb5b4f673be630d882` |
| `_byte_swap_bungie_ima_adpcm_header` | `24 / 32` | `24 / 32` | `+10,+15 / +10,+15` | `0831428713702f318c38875175e23ba63701439c51add602977f7e890ca3a832` | same |

The compressor's only baseline/target instruction divergence is still the
second reconstruction pass. January emits `mov esi,edi; mov ecx,esi; sar
ecx,3`; the restored baseline emits `mov ecx,edi; sar ecx,3; mov esi,edi`.
All inherited runtime data, relocations, and translation-unit ownership remain
the already-validated exact state recorded in
`ima_adpcm_obj_jonas_complete_20260825.md`.

## Provenance and artifact census

The census was completed before editing.

- Every reachable `ima_adpcm.c` history blob was inspected: current
  `66244dc3...`, earlier `bec0...`, `30c...`, `a3b...`, and skeleton
  `d156...`. The relevant history establishes two independently observed
  source orders but contains no exact donor.
- IMA-specific branches/worktrees were audited, including
  `exact-ima-adpcm-closeout`, `ima-adpcm-closeout-wave-20260821`,
  `ima-adpcm-complete-20260825`, `work/ima-adpcm-closeout`,
  `work/ima-adpcm-closeout-20260820`, `lanes/exact-ima-adpcm`,
  `lanes/ima-adpcm-compress`, `lanes/ima-adpcm-exact`, and
  `work/root-ima-adpcm-20260830`. Their source trees were clean or already
  represented by reachable history.
- All surviving base objects in those worktrees were fingerprinted. Compressor
  families included `524f...`, `1e89...`, `3f0f...`, `0aab...`, and the
  restored `9ef2...`; decompressor families included `1b7b...`, `6aa0...`,
  `fca3...`, and restored `92f2...`. None was strict exact. The byte-swap
  owner was exact throughout.
- Workspace-wide source and artifact searches covered the other Halo trees,
  Claude scratch `_gate_*.c` files, and the authenticated research-cache
  checkouts for HCEA, Stian, Pastudan, punp, sp00nz, and Aerocatia. Claude's
  surviving files reduce to the current, older, or skeleton repository blobs.
  The public checkouts contain no semantic implementation donor for these
  functions; unrelated XAudio and generic IMA implementations were excluded.
- Exact function-name, distinctive-string, repository, and broader public
  searches found no additional authenticated Halo/Bungie source. Thus the
  target disassembly and reachable repository history are the only positive
  provenance for this packet.

Claude's cumulative register census was also honored. It records eleven inert
compressor levers without preserving their spellings. Those undocumented
spellings were not reconstructed, and the already-rejected E01/E02 forms from
the 2026-08-25 ledger were not deliberately repeated.

## The single fresh candidate

Commit history shows an older compressor pass spelling in which
`temporary_step_size` is assigned before `sample_difference`, while the
current near-match source has the later declaration order and surrounding
control-flow shape. The sole candidate combined those independently attested
features:

```c
temporary_step_size = step_size;
sample_difference = step_size>>3;
```

This combination does not occur in reachable history. It was selected because
the target's first divergence likewise materializes the temporary before the
shift source. It is ordinary, defined, readable C89 and changes neither
behavior nor ownership.

The XDK syntax-only pass succeeded without object emission. Exactly one normal
candidate production edge was then run. The compiler canonicalized the fresh
source to the already-known E01 machine-code family:

| Function | Candidate meaningful / padded | Candidate relocations | Candidate normalized SHA-256 | Decision |
| --- | ---: | --- | --- | --- |
| `_compress_ima_adpcm_audio_data` | `339 / 352` | `+108,+212` | `0aab3da271db0e4ba7ab2e396dc3a497fefad58d71922dacb73abe22e7086bc3` | rejected |
| `_decompress_ima_adpcm_audio_data` | `410 / 416` | `+181,+277` | `92f27a5e493fd0e401a7ee0c7c823e3040bddfdd6100cccb5b4f673be630d882` | unchanged, still nonexact |

The candidate shortens the compressor by seven meaningful bytes and moves the
second table relocation seven bytes ahead of the target. It is therefore not
an admissible improvement even though its high-level source spelling is new.

## Disposition and durable boundary

The candidate was reverted with the exact inverse patch. The source again
hashes to baseline blob `66244dc35af5020848642afb014d8f7939305d85` and
has no tracked diff. A restoration build reproduces the baseline fingerprints
above; the local restored object raw SHA-256 is
`7e01476d38d08d4d6db6f258c7610fec0d88b91615ec20bb41d284cee2e120ec`.
There is no implementation credit and `ima_adpcm.obj` remains `1 / 3` strict
exact, `32 / 800` padded bytes.

Do not repeat this historical hybrid or any source spelling that exposes the
same temporary-before-shift dependency: under the pinned compiler it belongs
to rejected family `0aab3da2...`. Do not manufacture Claude's undocumented
eleven-row lever matrix. The compressor may reopen only with an authenticated
original-source donor or a genuinely new ordinary-C topology that explains
the target's exact six-byte schedule without shortening the function. The
decompressor may reopen only with fresh evidence explaining its complete
stack/register allocation change (`0x0c` target frame versus `0x10` baseline),
not another local declaration-order nudge.

Assembly, volatility, `register`, pragmas, intrinsics, barriers, attributes,
raw addresses/offsets, pointer or union puns, undefined behavior, synthetic
anchors, flag/tool substitution, object-byte patching, comparator exceptions,
and fuzzy credit remain forbidden.

## Verification

The post-revert focused compare reproduces all three baseline rows above and
the campaign board remains `1 / 3`, `32 / 800`. The focused regression gate
passes with `changed_nonexact=[]`, `newly_exact=[]`, and the byte-swap owner in
`still_exact`.

The complete verification set passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and `semantic_progress`;
  semantic scan reports `unit_errors=0` and progress reports Halo code
  `600230 / 1770166` bytes at the tested canonical baseline;
- object admission reports zero candidates, contradictions, or revocations;
- parked-function audit reports 12 active, zero stale, and zero invalid;
- `python -m pytest -q -p no:cacheprovider`: `212 passed`;
- fixed Units sentinel `_unit_preprocess_node_orientations`: strict exact at
  `1920` padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `git diff --check`: clean.
