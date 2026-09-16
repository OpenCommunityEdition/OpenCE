# `ima_adpcm.obj` Opus 5 next-150K lane, wave n1 (2026-09-15)

## Scope and provenance

- Translation unit `source/sound/ima_adpcm.c`; target `build/split/source/sound/ima_adpcm.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/next-150k-house-clean-20260915`, frozen baseline `ae10935da`, worker n1. No header, config,
  park or build edit; the real file was never modified.
- Assigned target: `_decompress_ima_adpcm_audio_data` (PARKED, scratch-only re-investigation).
- Ledgers read in full: `ima_adpcm_obj_jonas_complete_20260825` (one-shot candidates E01/E02, both
  rejected and reverted), `ima_adpcm_obj_jonas_historical_hybrid_20260830` (historical hybrid rejected;
  binding reopen clause quoted below), `ima_adpcm_obj_jonas_compressor_exact_20260831` (compressor closed
  from a Fable donor; decompressor parked fuzzy).
- Worker notes: `scratch/workers/n1_game_sound_ima_adpcm.md`; candidates in
  `scratch/workers/n1_game_sound_ima_adpcm/`.

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 2 | 1 | 0 |
| final real file (unchanged, guard passed) | 2 | 1 | 0 |

`git diff --stat -- source/sound/ima_adpcm.c` is empty. The single park drifts 0 (`scratch/parkcheck.py`:
`_decompress_ima_adpcm_audio_data` 416/2). Fake-match scan 0 leads. Owner census unchanged.
`branch_sweep.py source/sound/ima_adpcm` sees 8 unique blobs; the current tree (2/1/0) is the best, every
historical blob is worse.

## `_decompress_ima_adpcm_audio_data` — NOT LANDED; the park's reopen demand is now ANSWERED

Target `416 padded / 404 meaningful / 2 relocs / sha16 c308b60091d47921`, relocations at `+0xb2` and
`+0x113`. Ours `416 / 2 / 410 meaningful / 92f27a5e493fd0e4`, relocations at `+0xb5` and `+0x115`.

The 2026-08-30 ledger states: *"The decompressor may reopen only with fresh evidence explaining its
complete stack/register allocation change (`0x0c` target frame versus `0x10` baseline), not another local
declaration-order nudge."* That evidence is supplied below. It explains the frame gap — and it also shows
that the gap is not source-reachable by an admissible form, so the park stands on a firmer basis than
before.

### Frame census

| | January (`sub esp,0xc`) | ours (`sub esp,0x10`) |
| --- | --- | --- |
| `[ebp-4]` | `input` (`char const *`) | the `code` byte, at `[ebp-1]` inside this cell |
| `[ebp-8]` | `result` | `result` |
| `[ebp-0xc]` | compiler temp: the spilled `movsx` of `step_size_index` | `sample` |
| `[ebp-0x10]` | — | `input` |
| `[ebp+8]` (home of parameter `input_header`) | **`sample` (dword)** — `mov [ebp+8],edx` @0x55, `mov [ebp+8],eax` @0x7c, PIN stores @0xf2/0x101/0x10a, `mov dx,word[ebp+8]` @0x130/0x17f | `read_high_nibble` (one byte at `[ebp+0xb]`) |
| `[ebp+0xc]` | `input_data_size` (parameter, updated in place) | same |
| `[ebp+0x10]` | `output_samples` | same |
| `[ebp+0x14]` | `output_sample_count` | same |
| `[ebp+0x18]` | `state` — loaded into **EDI at 0x2a and never re-read** (`test edi,edi` @0x15f, `inc dword[edi+4]` @0x16c, `test edi,edi` @0x17a) | re-loaded at 0x164 and 0x17a; ESI is clobbered mid-loop at 0xd0 |

Spilled objects: January four (`input`, `result`, the `movsx` temp, `sample`); ours five (`input`,
`result`, `sample`, `code`, `read_high_nibble`).

