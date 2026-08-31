# `ima_adpcm.obj` compressor closure and decompressor park (2026-08-31)

## Result

The Fable/Claude campaign supplied a natural source correction for
`_compress_ima_adpcm_audio_data`.  It is now strict exact, moving the object
from one to two exact functions while the translation unit correctly remains
`NonMatching` because the coherent decompressor is still a compiler-scheduling
residual.

| Function | Meaningful / padded bytes | Relocations T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_compress_ima_adpcm_audio_data` | `346/352` | `2/2` | `97e166836a7a6f05810477e6d722d209a4c07f5fb2e604756915f8b3de58fff3` | strict exact |
| `_decompress_ima_adpcm_audio_data` | `404/416` | `2/2` | target `c308b600...`, base `92f27a5e...` | fuzzy-parked |
| `_byte_swap_bungie_ima_adpcm_header` | `24/32` | `2/2` | `0831428713702f318c38875175e23ba63701439c51add602977f7e890ca3a832` | strict exact |

The compressor's second nibble-reconstruction pass now reloads
`step_size_table[step_size_index]` into `temporary_step_size` and derives
`sample_difference` from that value.  This is meaningful IMA ADPCM state
reconstruction, not an inert lifetime or byte-control trick.  It produces
January's exact instruction order and preserves the two table relocations at
`+108` and `+219`.  The source came from Fable commit
`c8ff585ee941bd8454e991d7512b5b36ba0f161a` and was transplanted alone rather
than importing that multi-object commit wholesale.

## House-rule cleanup

The public header now owns the two public structures and all three public
prototypes.  The translation unit retains only its private byte-swap wrapper
structure.  Raw clamp bounds were replaced by `SHORT_MIN`/`SHORT_MAX`, table
sizes use semantic constants, and the address placeholder `_data_00316a7c`
was replaced with `bungie_ima_adpcm_header_bs`.  That global name is inferred,
not claimed as PDB-recovered: it follows the authenticated sibling
`*_bs` byte-swap convention and the embedded definition string
`"bungie ima adpcm header"`.  The complete 44-byte owned data section remains
strict exact under the semantic name, with two relocations and normalized
SHA-256 `69aafaeb175e6fba4e4bf2f0ba927ecaafc354ba050e67f3c18e882c9fd8312a`.

All cleanup changes are byte-neutral outside the compressor closure.  No
inline assembly, volatile access, barrier, pragma, forced inlining, dead code,
undefined behavior, or manual bitstream construction was introduced.

## Honest fuzzy boundary

The decompressor has the target's exact padded envelope and table targets, but
January stores decremented `input_data_size` before materializing
`sample_count * 2`; the current VC7 schedule does the materialization first.
The step-size relocations consequently land at target `+178/+275` versus base
`+181/+277`.  Earlier bounded experiments could not change only this packet.
The readable implementation is therefore recorded in `config/parked.json`
and left fuzzy rather than accepting synthetic source.

Validation used `tools/campaign/gate.py source/sound/ima_adpcm --all`, direct
`tools/coff_compare.py` comparison of all functions and the named data owner,
the full semantic/progress build, parked-function validation, and the tooling
test suite.
