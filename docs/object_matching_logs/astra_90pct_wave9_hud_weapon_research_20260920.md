> HUD research only: no new shape, production change, or exact credit.

# Packet 9: render_weapon_hud — no new supported closure mechanism

**Hold; zero source shapes, zero compiler invocations, zero credit.** The assigned residual is 2,645 meaningful January bytes. Fresh read-only evidence reproduces the existing boundary rather than reopening the exhausted source variants. No production source, header, configuration, target metadata or build object was changed. `IDA_AUTOMATION_UNAVAILABLE`.

## Frozen state and controls

Production is the packet-6 source baseline, with later evidence-only commits; receipt HEAD is `5fe5f0012206c416b133542063598a0bcebf6146` after root's packet-8 archive. `audit.py` snapshots and hashes this file and its current/base and January/split objects. Source SHA-256 is `b4644b8667344baa10a1f69c75155393183e58cdda44a9720a231ec37feb1188`; current source and base object still equal those snapshots.

| Measure | January | Current |
|---|---:|---:|
| `_render_weapon_hud` padded bytes | 2,656 | 2,624 |
| Relocations | 84 | 85 |
| Normalized SHA prefix | `57b1b8d369cd045d` | `343910c88f6d61a3` |
| Listed unit rows | 16 | 11 exact / 5 residual |

The only relocation-multiset difference is `__ftol2`: January has two calls, current has three. This is the already-researched decimal conversion, not a newly discovered helper. Full function bytes and relocation identities remain the standard; matching call counts would not close the function.

The frozen current inventory contains 18 emitted function owners, 42 named defined owners, 25 non-debug noncode sections, no COMMON, no `_point_from_line3d`, and no active parks for this unit. All 11 exact controls are recorded in `evidence.json`, including `_hud_render_weapon_interface`, both state accessors, the initialize/dispose functions, `_hud_fix_weapon_data`, `_play_weapon_hud_sounds`, `_scripted_hud_show_crosshair`, and `_strip_path_name`. There is no candidate, so these are frozen admission guards, not a claimed before/after successful probe.

## Prior-art boundary

Read the September 12 private-graph packet; September 14 HUD w1/w3 ledgers and original worker notes; the crosshair ownership/intake/reconciliation records; later corpus references including the September 16 zero-gain wave-14 summary; and both packet-6 ARGB independent reviews. The ARGB true-inline-header proposal lost screen-effect and emitted two surplus copies. The coherent declaration-header migration removed surplus copies but still lost screen-effect. Neither authorizes include/prototype/declaration-order tuning here.

The old `R1`–`R3`/`U1` already tested late aggregate initialization, direct array `SET_FLAG`, repeated PIN conversions, two number-drawing branches, element-state rereads, bitwise sentinel access, and real distance helpers. R3's 2,672-byte/84-relocation body stayed residual. This receipt independently loads the old R3 object and all three w3 distance variants (`R3d1`–`R3d3`): all four have identical full function hash `ec333ee6865bfc38…` and relocation identity. The old files remain at `C:\halo-worktrees\opus5-150k-house-clean-20260914\scratch\workers\hud_weapon\` and `...\w3_hud_weapon\`; full paths/hashes are saved in the JSON. No archived candidate was recompiled.

The historical raw-bit-sentinel policy objection is obsolete: the current file already uses the admitted local union transfer. The remaining prior-art boundary is the statics-loop allocation context (January keeps definition in EDI, statics block in ESI and element index in memory), frame-home permutation, and contextual x87 sum ordering. An exact CTF control demonstrated both square-sum orders from the same distance inline. A named horizontal subtotal or local reorder therefore remains unsupported steering.

## Fresh primary evidence

`collect.py` queried the actual HCEA PDB and opened the supplied Ghidra project through `getReadOnlyDomainObject`, exported one function, released the program and closed the project. `ghidra.log` records successful decompilation and the expected PE SHA-256. No project edit/save occurred. HCEA's source checksum/lines are original debug evidence; `research/halocea/src/render_weapon_hud.c` is a later reconstruction and was not treated as recovered source.

* HCEA `render_weapon_hud` authenticates the current seven-parameter signature: long HUD index, short player index, two typed const structure pointers and three const short-array pointers. It preserves six aggregate locals: `state_flags`, `overlay_flags`, `numbers` as `short[8]`, `numbers_real` as `float[8]`, and `position`/`target_position` as `real_point3d`. It provides no scalar element-index, map-flags, helper-temporary or declaration-order evidence. The 109 line records identify `interface/hud_weapon.c`, source MD5 `61CBF8FB3663EDC518BE4805AC6CF116`. PPC register labels printed by DIA are not x86 allocation evidence.
* Fresh raw PE RTC descriptor `0x63ba40`, attached to `0x6395f0`, independently gives the same six extents: 16/16/16/32/12/12 bytes. The first four homes are -32/-56/-80/-120; point homes are -144/-164. These are instrumented later-build homes, not January stack slots. The later binary is the authenticated 2020/v140 cross-build; same-source-revision remains unverified.
* Raw assembly and Ghidra confirm tag/state access before the four zeroed arrays, word-sized flag updates, repeated age/PIN calls, signed-short element-loop increments, recursion before drawing, four typed tag-block loops, and two number-drawing source paths. These are the previously tested mechanisms. They do not predict a new legal source shape.
* The later statics loop has extra data repairs at `0x63b55d`–`0x63b588`: at element offsets +0x60 and +0x6c it rewrites `0x00003f80` to `0x3f800000`. January goes directly from its element getter into flag/map checks. This is a concrete later-build difference, not missing January behavior to transplant.
* January's numeric tail is explicit: `fabs` at +0x8a1, `__CIfmod` at +0x8a6, and inline `fistp` at +0x8b1. The current C already has absolute value before remainder. Ghidra's inferred nested helper argument expressions are unreliable here because pushed outer-call arguments stay live across the conversions. Fresh raw helper receipts resolve the three thunks at `0x402e64`, `0x409aa7`, and `0x406b36`; the last body at `0x455400` explicitly performs `fld`/`fistp` and returns the integer. This supports the old R3 decimal conversion distinction, not a new fabs/remainder swap.

## Reopen criteria

Reopen only for an independently authenticated January-compatible scalar lifetime/type, genuine call boundary, or ownership fact that explains the statics-loop register priority and predicts full closure while retaining the 11 exact controls and all selected owners/data/COMMON/point policy. The recovered aggregate extents and later-only statics repairs do not supply it. The supplied local-artifact search is closed; there is no pending request for more files. No bitmap/header visibility replay, declaration-order repair, horizontal-distance temporary, general scheduling sweep, or fuzzy source polish follows from this packet.

## Reproducible files

All new files are under `scratch/astra-wave9/hud-weapon/`: `packet.md`, `audit.py`, `evidence.json`, `summary.json`, `baseline.c`, `baseline.obj`, `target.obj`, `baseline.asm`, `target.asm`; `collect.py`, `AstraHudWeaponLaunch.java`, `collection.json`, `ghidra.log`, `ghidra_006395f0.c/.asm`, `hud_weapon_source_xrefs.tsv`; HCEA `hcea_render_weapon_hud_sym.txt` and `_lines.txt`; `primary-raw.asm` and five helper raw receipts. These are evidence only. File ownership is released.
