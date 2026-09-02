# `key_agreement.obj` small-families worker ledger (2026-09-01)

## Scope and provenance

- Unit: `source/bungie_net/common/key_agreement` (`key_agreement.c`), target
  `build/split/source/bungie_net/common/key_agreement.obj` (January 2002 split).
- Compiler/flags: XDK 3911 `CL.Exe` 13.00.9254.1 with the `build.ninja` flags, via
  `tools/campaign/gate.py` only (no ninja, no header/config edits, no commits).
- Task: the single UNWRITTEN function `_is_message_encryption_key_message`
  (112 padded bytes). `_complete_key_exchange`, `_initiate_key_exchange` and
  `_key_agreement_get_packet_type` are PARKED and were not edited.
- Evidence: January target bytes/relocations (`gate.py --disas`, `scratch/xrefs.py`),
  the January assert string `msgptr && packet_type` at line 0xC4 with the
  `KEY_AGREEMENT_FILE` path string, `message_header.h` accessor macros, the
  sibling exact functions in this TU, and same-library January precedents
  (`transport_address_equivalent`, `thread_has_exited`). No HCEA donor and no
  HCEX PDB record exist for this function (bungie_net is absent from that cache).
  Prior ledger `key_agreement_obj_jonas_message_predicate_boundary_20260830.md`
  was read first; its private-name dispositions are unchanged.

## Validated baseline (start of session)

`board.py`: 448/1184 B, 6/10 fn. `gate.py`: `== exact 6 residual 3 unwritten 1`.
Residuals were exactly the three PARKED functions. Target normalized SHA-256 of
`_is_message_encryption_key_message`:
`18904e84b6ada1ed80b32160f9aece6ac3e2907f6fb403dfc53f9f214fd03db8`.

## Target decode

```
assert(msgptr && packet_type)                          ; line 0xC4
cl  = ((byte *)msgptr)[message_size - 1]               ; movzx ecx,word [ebp+0xc]; mov cl,[ecx+esi-1]
eax = (byte)*msgptr                                    ; xor eax,eax; mov al,[esi]   (BEFORE the store)
*packet_type = cl                                      ; mov [edi],cl
eax &= 3; if (!(al & 2)) return 0                      ; GET_MESSAGE_FLAGS + TEST_FLAG(...,1)
dl = (byte)*msgptr; dl >>= 2; dl &= 3; if (dl != 3) return 0   ; reload after the store; GET_MESSAGE_TYPE == _message_type_packet
if (cl == 0 || cl == 1) return 1; return 0             ; initiate / finalize packet types
returns: mov eax,1 / xor eax,eax                       ; 32-bit return value
```

## Accepted source shape (retained in the .c)

```c
boolean is_message_encryption_key_message(
	word const *msgptr,
	word message_size,
	byte *packet_type)
{
	word message_flags;
	byte message_type;

	match_assert(KEY_AGREEMENT_FILE, 0xC4, msgptr && packet_type);

	message_flags = GET_MESSAGE_FLAGS(*msgptr);
	*packet_type = ((byte const *)msgptr)[message_size - 1];
	message_type = GET_MESSAGE_TYPE(*msgptr);
	if (TEST_FLAG(message_flags, 1) &&
		message_type == _message_type_packet &&
		(*packet_type == _key_agreement_packet_type_initiate ||
		*packet_type == _key_agreement_packet_type_finalize))
	{
		return TRUE;
	}

	return FALSE;
}
```

Why the evidence supports it:

- The header byte is loaded before `mov [edi],cl` and reloaded after it. `byte *`
  aliases everything, so the flags read must precede the `*packet_type` store in
  source and the type read must follow it. A `word message_flags` local carries the
  first read across the store and reproduces `xor eax,eax; mov al,[esi]; and eax,3;
  test al,2` (the unfolded `(x & 3) & 2`). `long` also closes; `byte` does not.
- `byte message_type` reproduces January's unfolded `shr dl,2; and dl,3; cmp dl,3`.
  With `enum message_type` (as the parked siblings use) VC7 folds the test to
  `and dl,0xC; cmp dl,0xC`. The `message_header.h` API itself types the header's
  `type` and `flags` as `byte` (`build_message_header(word *msg, word length,
  byte type, byte flags)`), so the narrow local is the natural type, not steering.
- Two `return` statements reproduce the two duplicated epilogues.
- No volatile/register/pragma/barrier/asm/placeholder/offset was used;
  `tools/fake_match_scan.py` reports 0 leads.

## Experiment matrix (`_is_message_encryption_key_message`, target 112 B / 4 relocs)