**The extra dword is not a missing or extra local.** Both builds declare exactly the same objects with
exactly the same widths, verified instruction by instruction: `code` and `mask` are 8-bit
(`sar dl,4` / `and dl,0xf` / `mov bl,4` / `sar bl,1` in January, the same in ours), `step_size_index` is a
16-bit local sign-extended with `movsx` (`mov si,word[edi+0xa]`, `movsx ecx,si`, `mov word[edi+0xa],si`),
`sample` is a 32-bit long (`mov dword[ebp+8],0xffff8000` / `0x7fff`), `read_high_nibble` is 8-bit
(`mov al,1`, `sete al`).

The gap is an allocator *priority* difference seeded at 0x2a. January gives EDI to the `state` pointer for
the whole body and spills the `(long)step_size_index` CSE to `[ebp-0xc]`; we give EDI to that CSE, which
evicts `state` to memory and then forces `code` and `read_high_nibble` into memory as well, which in turn
pushes `sample` out of the `[ebp+8]` parameter home and into a fourth frame dword. A24 (parameter-home
offer) is satisfied in **both** builds — ours also reuses `[ebp+8]`, as the byte `[ebp+0xb]` — so A24 does
not discriminate between them; what differs is only which value wins that home.

Two secondary consequences confirm the reading: January's `lea ebx,[ebx]` at 0xca is a 6-byte alignment
NOP that puts the inner `do..while` head on 0xd0, while we spend those bytes on a real `mov esi,edx`
copy, because EDX rather than ESI receives `step_size_table[step_size_index]` (ESI still holds `state`
there); and our two extra `state` reloads plus the `code` / `read_high_nibble` spill-reload pairs are
exactly the six meaningful bytes by which we exceed the target.

### Relocation census

2 versus 2, both `IMAGE_REL_I386_DIR32` to `_step_size_table + 0` and `_step_size_adjustment_table + 0`.
No call and no global is missing or added. The `+3` / `+2` address deltas are entirely a consequence of the
byte differences above, i.e. a symptom of the allocation, not an independent defect.

### Shapes

| shape | change | result |
| --- | --- | --- |
| `d1.c` | inside `if (state)`, swap to January's statement order: `input_data_size -= state->sample_index>>1;` then `input += state->sample_index>>1;` | `416 / 2 / 7354fff350f9b59d`. Reproduces January's `sub ebx,eax` before `add edx,eax` (that block becomes equal) and trims the alignment from 191 to 187 lines, but the frame stays `0x10` and the ESI/EDI assignment is unchanged. Not exact, not usefully closer (32 aligned blocks versus 31). |

Fingerprint behind `d1`: January loads `input_data_size` at 0x4e *before* `step_size_index` at 0x51, then
emits `sub ebx,eax` at 0x5f before `add edx,eax` at 0x61, and defers both stores past the `result`
computation (`mov [ebp+0xc],ebx` @0x6e, `mov [ebp-4],edx` @0x71). Our source had the two `>>1` updates the
other way round. This is a genuine statement-order fact about January's source and should be carried into
any future candidate, but on its own it earns no credit (R11: a parked-body remeasure that is not strictly
closer).

Not re-tried, all previously rejected: E01 (compressor temporary-before-shift, family `0aab3da2`), E02
(`long result;` assigned after the header subtraction — measured inert on both relocation addresses), the
2026-08-30 historical hybrid, and any further local-declaration-order nudge (explicit do-not-repeat).
Only one shape was spent because the census shows the gap is not a missing statement, a wrong declared
width, a wrong loop, a wrong local or wrong control flow — the five categories brief section 5 asks a
structural census to discriminate.

## Reopen criterion (supersedes the 2026-08-30 clause)

The frame gap is explained and is *not* a missing local. Reopen only with a measured source lever that
makes VC7 keep a parameter pointer (`state`) enregistered across a loop in preference to a sign-extension
CSE, or that removes the `(long)step_size_index` CSE without a steering pointer alias (R16) or a named
width local without byte evidence. Carry `scratch/workers/n1_game_sound_ima_adpcm/d1.c`'s statement order
into any such candidate.

## Orchestrator proposals

None. No header, config, symbols.json or park change is required or requested by this wave.
