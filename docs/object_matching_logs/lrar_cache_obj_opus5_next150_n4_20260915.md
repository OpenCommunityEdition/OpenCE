# `lrar_cache.obj` — opus5 next-150K lane, wave n4 (evidence-led census pass, 2026-09-15)

Baseline = final real-file gate (`tools/campaign/gate.py source/memory/lrar_cache --all
--forbid-emitted-symbol _point_from_line3d`, guard passes): **11 exact / 1 residual / 0 unwritten**.
Park drift 0, fake scan 0 leads, `git diff --stat -- source/memory/lrar_cache.c` **empty**. No credit claimed.

The single non-exact row is the parked `_lrar_allocate`:
target 784 padded / 27 relocs / sha16 `a34c14dad0afc2a3`, ours 1056 / 47 / `8a2fe453345af3d0`.

Worker notes and the complete scratch candidate: `scratch/workers/n4_lrar_cache.md`,
`scratch/workers/n4_lrar_cache/v1.c`, `v1.obj`, `secdis.py`.

## 1. The wave's leverage census is an inline-expansion artifact, not a missing call

`relocdiff --count-by-target` confirms the dispatched delta exactly: January owns `_lrar_get_block` ×2 that we do
not; we own `_lrar_verify_block` ×2, `_lrar_verify_cache` +2, `_csprintf` +2, `_temporary` +2, `_display_assert` +4,
`_system_exit` +4, the file literal +4, `"…appears to be corrupt"` +2 and `"block_index>=0 && block_index<ca…"` +2.

