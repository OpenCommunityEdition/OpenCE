# Continuation 006 — initialization, interrupts, clear and mode selection

## Result and baseline

The full checkpoint005 source was freshly replayed before authoring: all14
stages passed, 289 code sections /31,643 padded bytes /29,642 NOP-excluded bytes.
The untouched baseline source, objects, report and replay log are retained in
`evidence/wave6_baseline/`. Current-source final results are under evidence/wave6;
old checkpoint reports elsewhere are history, not fresh verification.

Nine newly reconstructed functions account for3,288 padded bytes. All are
same-name passes through the unchanged strict comparator, with no new private
pair, relocation alias, SDK wrapper, approximate score or standalone-data credit.
The resulting code census is298 sections /34,931 padded /32,895 NOP-excluded bytes.

## Reviewed source and actual target interface

| Function | Code bytes | Interface/behavior evidence and limitations |
|---|---:|---|
| InitHardware |312|Original PDB member signature, record addresses, DPC/event initialization, interrupt setup, shutdown registration, gamma arrays and GPIO setup. No later AGP-configuration additions were copied.|
| IsFlipPending |16|Actual indexed queue member and PDB signature, not an unimplemented constant stub.|
| Isr |192|Original static stdcall callback, hardware status/enable reads, field GPIO, flip condition, callback queuing and return.|
| VBlank |320|Original queue state, gamma updates, read counter, hardware acknowledgement loop, event/callback dispatch and CRTC-index restoration.|
| SoftwareMethod |560|Original un-packed method IDs0x300–0x324, callbacks, actual16-byte fixup record and hardware register programming. Its jump table is part of its code contribution.|
| ServiceFifoInterrupt |400|Original FIFO exception masks, retry countdown, diagnostics, GET recovery and resume sequence.|
| D3DDevice_Clear |976|Original color/depth format cases, viewport clipping, sample scaling, ordered NV2A writes and format restore. Includes associated jump tables.|
| GetTableForCurrentAvInfo |121|Original191-entry table, region/pack selection and generic-pack fallback.|
| SetVideoMode |391|Original zero-versus-explicit refresh choice, flag/size/mode filtering, gamma reset and deferred mode state.|

Each new procedure is authenticated against a unique original-PDB record,
return/parameter types, calling convention and length, then compared again
against the current candidate. Clear and the table lookup are ordinary stdcall
functions; Isr is a static stdcall member. Other new CMiniport methods are
thiscall. Do not infer kernel-import return types solely from callers that
ignore them: primary kernel headers corroborate those declarations separately.

### Kernel records and owner headers

The original PDB establishes112-byte KINTERRUPT,28-byte KDPC,16-byte event,
16-byte dispatcher header,16-byte shutdown registration and16-byte TIME_FIELDS.
The required event/shutdown/time fields are decoded. Interrupt/DPC internals stay
opaque and are only passed to real kernel routines. Replacing former byte spans
with their true owner-record types preserves every affected CDevice/CMiniport
size and offset; all local consumers are rebuilt in the final replay.

KeInitializeEvent is reconstructed as the genuine historical kernel header
macro, supported by cross-build NT header evidence and the original inline
instruction sequence. The simple field assignments set Type, Size, SignalState
and the empty wait list; Absolute/Inserted are not invented stores. All current
macro arguments are stable addresses/constants. This is not a fake emission
anchor or a new force-inline annotation. The first ordinary helper form was
nonexact and is retained as an experiment. It is not counted.

EnableInterrupts now lives as an ordinary inline in its genuine hardware owner
header next to DisableInterrupts. The standalone original entry still matches,
as do all previous Dpc consumers. The original GPIO polarity is inverse bit5;
using its real unsigned port value closes Isr and VBlank. Intrinsics _inp/_outp
express actual I/O operations, not assembly written to steer other registers.

### Interrupt graph and retained nonexact work

SoftwareMethod uses actual method enums and true callback/fixup interfaces.
The isolated FIFO debug-register reads access the real fixed MMIO lvalues;
no volatile local or synthetic load was introduced. They reflect the original
hardware macro access, unlike a discarded helper-return temporary. This closes
the FIFO body while preserving its bounds/retry checks.

ServiceGrInterrupt is complete but NONEXACT: current target/candidate sizes are
368 bytes, with a this-pointer versus method-variable register-lifetime
permutation. Its real fatal diagnostic uses the compiler's __debugbreak intrinsic
(the original target has INT3); no new inline assembly or raw-byte emission was
introduced. A separate probe confirmed compiler support. This body gets zero
credit and was parked instead of pursuing arbitrary register constraints.

