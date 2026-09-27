# Hardware-bitmaps source reconciliation — 2026-09-27

Canonical predecessor: `7b51e0bac5788b5294c510daf1c50fac64638997`.
Reviewed donor packets: `334f14dce931d5f44c9049544115c184e89fcf1c`
(table/header ownership), `a837d8c7549fe9a44d5872dde2c5218a7c8bfcfd`
(C1 structured upload loops), and prepared admission `e0088c4a`.

The owner approved C1 plus admission after complete gates. This reconciliation
includes the missing prerequisites; applying only the status flip would have
left the invented fused table and consumer-local declarations in canonical.
No function or data bytes are newly matched by this source packet.

## Coherent source changes

- Replace the fused format/face aggregate with the separate format and face
  arrays. January's own stringifications use the two names directly. Raw bytes
  at 0x290958..0x2909ab contain 72 format bytes and 12 face bytes; the latter
  decode as shorts `[0, 2, 1, 3, 4, 5]`. Add only the static face-table symbol
  at file offset 2689440, preserving the existing section and row order.
- Move `texture_cache_bitmap_delete` into its genuine associated owner header,
  matching the definition in xbox_texture_cache.c. Use the existing
  rasterizer_xbox.h declarations for rasterizer_error/global_d3d_device.
- Replace the three goto-transcribed uploads with C1's structured loops and
  the approved accumulating-success Lock/Unlock idiom. Remove redundant
  numeric casts and unused labels. No new assembly, attributes, compiler
  options, decorative parentheses or synthetic declarations.

Source blob: `48ec57bbe31c3ea518592517e5aaa7758761ca8e` (identical to C1).
Texture-cache header blob: `7014f9ae871dd2fd96a6bc13baa6583a42adc84d`.

## Evidence limitations retained, not upgraded into facts

The historical D3D macro name and complete definition are unknown. C1 expands
an established idiom around real XDK calls; it does not copy helper bodies or
claim to recover a macro named D3DCALL. First-party /Od code and January-era
stringifications support wrapped Unlock and structured loops. The three dead
Unlock diagnostic strings are inferred from later D3D9 text using Xbox D3D8
names, not recovered January text; none is emitted.

January directly names the face table, and storage evidence supports static
linkage, but file-scope versus function-local static remains inferred. The
existing TU-local bitmap constants' original declaration location is likewise
not established. The unusual inherited Create out-parameter casts are directly
present in January's strings; this grants no new general cast exception.

## Fresh source-packet gates

- Full `ninja all_source progress build/report.json` succeeds, including fresh
  csplit regeneration. Scorer unchanged at 3.3.1.
- All 19 hardware functions remain strict exact. All 32 January-owned sections
  match payload, extent, flags/alignment and complete resolved relocations;
  all 33 January-owned symbols match storage and offsets. Data: 1,584 bytes,
  already credited before this packet. PDB-public cross-check: zero conflicts.
- Stable whole-board sweep against post-P1: 8,252 owners / 7,639 exact;
  zero gains and zero losses. P1's separate approved debit is not concealed.
- Captured all 622 current base objects before and after. All 15 existing
  texture-cache consumers retain every nondebug section and symbol; hardware
  becomes the sixteenth consumer. No unrelated code/data/COMMON change.
- The combined table correction changes one raw relocated dword in the cube
  uploader: format-table +72 becomes face-table +0. It resolves to precisely
  the same address; full relocation-aware identity remains exact. C1 alone
  was also separately measured as raw-byte/raw-relocation inert.
- Five surplus data definitions (two literals, three SDK tables) match
  January's selected copies and their current providers. Ten both-order
  provider probes show no duplicate-definition errors; expected unresolved
  externals remain. These probes are not a complete program link. Surplus
  copies earn no extra bytes. Existing default-library directives unchanged.
- COFF auxiliary records are not claimed literally identical: inherited SDK
  wrapper selections are 1 in the synthetic January split versus 2 in compiler
  output; the six Halo functions use 1 on both sides. The three surplus SDK
  tables likewise compare split selection 1 with current selection 2, while
  their current providers also use 2. Split checksums/section-number fields
  differ from compiler metadata. No selection/association changes from this
  source packet; only the cube-section checksum changes with its explained
  relocation addend. These are explicitly retained linker-model limitations,
  not patched metadata or a claim of identical raw COFF files.
- Parks: 72 active / 0 stale / 0 invalid. Admission queue: 12 candidates /
  0 contradicted / 1 rejected / 0 audit revocations before the status flip.
- Fake-scan output identical to baseline (26 inherited leads).
- Pytest: 1,161 passed, 5 skipped, 26 subtests. No new warnings; existing
  object_lights C4133 and rasterizer_xbox C4090 remain. Diff whitespace clean.

Complete local receipts are in `scratch/astra_one_more_20260927/hardware/`
(REPORT.md, post_verification.json, current_object_audit.txt, provider logs,
consumer snapshots) and `hardware_policy/` (REPORT.md, direct_evidence.json).
Large snapshots and private binary inputs stay local; no SDK/PDB/object files
are added to the repository.

## Accounting boundary

The source prerequisites and the Matching-status change are separate commits.
Before admission, Halo remains 388/468 objects following P1's explicit
revocation. Successful hardware admission returns it to 389/468, not a net
object gain over the pre-P1 baseline. Code remains 1,593,054/1,770,166 meaningful
bytes, 7,467/7,574 functions; data remains 2,588,903. Nothing is pushed here.

## Bounded next-source triage

A newly located Tool witness for `_bitmap_group_add_bitmap` at 0xb76280
supports already-tested descriptor initialization and nested success arms.
It does not supply an unspent January-consistent matching lever: its later
tag_data_get_address calls conflict with January's direct-field assert text.
The `_connected_geometry_find_or_add_vertex` Tool body repeats the known
call-first negative. No duplicate compile sweeps were run. Full witnesses and
reopen criteria remain in `scratch/astra_one_more_20260927/next_target/`.
This is a bounded negative result, not a claim that further reconstruction is
impossible. No unrelated hold was lifted.

## Admission checkpoint

Source prerequisites committed as `27c97361`. The subsequent single status
change to Matching was independently gated again: full build/report succeeds;
strict snapshot unchanged (0 gains / 0 losses); parks 72/0/0; admission audit
11 candidates / 0 contradicted / 1 rejected / 0 audit revocations; complete
fake-scan JSON unchanged; pytest 1,161 passed / 5 skipped / 26 subtests.
Fresh report confirms 389/468 Halo objects, with code and data exactly as in
the accounting section above. The only inherited exact loss in the complete
sequence remains the explicitly approved P1 draw debit.

Final local receipts: `after_admission.json`, `hardware_admission_build.log`,
`admission_final.json`, `parks_admission.json`, `fake_admission.json`, and
`pytest_admission.log` under the same scratch root. Only the reviewed source,
configuration and compact provenance documents are committed; inherited
README and untracked research changes remain untouched.