Every one of those is the **expansion of the same call** — `lrar_get_block` — at two sites January leaves out of
line (`+0x83`, the `else` arm of `if (cache->last_block_index == NONE)` inside the retry `for(;;)`; and `+0xCF`, the
pre-loop eviction fetch inside `if (block_index != NONE)`). `alndiff` aligns 262 target vs 345 candidate
instructions with exactly three replace regions: those two expansions plus the `mov ax` / `mov si` register pick at
`+0x70`, which is downstream of January owning EAX for the helper's private ABI (`+0xCC: mov eax, ebx;
call _lrar_get_block` — block_index in EAX, cache in EDI). **No statement, branch, frame slot, store or literal
exists on one side and not the other.** There is nothing to recover from January's bytes.

New supporting measurement (whole-object `tinfo`, not in any previous lrar ledger) — January's per-caller decision:

| January caller | `lrar_get_block` | `lrar_verify_cache` | `lrar_verify_block` |
| --- | --- | --- | --- |
| `_lrar_block_address` (96 B) | inlined | called `@0x0b` | called `@0x4c` |
| `_lrar_deallocate` (112 B) | inlined | called `@0x0b` + `@0x12` | called `@0x53` |
| `_lrar_flush` (320 B) | inlined | own call `@0x0c` + inlined copy | inlined |
| `_lrar_allocate` (784 B) | **2 out-of-line calls + 1 full inline** | own call `@0x11` | inlined once |

January inlines the getter into every small caller and at the doubly nested loop site, and declines it only at the
two retry-loop sites behind an `if`, inside the largest caller. That is a caller-size/frequency budget effect, i.e.
the recorded park premise (`tu-context-optimization`, refined by w2/n2 to an inline-frequency boundary) is
**confirmed**, not refuted. Per the wave directive the target stops here: the exhausted park-premise sweep
(w2's nine shapes, n2's tier model and `while (!found)` = 912/37) was not re-run, and no register-allocation or
x87-ordering shape was spent.

## 2. NEW structural finding: the private helpers are defined in the wrong order

This is a genuine source defect that no lrar ledger had identified (the w2 ledger recorded the opposite conclusion).
It is byte-inert for `_lrar_allocate`, so it is reported as a proposal rather than landed.

Our source order is `lrar_verify_block`, `lrar_verify_cache`, `lrar_get_block`.
**January's order is `lrar_get_block`, `lrar_verify_block`, `lrar_verify_cache`.**

**Proof 1 — assert line anchors.** The `match_assert`/`match_vassert` calls carry January's own line numbers:
`lrar_get_block`'s index assert is `0x16E` = line **366**; `lrar_verify_block`'s vassert `0x186` = **390**;
`lrar_verify_cache`'s vassert `0x199` = **409**. (`lrar_allocate`'s are `0x111`/`0x11C` = 273/284, so all three
helpers follow it, as in our file.) The getter is therefore ~24 lines before `verify_block`, which is ~19 lines
before `verify_cache` — the exact inverse of our order.

**Proof 2 — emission order under the deferred-compile law** (laws_w2 A22 EMISSION ORDER: source order, deferring
callers of later statics). COFF section index order:

| January `build/split` | HEAD `build/base` | `v1.c` (reordered) |
| --- | --- | --- |
| lock, unlock, unlock_block, verify_block, verify_cache, **new, dispose, get_block**, flush, allocate, block_address, deallocate | lock, unlock, unlock_block, verify_block, verify_cache, **get_block, new, dispose**, flush, allocate, block_address, deallocate | **identical to January** |

With the getter defined first it is itself deferred (it calls the two later statics) and is released in the second
pending pass — after `new`/`dispose`, before `flush`/`allocate`/`block_address`/`deallocate`. HEAD, where the getter
is defined last, compiles it in source position and emits it before `new`/`dispose`. `v1.c` reproduces January's
section order exactly, which is an independent confirmation of the line-anchor reading and of the deferral law.

**Effect:** zero byte change — all 12 rows identical to HEAD (`_lrar_allocate` sha `8a2fe453345af3d0`),
11 EXACT, park drift 0, guard passes, fake scan 0 leads, no new owner (pure reordering).
Consumers: `source/memory/lrar_cache.c` only (the six helpers are static; `lrar_cache.h` declares none of them and
has exactly one includer). Value: source-order fidelity and a prerequisite for any future whole-object byte claim;
it also retires the w2 note that "January section order for statics is not source order here".

Not landed because brief section 8 / w3 E18 require an empty file diff in a unit where nothing lands.

## 3. Carried-forward proposal (w2, still absent from HEAD): authentic private names

`scratch/atlas/lrar_cache.obj.txt` (exact tier) supplies the authentic names; `config/symbols.json` still carries the
invented ones:

| Address | symbols.json (line) | Atlas name |
| --- | --- | --- |
| `0x0010bfe0` | `_lrar_verify_block` (4627) | `_verify_lrar_cache_block` |
| `0x0010c040` | `_lrar_verify_cache` (4628) | `_verify_lrar_cache` |
| `0x0010c280` | `_lrar_get_block` (4631) | `_get_lrar_cache_block` |

Edit the three lines in place (keep `"static": true`) and rename the identifiers in the same commit; w2's alias gate
stayed 11/1. These names match the sibling `lra_cache.obj` family (`verify_lra_cache_block` / `verify_lra_cache`).
If this and section 2 are both taken they must land as one commit — both touch the same three definitions.

## 4. Rejected / not repeated

- The only exact `_lrar_allocate` blob in history (`14e4e8adf`) is the revoked `__declspec(noinline)` +
  hand-expanded-getter construction, and w2's `if (size >= 0)` probe is a fixed redundant condition. Both remain
  prohibited; neither was rebuilt.
- `branch_sweep.py source/memory/lrar_cache`: 7 blobs, current is the best (every other blob is 0 exact).
- No natural loop respelling, polarity flip, `goto`, `do/while`, early-return or flag shape was re-tried
  (w2 ×9, n2 ×1 already spent them; the directive forbids re-running that sweep).

## 5. Reopen criterion (unchanged, restated)

A natural source construct that cools a call site one tier below a depth-0 statement **without emitting a branch**,
or evidence that `lrar_get_block`'s IL cost differed in January. Half-way control: n2's `lr/n1.c` (912/37, both sites
drop exactly one tier). Expected-byte control: `scratch/workers/player_profile_files_windows_etc/lr_p8.c` (exact but
inadmissible). Reopening also requires a whole-TU regate, because the getter's inline cost is shared with
`_lrar_flush`, `_lrar_block_address` and `_lrar_deallocate`, which are exact today.
