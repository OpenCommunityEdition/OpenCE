> Packet18 final disposition: all three source/header probes restored, zero admission. Qualification packets precede their measured results; production remains wave17.

# Packet 18: `ai_conversation_new` private owner

**Qualified for one linkage-only test; no landing or new credit established.** Add `static` to the existing definition in `source/ai/ai_communication.c` and remove only its prototype from `source/ai/ai_communication.h`. Keep its body, signature and position unchanged. The definition precedes the sole caller, so no replacement forward declaration is needed.

The independent evidence is reproducible with `python scratch/astra-wave18/communication-private-owner/audit.py`; complete raw records, hashes and resolved calls are in `audit.json`. This audit ran no compiler and changed no production, header, configuration or target metadata.

| Original input | Private MAP record | Bound original XBE window |
|---|---|---|
| August 15, 2001 | Line 20595, section 1 + `0x2cee0`, `_ai_conversation_new`, flag `f`, `ai_communication.obj` | File `0x2dee0`, VA `0x3dee0`, 304 bytes |
| September 25, 2001 | Line 21993, section 1 + `0x31350`, same name, flag and owner | File `0x32350`, VA `0x42350`, 304 bytes |

Both records occur under the MAP's **Static symbols** heading. In both builds, the preceding named owner is `ai_conversation_finish` and the next is `ai_conversation_line_begin`. The complete 304-byte windows, with January relocation positions excluded, equal January and current code: normalized SHA-256 `5a0b98904215778ab0c49562dad691bd01ca3ace2fbd634cb0d10a8cfe547042`, 17 relocations. Independently decoding all 11 direct E8 destinations in each original XBE binds them to the expected named MAP callees. January's actual executable also resolves all 11 calls to those identities. Cross-build data addresses are not claimed identical. The August `1749betaP.xbe` filename retains its patched-image caveat; September independently supplies the complete correspondence.

January's supplied public-symbol inventory has no `ai_conversation_new` entry. Its current split COFF symbol has storage class 2 and type `0x20`; that is synthesized split metadata, not an original private/public source declaration. The August 31 small-trio report retained external linkage specifically from this COFF storage class. The new original private records and full body/call binding supply the missing independent linkage evidence without changing that metadata.

A fresh bounded raw HCEA PDB read of the `ai_communication.obj` compiland finds `S_LPROC32` at stream 2601 + `0x2208`. Procedure type `0x6e329` returns signed long (`0x12`) with short (`0x11`) and unsigned char (`0x20`) arguments. The current project defines `boolean` through unsigned-char `byte`; its signature already agrees. HCEA independently corroborates private linkage and types, but neither the original lexical text nor a common source revision is claimed.

An identifier scan excluding comments and string literals finds exactly three source tokens: the header prototype at line 159, definition at C line 5149 and same-TU call at line 5530. It scans bare identifiers as well as call forms; there is no function-pointer or cross-owner use. The existing scripted-overwrite predicate and all other body behavior remain untouched.

Root's frozen `scratch/astra-wave18/communication-context/baseline.json` supplies all 21 actual Ninja and static include consumers. Every inherited exact control, runtime owner, datum, COMMON and point definition must be preserved in the test. The conversation function's 304 padded bytes are already exact and cannot be counted again. The 8,064-byte communication-event residual is a possible context beneficiary, not a predicted closure; no generic compiler law follows from the private-owner proof.

Evidence ownership **RELEASED**. This qualification authorizes no production landing by itself.
