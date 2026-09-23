# objdiff v3.3.1 mis-scores MSVC data relocations

*2026-09-22, Lane C. Established independently three ways: by reading objdiff's
source at tag v3.3.1, by black-box perturbation of January's own object files,
and by auditing this project's own adjudication ledger. All three agree.*

## The defect

A data section's objdiff score is decided by its **relocations**. Two
relocations are compared by `reloc_eq` in `objdiff-core/src/diff/data.rs`, which
first tries `symbol_name_matches()`. That helper assumes a `$` in a symbol name
is the Metrowerks `symbol$1234` unique-ID separator:

```rust
if let Some((prefix, suffix)) = left_name.split_once('$') {
    if !suffix.chars().all(char::is_numeric) { return false; }
    ...
}
```

Every MSVC mangled string literal contains `?$AA` — the encoded NUL. For example
`??_C@_0P@LFOALBOL@objects_update?$AA@`. The suffix after the first `$` is
`AA@`, which is not numeric, so **the helper returns false for two
character-for-character identical names**.

`reloc_eq` then falls through to `address_eq`, which requires the target literal
to sit at the same offset inside the *synthetic combined* `.rdata` that
`report generate` builds (it forces `combine_data_sections: true`, and
`do_combine_sections` concatenates every same-named section in COFF
section-table order, padding each to `max(align, 4)`). csplit lists January's
`.rdata` COMDATs in image order; CL emits ours in its own order. So the address
test fails too, and a byte-perfect relocation is scored as a mismatch.

That costs the section twice. Relocated bytes are counted twice in
`diff_data_symbol`

    match_ratio = (bytes_ratio*len + relocs_ratio*reloc_bytes) / (len + reloc_bytes)

and, worse, the whole-section byte fallback in `diff_data_section` is gated
behind `if all_left_relocs_match`, so it never runs. Since `report.rs` credits
data all-or-nothing per section, one mis-compared relocation forfeits the
entire section.

The code path is immune: `diff/code.rs::reloc_eq` compares names with plain `==`.

Fixed upstream in **objdiff v3.6.0** (2026-01-25), which added
`get_normalized_symbol_name` in `obj/read.rs`; it returns `None` for
`??_C@...?$AA@` so `symbol_name_matches` falls back to exact equality.

## Reproduction

Copy January's `objects.obj`, swap only the two `.rdata` COMDAT section headers
for `_object_memory_release_procs` (24 B) and the `objects_update` literal
(15 B), fixing symbol section numbers; change nothing else. January against
itself scores `.data` 100.0; January against that header-swapped copy scores
`.data` 99.739235 — the exact production number. Swapping two COFF *symbol
table* entries instead leaves it at 100.0, so symbol-table order is inert.

## Scale, measured at HEAD

40 data sections / **32,262 bytes** have identical size, identical raw bytes and
an identical relocation multiset, and would score exactly 100.000000 under an
exact-name rule. Of those:

* **37 sections / 29,582 bytes are already credited** by
  `config/semantic_data_matches.json`. No progress bytes are being lost there —
  but they are credited as *adjudications*, and the standing rule is that
  adjudicated does not count as closed.
* **3 sections / 2,680 bytes are genuinely uncredited**:
  `source/ai/actions .rdata` (2,388), `source/ai/ai_communication .data` (276),
  `source/ai/actions .data` (16).

## Recommendation

Upgrading the differ to >= v3.6.0 is the root-cause fix. It would recover the
2,680 uncredited bytes and let roughly 37 adjudications be retired, converting
29,582 bytes from adjudicated to genuinely exact. That is a shared build-tool
change and needs an owner decision, and it must be measured board-wide first,
because a differ upgrade re-scores everything.

Do **not** chase these sections with source edits. The only source-side lever is
which `.rdata` COMDATs a TU emits and in what order, so that a literal lands at
January's combined offset — and the target order is csplit's, i.e. the linked
image's layout, which is a link-time property. Reordering or inventing literals
to shift an offset fails the strip test.

Do **not** add new adjudications for them either; that papers over the
instrument. Two entries were retired on 2026-09-22 for the opposite reason —
`cheats .bss` and `tif_version .data` became genuine matches, so their
adjudications were deleted at no cost to credited progress.

## Triage rule this establishes

Run the January-against-January self-diff **before** spending any wave on data,
and **regenerate it whenever the differ changes**. Rewrite `objdiff.json` with
`base_path = target_path` and absolute paths, drop `custom_make`/`build_target`,
then `objdiff-cli report generate`; it takes 0.12 s over all 833 units.

Under v3.3.1 it capped 47 sections / 87,889 bytes. **Under v3.6.0 it caps
nothing at all** - 0 sections, 0 bytes. Every one of those "permanent" ceilings
was this same defect: a January-side string COMDAT comparing unequal to itself,
so even a byte-identical copy could not reach 100. The campaign treated 87,889
bytes as unreachable on the strength of a number produced by the broken
comparator. Do not quote the old figure, and do not kill a target with a stale
ceiling report.

## Resolution

Landed 2026-09-22: `sln.objdiff_tag` pinned to v3.6.0, and the 40 adjudications
the fix makes redundant retired in the same commit. Board effect, measured on
identical objects: raw data +34,886, raw code and functions unchanged, zero
units losing anything, credited data +5,296, adjudications 43 -> 3. v3.8.1 was
measured and rejected - same data gain, but it loses 6 bytes of code.
