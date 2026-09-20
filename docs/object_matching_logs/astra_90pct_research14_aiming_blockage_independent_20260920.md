> Research-only packet14: zero production change and zero exact credit. Validated production remains wave13.

# Independent aiming-blockage primary review

**APPROVE BRANCH PROVENANCE ONLY; HOLD FULL CLOSURE.** The supplied unoptimized code independently supports the zero assignments in historical `avb_body3b.c`. This resolves the old source-evidence objection to those assignments; it does not make the candidate exact or justify a new source/header test. Zero source changes, C compiler calls and credit.

I independently decoded PE bytes0x4710a0..0x47124a, resolved the seven real helper thunks, read the RTC descriptor and HCEA procedure type, and compared the saved old object against current January COFF. PE SHA256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. This review uses raw bytes rather than Ghidra's inferred C return type.

| Raw address | Actual returned word `[EBP-0x18]` |
| --- | --- |
| `0x4710c2` | Initialized to0 before copying/normalizing the planar source vector. |
| `0x4711a3` | Set to2 for the narrow vertical range. |
| `0x4711ca` | Set to1 for the wider vertical range. |
| `0x4711d2` | Re-set to0 when neither vertical range qualifies. |
| `0x471210` | Set to1 for the wider horizontal tolerance. |
| `0x471218` | Set to0 outside the horizontal tolerance. |
| `0x47121c` | Loaded into **AX**; EAX is saved/restored around the RTC stack check. |

An independently constructed intraprocedural CFG proves that the initial store dominates every later result store and the return load. No address of this two-byte result cell is passed to a helper. The vertical-failure0 at0x4711d2 is redundant with its dominating initializer, because the alternative2/1 stores branch past that arm. It is nevertheless present in unoptimized first-party machine code. The horizontal-failure0 at0x471218 is **not** redundant: that path is entered only after a positive result and clears it. Both are the real assignments represented in body3b. The raw float constants independently match0,0.8660254,-0.5,0.9,-0.8,1.2,0.36 and1.21; the arm correspondence is not inferred from store values alone.

Ghidra's `void FUN_004710a0` display and `local_1c` name are inaccurate source-level descriptions. In addition to the AX flow, raw HCEA stream2584 record0x2218 is a GPROC32 referring to LF_PROCEDURE0x6e22f. Its bytes `0e00081011000000000004002ee20600` specify **signed-short return0x11 and four parameters**. DIA corroborates point/vector pointer types and source file `ai/actor_perception.c`, lines4765–4838. These records authenticate the interface and source owner, not exact January declaration text or brace placement.

Raw RTC0x47124c supplies three independent aggregate records: `source_planar_direction` at-16/8bytes, `friend_vector` at-40/12bytes, and `friend_perpendicular_vector` at-68/12bytes. Their actual use, the four input/output arguments, and the distinctive two-stage threshold graph support function correspondence. The later PE has no recovered source labels for the private helper bodies, so their names below remain operation-based identifications.

The real call sequence is normalize2d, vector-from-points, planar magnitude, planar dot product, a three-component `p+t*v` operation, optional negate, and planar magnitude-squared. Each call/thunk/body address is recorded in `review.json`; helper bodies were independently decoded. This is stronger than trusting a decompiler's apparent arithmetic expansion. **A newly established ordering detail is that the later code obtains friend planar magnitude before dot product**, whereas body3b assigns the dot product before evaluating magnitude in its comparison. The affine operation and negation also appear as genuine later helper calls rather than body3b's manual component statements.

Those are new primary operation/context facts, but they do **not** provide a credible full-match closure premise yet. The first normalize helper is already explicit in body3b. Later SSE helper calls do not specify how VC7 must retain an inlined x87 reciprocal, and a real affine operation does not by itself authenticate a particular January helper name, point/vector interface cast, selected provider or inlining schedule. Do not introduce a helper owner or reshape declarations merely to obtain the desired allocation. No test is recommended from call ordering alone while the independent reciprocal boundary remains open.

The archived `avb3b.obj` freshly measures400bytes/10relocations, SHA `cfa09d0257e6d5096b4ae00caf366dc50a44ecc2dc289b278c9d752c67268cad`; January is400/10, SHA `a9a57924652aa5132cd73ed04e0134d620360c9b1d98a2a15db8c042541f356e`. It is **not strict**. The saved alignment contains the reciprocal/explicit-pop difference near+0x4c/+0x54, a magnitude/projection stack sequence near+0x9b, and return-register/pop/epilogue differences around+0x169..0x181. Thus “only the zero store was missing” would be false. Existing symbol-versus-defined-noncode literal annotations in the raw alignment are disclosed separately from these instruction changes.

I also independently rechecked all ten function descriptors in root's normalize-context packet: **nine exact controls and this one residual**. The controls include the selected normalize/magnitude/scale/sqrt helpers and exact observer, firing-position, valid-aim and valid-look callers. They agree with their current January targets. This contradicts treating the old “all20 normalize2d sites require the post-j pop” statement as a universal implementation law; the bounded sample does not claim to re-audit all20 historical sites.

Same source revision across January, HCEA and the supplied later PE remains unverified. The authentic later graph satisfies the narrow repeated-assignment provenance criterion without proving the complete January source. No canonical source, header, configuration, compiler option, owner or target record was changed.

Receipts: `review.py`, `review.json`, `main.raw.asm`, seven small helper raw listings and `january-vs-old-body3b.alndiff`. Inputs include root's fresh `scratch/astra-wave14/aiming-blockage/` Ghidra/DIA exports, the September14 perception ledger, and `C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/w3_actor_perception/avb_body3b.c` / `avb3b.obj`. **RELEASED.**