| id | shape (one factor each) | size ours/target | relocs | first divergence | siblings | decision |
| --- | --- | --- | --- | --- | --- | --- |
| S1 | natural two-return, no locals, real header (`boolean`) | 96/112 | 4/4 | +0x33: single header load after the store; `test al,2` folded; `and al,0xC/cmp al,0xC`; `mov al,1` | none | reject |
| S2 | S1 + `word flags` read before the store | 112/112 | 4/4 | +0x4a: `and dl,0xC` vs `shr dl,2`; return width | none | keep as step |
| S3 | S2 + `byte message_type` local | 112/112 | 4/4 | +0x5f only: `mov al,1`/`xor al,al` vs `mov eax,1`/`xor eax,eax` | none | RETAINED (header-compatible) |
| S4 | S3 with `long` return type (variant-only shadow header) | 112/112 | 4/4 | none: EXACT; whole unit 7/10, zero sibling change | none | needs header edit |
| S5 | S4 with `byte flags` | 112/112 | 4/4 | flags block changes (sha residual) | none | reject |
| S6 | S4 with `long flags` | 112/112 | 4/4 | none: EXACT | none | equivalent to S4; `word` kept (sibling `word message_size` convention) |
| S7 | S4 with `enum message_type message_type` | 112/112 | 4/4 | `and dl,0xC/cmp dl,0xC` fold returns | none | reject; `byte` required |

Whole-unit gate with the real header after retaining S3:
`== exact 6 residual 4 unwritten 0` (baseline 6 preserved; the predicate is now a
size-exact/reloc-exact residual whose sole difference is the return width).

## Residual classification

`_is_message_encryption_key_message`: class A (recoverable source shape),
return-type width. Size 112/112, relocs 4/4 identical, every instruction identical
except the return materialisation. Closed exactly by declaring the function `long`
(S4/S6). The prototype lives in `key_agreement.h`, which this worker may not edit,
so the function is retained in its `boolean` form and a header request is filed.

Same-library precedent for the type: `transport_address_equivalent`
(`bungie_net/network/transport.h`, `long`, EXACT) returns `mov eax,1`/`xor eax,eax`;
`thread_has_exited` (`boolean`, EXACT) returns `mov al,1`. The January bytes place
this predicate in the `long` family. No object in `build/split` references the
symbol, so the `boolean` in the header was never externally constrained.

## Header request (orchestrator)

`source/bungie_net/common/key_agreement.h`: change
`boolean is_message_encryption_key_message(` to
`long is_message_encryption_key_message(`, and make the same one-word change on the
definition in `key_agreement.c`. Measured result (S4): predicate EXACT, unit
`exact 7 residual 3`, no change to any other function. `key_agreement.h` is included
only by `key_agreement.c`, so the blast radius is this TU.

## Report-only probes of PARKED siblings (variant files only; nothing retained)

New evidence from S7: the `enum message_type` local is what lets VC7 fold
`GET_MESSAGE_TYPE(x) == _message_type_packet`; a `byte` local reproduces January's
`shr/and/cmp`. Both parked siblings show exactly that fold, so one measurement each:

| probe | change | result |
| --- | --- | --- |
| P1 | `byte message_type` in `key_agreement_get_packet_type` | helper EXACT 80/80; but `complete_key_exchange` moves from `[sha]` to `[size 464!=416, relocs 21!=17]`: its disassembly shows the helper's line-0x4d assert inlined into the caller (no call remains) |
| P2 | `byte message_type` in `complete_key_exchange` only | `complete_key_exchange` `[size 464!=416, relocs 21!=17]`, helper unchanged |

Lead for whoever reopens the parked pair: the type lever is real for the helper,
and the remaining problem is keeping the helper out-of-line in `complete_key_exchange`
(January calls it). Not pursued here (PARKED, out of scope).

## Do-not-repeat

- Do not try to reach the 32-bit return with `boolean` declared: it is the declared
  return type, not an expression shape.
- `byte`-typed flags local (S5) and `enum`-typed message_type local (S7) are measured negatives.
- Reading the header once into a local before the store (single load) cannot reproduce
  the post-store reload `mov dl,[esi]`.

## Reopen criteria

The predicate reopens (and closes) the moment the header prototype is `long`; no
other work is required. Parked siblings: see the probe table above.

## Disposition

- `_is_message_encryption_key_message`: written, semantically complete, retained as a
  fuzzy residual (return width only) pending the header request. Not counted as closed.
- Newly exact functions with the real header: none. Whole-unit gate unchanged at 6/10.
- `fake_match_scan.py`: 0 leads.
- Bank of every measurement: `scratch/workers/key_agreement.log`; variant files
  `scratch/workers/key_agreement_s1..s7.c`, `_p1.c`, `_p2.c`, shadow header
  `scratch/workers/key_agreement_shadow_long.h`.

## Name provenance

| target symbol | source name | provenance | evidence |
| --- | --- | --- | --- |
| `_is_message_encryption_key_message` | `is_message_encryption_key_message` | unchanged (authenticated by the prior ledger's January PDB public-name evidence) | existing header prototype; target symbol is a real name, no alias needed |

## Orchestrator integration (2026-09-01, same session)

- The header request above was applied by the orchestrator: the `is_message_encryption_key_message`
  prototype in `source/bungie_net/common/key_agreement.h` and the definition in the `.c` now return
  `long`. Blast radius: `key_agreement.h` is included only by `key_agreement.c`, and no other
  translation unit references the symbol (`grep -rn is_message_encryption_key_message source`).
- Result: `gate.py source/bungie_net/common/key_agreement` -> `exact 7 residual 3 unwritten 0`;
  `_is_message_encryption_key_message` is strict EXACT (112 padded bytes, 4 relocations). The three
  residuals are the previously PARKED functions, untouched.
- Disposition updated: **newly exact +1 function / +112 strict padded bytes**.
