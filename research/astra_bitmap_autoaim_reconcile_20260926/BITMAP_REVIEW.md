# Independent source review: `_rasterizer_bitmap_new`

Reviewed donor commit `8202ac1355b7732a948c9887223b4066ff47f148` from
`C:/halo-worktrees/claude-remaining-frontier-20260926`, against the requested
canonical starting point `d946958e`. This review made no source, config, build,
or evidence edits in either worktree and performed no compiles. Canonical's
live build was changing during the coordinating agent's integration, so no
live `build/base` measurement is represented as the frozen starting baseline.

## Verdict

**Admissible source reconstruction, conditional on the coordinating agent's
fresh strict function and regression gates.** This is a meaningful, natural
single-exit/failure-reporting repair, not an inert decoration, manufactured
temporary, ABI change, or held compiler-control construct. It introduces no
new casts, helpers, declarations, macros, or diagnostics. Whole-object
admission and any `Matching` status remain explicitly deferred.

The exact position of the common `error()` statement is an inference from
January's code and the measured coupled repair. It is not authenticated
original C. Later `/Od` and retail observations support the reconstruction
but do not establish January's original source. No special exception is
being inferred from those witnesses.

## Actual patch and behavior

Ignoring indentation, the source patch only changes the constructor tail:

```c
if (global_d3d_device)
{
    /* Existing switch and calls unchanged. */
    if (!bitmap->hardware_format)
        success = FALSE;
    if (!success)
        bitmap->hardware_format = NULL;
}
else
{
    bitmap->hardware_format = NULL;
}
if (!success)
    error(_error_silent, "### ERROR failed to create bitmap hardware format");
return success;
```

The other change is the narrow removal of this function's park. Do that only
after fresh canonical verification. The retained per-case success assignments,
assertions, strings, casts, and calls are unchanged. The upstream invented
`D3DCALL` macro and redundant no-device `success = TRUE` were not imported.

Behavior is preserved for every defined execution of the original body:

- No device: `success` still has its initial TRUE value; clear hardware format,
  make no creation/error call, and return TRUE.
- Device, successful result and non-null hardware format: preserve hardware
  format, make no final error call, and return TRUE.
- Device, failed result or null hardware format: set success FALSE as before,
  clear hardware format before the same error call, and return FALSE.
- The default assertion path is unchanged, including its behavior if the
  assertion machinery were to return. There is no new read of `result` there.

Separating device cleanup from the post-join failure report is coherent source
organization: the report consumes the overall success status. The extra local
test on the no-device path is side-effect free and sees TRUE. No new memory
access, API ordering, or error condition is introduced.

## Independent raw-image corroboration

Read the supplied PE as data with Python `pefile` and Capstone; did not execute
the image or rely on the worker's saved disassembly for this readout.

- File: `research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe`
  under the outer workspace.
