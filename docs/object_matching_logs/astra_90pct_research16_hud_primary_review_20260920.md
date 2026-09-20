> Research-only packet16: zero production change and zero exact credit. Validated production remains wave13.

# Packet 16: independent HUD primary-source review

**Disposition: hold reconstruction.** Fresh primary evidence supports several historical source details, but does not support the gain-critical rectangle store order selected by the old 24-permutation sweep. This lane produced zero source shapes, zero C compiler calls, zero production changes, and zero matching credit. Ownership is released.

`_hud_messaging_update` represents 2,922 meaningful bytes. Root's frozen January object is 2,928 padded bytes / 137 relocations; current code is 2,880 / 136. This review rehashed the source, both objects, supplied PE and HCEA PDB against `hud-messaging/current-and-inputs.json`; all remain unchanged. Historical MF2/hm1 normalized exactness is documented in the September 14 w1/w3 reports, but the candidate source was unavailable after root's bounded search and was not reproduced here.

## Decisive new evidence

The supplied PE's actual RTC descriptor at `0x62ddf4` identifies two 8-byte `bounds` locals at EBP−0x8ac and EBP−0x9e0. With `rectangle2d` fields at y0=0, x0=2, y1=4, x1=6, direct raw-byte decoding gives:

| Bounds | x0 store | x1 store | y0 store | y1 store | Observed order |
|---|---|---|---|---|---|
| State | 0062d062 | 0062d079 | 0062d084 | 0062d098 | x0, x1, y0, y1 |
| Message | 0062db04 | 0062db1b | 0062db26 | 0062db37 | x0, x1, y0, y1 |

The w1 report explicitly records **y0, x0, x1, y1** as the winner after measuring all 24 orders. The fresh unoptimized primary stores therefore differ from that winner. They cannot independently authenticate replaying the old source choice. This is a difference between the observed later instruction sequence and the reported candidate order; it is not proof of January lexical source order, semantic invalidity, or impossibility of matching.

## Independently supported partial facts

- **256-character buffer:** HCEA DIA records `wchar_t[0x100], buffer`; the actual supplied-PE RTC descriptor gives `buffer` 512 bytes at EBP−3048. The decompiler's larger apparent array is not the primary extent.
- **Saved state pointer:** 0062ce56–0062ce5e forms datum+0x230 and saves it at EBP−0x8c4. Custom icon flag/info reads at 0062d448, 0062d472 and 0062d48c reuse that pointer. This supports the specific saved access path, without endorsing the historically rejected uniform-pointer rewrite.
- **Separate formatted-item draw:** 0062db87 tests item against NONE and branches to the plain-message arm. Message-offset selection precedes the item lookup. The formatted-item, unformatted-item, and plain-message paths have distinct calls at 0062dd01, 0062dd1f and 0062dd40, all to the same draw thunk. Short string-index and scale consumption also remain explicit in the raw word stores and sign extensions.
- **Other operation ordering:** The raw windows support objective-start computation before nested flash-color/conversion calls, the color copy between time retrieval and elapsed-time subtraction, and the negative-first help-color branch.

HCEA reports `custom_color` and `color` at the same frame offset 0xA0, and `cursor_bounds` and a later `bounds` at 0x50. That supports disjoint storage lifetimes. The flattened dump does not recover exact lexical braces, all historical local names, or declaration placement. Merely finding plausible block scopes does not authenticate every gain-critical choice.

## Evidence and reopening boundary

Reproduce the bounded verification with `python -B scratch/astra-wave16/hud-primary-review/review.py`. `review.json` contains hashes, the full ten-record RTC census, exact store bytes/offsets, and input-restoration checks; `primary-windows.raw.asm` contains the selected independently decoded raw windows. Root's original Ghidra/raw and DIA captures remain under `scratch/astra-wave16/hud-messaging/primary/`. Historical sources are `docs/object_matching_logs/hud_messaging_obj_opus5_150k_w1_20260914.md` and the corresponding w3 report.

Reopen with independent evidence for the gain-critical January rectangle operation/staging and any required lifetime distinction, or a complete admissible reconstruction from authenticated facts. Do not replay the 24-order sweep. The separate relocation-alias question may use existing `objdiff-coff-shape` support; this report makes no comparator impossibility claim. Same-source-revision correspondence across January, the 2011 HCEA PDB and the supplied 2020 executable remains unverified. IDA_AUTOMATION_UNAVAILABLE; this review used existing captures plus direct PE bytes and performed no source or header mutation.