LoadEngines is complete with equal315-byte normalized instruction sections, but
the authentic month-table owner is not reconciled with the portable target's
private data anchor. It remains NONEXACT and uncredited. No new relocation alias
or address-shaped source variable was used. Its original month-index and timing
arithmetic are visible; later corrected calendar formulas are not substituted.

### Clear and presentation modes

Clear matched on its first complete compiled draft. The two floating-depth
conversions read the high32bits of an IEEE754 double using memcpy through byte
views, avoiding an unrelated DWORD lvalue. Packing shifts use unsigned values.
Valid use assumes finite depth in[0,1], valid allocated rectangles, supported
surface formats and device-range coordinates. Original early return when depth
flags become empty occurs before swizzled-format restoration; this quirk is
preserved and commented, not represented as an improved driver.

GetTableForCurrentAvInfo matched initially. SetVideoMode initially differed
at384 bytes. The original separates unspecified refresh0 from explicit50/60Hz;
spelling that genuine outer conditional produced the391-byte exact body. There
is no compiler assumption, new optimization flag or fabricated caller. Table
selection assumes supported AV information and a valid mode as checked by its
real caller graph; it is not a new defensive parser for arbitrary AV values.

The original191 display records and count are recovered as typed configuration
data (2296 bytes). Their contents are verified independently against compiled
owned ranges and do not inflate code progress. MPMode now has4/4 matching code
sections. It is NOT a whole-object admission: the target data contributions are
COMDATs, the current constant section combines them with an alignment gap, and
three unused SDK encoding tables are emitted. These metadata/ownership holds
are recorded rather than hidden by making the target agree with the candidate.

## Rules and provenance

Latest fetched rules blob:6bee6e0c6b67e3799527fbf96880e11bc59b6508, unchanged
from005. New definitions have separate-line parameters and explicit returns.
Declarations stay in genuine kernel/display/hardware/math owner headers. Driver
SDK types are appropriate; unrelated Halo tag/object macros are not introduced.
Mode/depth/software-method switches use the actual SDK or method enumerations.

No new assembly, force-inline/no-inline/optimization pragma, volatile local,
raw instruction output, target patch or comparator change. The two inherited
BSF/SFENCE helpers remain disclosed. Genuine MMIO/shared-state volatiles and
port intrinsics do not establish concurrent hardware correctness by themselves.
The source-policy scan is limited lexical evidence, NOT all-house-rule approval
for the inherited reconstruction or an exhaustive undefined-behavior proof.

Attributed RXDK code remains GPL-3.0-or-later and is not automatically compatible
with canonical CC0 publication. The reference-provenance JSON distinguishes
later SDK corroboration from original PDB/byte evidence; no later flip queues,
modern callback shims or packed-method formats were substituted for January.

## Verification and recovery

Final replay recompiles all22 current C++ units from the47-file source/header/
initializer tree, binds source and generated ABI-overlay hashes to every output,
runs the unchanged comparator,44 comparator unit tests and the inherited full
mutation suite, and preserves all289 starting fingerprints plus earlier
protected checkpoint populations. The new verifier authenticates new PDB
procedures, selected record layouts and the independent display-data ranges.
Read final evidence/wave6/clean_replay.json for actual completed stages/status.

No supplied Halo image, extracted original function or GPU program is executed.
No new bespoke runtime harness is claimed. Event/interrupt concurrency,
hardware progress, callbacks, malformed inputs, driver linking and graphics
behavior remain untested. Native-csplit download attempts failed; this is NOT a
passed native-split comparison. Whole-object data/BSS/COMDAT/extra-section and
actual canonical consumer review remain open. Canonical/GitHub/Bink/Claude
worktrees are untouched; isolated counts are not added to the canonical ledger.

The new005-to006 source patch and full/repair-relative alternatives are replayed
independently. Apply only the one matching the starting source tree; source
patches alone do not replace the SDK ABI overlay and build scripts. Failed
source/interface trials, probes and authoring scripts are retained under
experiments/wave6; scripts/reproduce.py is the portable replay entry point.

Next: finish the GR handler and table ownership only with new interface/source
evidence, or move into presentation/lazy-state/device-initialization families.
Preserve all298 body proofs; do not generate fake callers for missing helpers.

## Packaging repair

The first in-place full replay passed source compilation and comparison gates
but failed git apply because the newly generated full patch omitted new-file
mode headers. The patch format was corrected; no production source, target,
compiler setting or comparator was changed to pass the patch check. All three
patches then recovered the current47-file source set. That failed attempt is
retained in evidence/wave6/failed_replay_attempt1, separate from the final
fresh-extraction verification.
