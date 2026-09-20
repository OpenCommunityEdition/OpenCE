> Research-only packet14: zero production change and zero exact credit. Validated production remains wave13.

# Packet 14: bounded ABI opportunity scout

**Closed with no supported new lead. Zero source shapes, C compiler calls, production changes, or exact credit. Evidence released.**

Baseline is `bdbedaf87a6b0a759b723377e8e7fdadbb7c1337`, production wave 13. Subtracting the current semantic accepted ledger from the current atlas gives 284 residual rows / 326,188 meaningful bytes. This study examined the following eight documented residuals, totaling 9,572 meaningful bytes; it did not enumerate every procedure in the PDB.

| Function | Meaningful bytes | Current similarity | Prior boundary and disposition |
| --- | ---: | ---: | --- |
| `_hs_evaluate_begin_random` | 519 | 99.89011% | Saved DIA output omitted a decoded return type. One targeted raw procedure query below confirms the existing declaration; no new source prediction. |
| `_hs_global_reconcile_read` | 1,405 | 92.30703% | Prior W1/W3c reports already give `static void(short)` and reject the measured short/mask variants. No new omitted-return premise; no raw re-query. |
| `_hud_show_action_response` | 1,744 | 94.235954% | Private EAX parameter convention is already recovered. The remaining register mirror is not an unexplained ABI. The short icon-helper trial regressed an exact owner. |
| `_stack_memory_pool_allocate` | 637 | 97.36842% | September 2 PDB signature audit explicitly records pointer return and unsigned size. Coherent signature corrections were byte-inert; local value/lifetime provenance remains missing. |
| `_editor_camera_flying_update` | 1,407 | 99.25616% | Existing boundary is global-load/address-taking schedule and helper ownership, with no newly identified consumed-return discrepancy. |
| `_player_control_modify_desired_angles` | 1,577 | 99.59677% | Existing boundary is independent x87/constant-load scheduling; no missing return/local-width evidence identified. |
| `_bsp3d_test_sphere_recursive` | 792 | 99.985664% | Saved packet 5 DIA output explicitly prints `static void(context *, long)`. It does not have the omitted-return premise. |
| `_bsp3d_test_pill_recursive` | 1,491 | 97.21691% | Saved packet 5 DIA output explicitly prints `static unsigned char(context *, long)`. It does not have the omitted-return premise. |

The two rows below 95% were retained in this bounded roster because their documented holds specifically concern private ABI or parameter width. Similarity is the existing wave 13 atlas value, not a new objdiff run.

## The single qualified raw query

`scratch/astra-wave5/shared-context/hcea_hs_evaluate_begin_random_sym.txt` supplies only the bare function name in `UndecoratedName` and three named parameter types. It does not print a decoded return type. Its `NoReturn: false` property does not answer that question.

The supplied HCEA PDB, SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`, contains the matching `S_GPROC32` record in `hs_runtime.obj`, module stream 2691, record offset `0x2ef8`. Its `LF_PROCEDURE` type `0x6ef86` is:

```text
0e000810030000000000030085ef0600
return: 0x3 (void)
argument list: 0x6ef85
arguments: 0x11 (short), 0x12 (long), 0x20 (unsigned char)
```

This explicitly confirms `void (short, long, unsigned char)`. Current `source/hs/hs_runtime.c:2115` declares `void hs_evaluate_begin_random(short, long, boolean)`; `cseries.h` defines `boolean` through `byte` as unsigned char. No return or parameter correction is supported.

A fresh read of only this January/current COFF function confirms 528 padded bytes, 30 identical ordered relocation identities, and 191 instructions in each. The four differences remain exactly:

| Offset | January | Current |
| --- | --- | --- |
| `+0x123` | `movsx eax, ax` | `movsx edx, ax` |
| `+0x126` | `movsx ecx, cx` | `movsx eax, cx` |
| `+0x129` | `mov [ebp+0x10], eax` | `mov [ebp+0x10], edx` |
| `+0x12c` | `mov [ebp+8], ecx` | `mov [ebp+8], eax` |

There is no newly exposed consumed return or width discrepancy here. The raw HCEA record closes an evidentiary omission, while leaving the previously documented register-choice boundary intact. No local-order or expression retry follows from it.

## Prior evidence and limits

The roster was screened against the current atlas/parks and the relevant histories, chiefly:

- `hs_runtime_obj_opus5_150k_w1_20260914.md` and `hs_runtime_obj_opus5_250k_w3c_20260915.md`: signed-short reconcile variants and begin-random register-tie probes already exhausted.
- `hud_obj_opus5_150k_w2_20260914.md` and `hud_obj_opus5_150k_w3_20260914.md`: recovered private ABI, remaining register mirror, rejected exact-owner regression from the icon helper return change.
- `stack_memory_pool_obj_codex_hcea_signature_audit_20260902.md`: explicit pointer return, genuine core parameter types/order, byte-inert signature corrections, and the still-missing allocator value/lifetime provenance.
- `editor_flying_camera_obj_opus5_150k_w1_20260914.md`, `player_control_obj_opus5_150k_w2_20260914.md`, and their current atlas dispositions: no new consumed-output mismatch to predict an ABI edit.
- `scratch/astra-wave5/shared-context/packet.md` and its three HS/BSP DIA dumps: the HS return omission is real; the two BSP returns are already explicit. The failed sound owner-width test remains excluded.

No weapon-place, player-stub, main-crash, fastftol, name-alias, seven-unwritten, whole-archive, or source-spelling sweep was repeated. No new owner/header declaration is proposed. HCEA is later corroboration; a shared January source revision and original lexical source remain unverified. This negative roster does not claim that every remaining function has the correct ABI.

`audit.py` reproduces the current roster, the one raw procedure/type extraction, and the selected January/current disassembly comparison. `audit.json` retains all identities, hashes, raw records and differences; `split.asm` and `base.asm` preserve the complete selected function. Only these evidence files were written. No owner-preservation claim from a new gate is made because no candidate was compiled.