- SHA-256:
  `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
- Image base `0x400000`; function VA `0x7ea7d0`; raw offset `0x3e9bd0`.
- At `+0x17`, initialize byte `[ebp-1]` to 1.
- At `+0x2e4`, compare hardware format `[eax+0x28]` with 0; on null,
  `+0x2ea` stores 0 into `[ebp-1]`.
- At `+0x2ee..+0x2f9`, test that success byte and clear hardware format when
  false.
- No-device arm `+0x30e..+0x311` only clears hardware format; it does not
  reassign success.
- At `+0x318`, load AL from `[ebp-1]`; the one function return is `+0x33b`.

Important differences visible in the same raw readout: this later DX9 body has
an additional format-validity guard, no January final `error()` call, per-case
failure-only assignments (rather than January candidate's explicit success
arms), and a different diagnostic call signature. It cannot be copied as a
January implementation or used to attest the final error placement.

## Independent January/saved-object readout

Read canonical's January split with the existing COFF parser and disassembled
the constructor section. Important instructions/relocations are:

```text
+0x063  je  0x177                 ; no device
+0x14b  mov eax,[esi+0x28]
+0x14e  test eax,eax
+0x151  jne 0x157
+0x153  xor bl,bl
+0x155  jmp 0x15b
+0x157  test bl,bl
+0x159  jne 0x171                 ; first return copy
+0x15b  push <failed-to-create diagnostic string>
+0x160  push 2
+0x162  mov dword ptr [esi+0x28],0
+0x169  call _error
+0x16e  add esp,8
+0x171  pop esi; mov al,bl; pop ebx; pop ebp; ret
+0x177  mov dword ptr [esi+0x28],0
+0x17e  pop esi; mov al,bl; pop ebx; pop ebp; ret
```

Padded extent is 400 bytes; meaningful extent ends at `+0x183` inclusive
(388 bytes). There are 26 relocations.

The existing saved RF-U `scratch/rf/workers/RF-U/b/CAND_hb.obj` was re-read,
not rebuilt. Existing canonical `coff_compare.section_infos_equal` returns
TRUE against January: 400 bytes, 26 relocations, normalized SHA-256
`041cef84e2a13eeae0fccfaef6e8e897a5c5926bf850bc6c10bc245f3df8a82d`.

File identities for that readout:

- January split SHA-256:
  `4160ef68b6729bd853d3e0e78192d12e9fe6513328c6aee351236bc7b2d0a002`.
- Saved candidate object SHA-256:
  `d3fae3b16a533b2ce21041edf2baf6e6dc2af4553d9c19f0d6d09adf7ab87d80`.
- Saved candidate source SHA-256:
  `e5ea0d4e4488f27248000cc357a5fb71b04711e41f5143c1930917ed8dca2aca`.

Raw parser dictionaries are not literally equal: January references external
pooled assertion strings that the candidate defines locally. The already
established comparator accepts their identical symbol names and addends.
No comparator, normalization, alias, or exclusion change was made here.

## Evidence and holds reviewed

Read the consolidated 2026-09-26 house rules, canonical campaign house rules,
and matching methodology; then the existing small-object re-audit, prior
fifty-objects ledger, RF-R report, RF-U report and B5/B6/B7/R1/R2/T1 cards.

The old park/re-audit allowed reopening on a measured natural control-flow
construct, in addition to original source/debug evidence. The RF-R mechanism
readout and RF-U coupled tail supply new evidence; this is not another generic
blind polarity sweep. No explicit construct-specific owner prohibition was
found for the ordinary single-exit/post-join reporting form. Unrelated holds
in the owner packet remain untouched.

Important controls, as reported by RF-U (not rerun here):

- B5: tail-only, with redundant else assignment removed, exact.
- B6: single exit but error restored to device cleanup block, residual
  `9f2d9d45...`; the predicted equality to the old HEAD was false.
- B7: separate error test while retaining the early return, equals old HEAD.
- R1/R2 retail lab: B5 and B6 match the September/October witnesses; HEAD and
  B7 do not. This corroborates the single-exit shape, not error placement.
- T1: one return block before backend splitting; the error-adjacent
  conditional-edge copy hosts the shared tail. The detailed route prediction
  was partly corrected. Compiler tracing explains reachability, not source
  provenance.

The complete change qualifies as a coupled natural repair under the policy
allowing independently supported A+B changes even when neither single is
exact. Merely saying `/Od attested` or `the compiler can produce it` would be
insufficient; the meaningful source organization, unchanged semantics,
January instruction/relocation constraints, and explicit negative controls
are material parts of this verdict.

## Minimal evidence to preserve with reconciliation

This review plus the following donor files are sufficient for a compact
source-evidence packet (in addition to the coordinator's fresh gate receipts):

- `workers/RF-U/REPORT.md` and `evidence/bitmap_new_candidate_checks.txt`.
- `workers/RF-U/cards/B5.txt`, `B6.txt`, `B7.txt`, `R1.txt`, `R2.txt`, `T1.txt`.
- `workers/RF-R/REPORT.md` for the prior negative and decoded mechanism.

These paths are under `research/remaining_frontier_20260926/` in the donor.
The source is already frozen by the reviewed commit, so its patch copies need
not be duplicated. Preserve the old park/rejection/reopen history rather than
presenting the closure as evidence that all earlier investigations were wrong.

No whole-object/provider/surplus-symbol approval, fresh compiler receipt,
retail rerun, or gameplay-runtime claim is supplied by this source review.
