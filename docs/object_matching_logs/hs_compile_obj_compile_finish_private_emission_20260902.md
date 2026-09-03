# `hs_compile.obj` finish/private-emission closure (2026-09-02)

## Outcome

This wave recovers `hs_compile_finish` and the three private string-constant
helpers that its authentic January call chain emits. All four functions are
strict exact, for four functions and 688 padded code bytes. The unit advances
from 12/64 functions and 2,000/17,072 bytes to 16/64 functions and
2,688/17,072 bytes. No park, matching exception, compiler option, or semantic
override was added.

The integration base is `cf85e4b81c29957d47dbb505afcb0f71b6a0be13`.

## Caller and naming evidence

January relocation evidence establishes the complete private call chain:

- the function formerly labeled `_code_000b8790` is the only caller of
  `_hs_concatenate_expression`, with two calls;
- `_hs_concatenate_expression` is the only caller of
  `_hs_concatenate_string_constant`, with three calls;
- `_hs_concatenate_string_constant` is the only caller of
  `_hs_find_string_constant`.

The HCEA source/PDB corpus names the root `hs_compile_finish` and describes the
same semantic operation: resize Scenario script-string storage, concatenate
every global initializer and script root, garbage-collect transient syntax
nodes, and reserve the final 1 KiB tail. That later corpus is used only for the
semantic name and field interpretation. January disassembly, relocations,
assertion data, and strict COFF comparison remain the byte authority.

The target root is externally visible, so its declaration is in the owning
`hs_library_internal_compile.h`. The three helpers remain file-static. This
avoids the earlier fake external-linkage workaround and emits them solely
through their real caller.

## Recovered source shape

The January root has one shared failure tail for both `tag_data_resize` calls.
An ordinary-C `boolean success` around the work followed by one final
`match_vassert` naturally produces that graph and January's exact
`"increase MAXIMUM_HS_STRING_DATA_PER_SCENARIO"` diagnostic.

Two semantic-preserving helper cleanups recover the January schedule without
steering constructs. The string helper records the old destination offset
after `memcpy`, immediately before advancing it. The expression helper tests
the existing source offset first and uses the function name in the fallback.
Both spellings are direct statements of the operation; neither introduces a
dummy dependency or changes behavior.

The exact sections are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_compile_finish` | 256 | 19 | `48d82dd1a23bb519cbe06df123b2846bf156f9048934766492ef74c17cdebe3c` |
| `_hs_find_string_constant` | 80 | 5 | `1c467a1b59d9b59debd8304b22c421761715e8eb752532e5efead313bee3ef8e` |
| `_hs_concatenate_string_constant` | 144 | 14 | `accdecf7ab51e58a510883a0f572adbbffb876a9ac55db841dc683760791fa4c` |
| `_hs_concatenate_expression` | 208 | 13 | `4aefb2add48f316886628dbc19a0f7bed26d542485a7caa1537a311c438f2829` |

## Verification

- Full `hs_compile.obj` gate: 16 exact, 27 residual, 21 unwritten. Its prior
  twelve exact functions remain exact.
- `hs.obj` gate after the owner-header declaration: 445 exact, 3 residual,
  zero unwritten.
- Full `ninja` rebuild: pass. Semantic audit: 473 units, 5,637 functions
  evaluated, 5,413 accepted exact, and zero unit errors.
- Strict board: 281/619 objects, 5,384/8,245 functions, and
  797,587/1,922,413 padded code bytes; the exact delta from the base board is
  four functions and 688 bytes.
- Test suite: 261/261 pass.
- Scoped fake-match scan: zero review leads. `git diff --check`: clean.
- The candidate emits one COMDAT for each recovered helper and root, with no
  `_code_000b8790` symbol and no `point_from_line3d` COMDAT.

The retained code uses typed Scenario, script, global, and tag-data fields,
the established typed tag-block macro, declaration-time initialization where
natural, multiline parameters, and explicit terminal returns. It contains no
assembly, raw address access, pointer punning, undefined behavior, forced
inline, pragma, `volatile`, register steering, inert carrier, or nonsensical
control flow.
