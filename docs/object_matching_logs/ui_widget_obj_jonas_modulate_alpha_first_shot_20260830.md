# `ui_widget.obj` real-alpha modulation codegen boundary

## Result

`_modulate_pixel32_by_real_alpha` remains unwritten after one natural typed
C89 production candidate. The candidate matched the target's 64 padded bytes
but not its complete normalized code hash or relocation stream. No alternate
spelling, tuning, compiler switch, or sweep was tried.

The January target has one relocation and normalized SHA-256
`2b826837864844e7029d6b020508d1f08ef53876fcef2e2c127abfb381ed59c5`.
The first-shot candidate has two relocations and normalized SHA-256
`dfbff510ccc9757671866eb13e589ab36063532001a9c995a75e12db7175fff3`.
The immutable first-shot object is
`build/audit/ui_widget_modulate_alpha_first_shot_20260830.obj`, 5,324 bytes,
with whole-object SHA-256
`5e33dce76b6d05cf34a158ded1e6ca631f5218de8ea0ee74bb905d0c04c9d500`.

## Evidence and fixed boundary

January's instruction stream preserves the input pixel in EAX, extracts its
unsigned high byte into ECX, converts that value to floating point with the
usual `__real@4f800000` correction at function offset `+0x17`, multiplies by
the `real` argument, stores and reloads a floating temporary, and uses an
inline x87 `fistp` conversion before masking the original RGB bytes, shifting
the converted alpha by 24, and combining the fields.

The readable candidate expressed exactly that pixel operation through the
existing public `pixel32`/`real` declaration. VC7 instead saved the original
pixel in ESI and emitted the conversion through a relocation to `__ftol2` at
`+0x20`; its unsigned-conversion constant relocation was at `+0x18`. The
target has no `__ftol2` relocation and uses a different register/frame layout.
This is a fixed float-to-integer code-generation boundary, not missing
semantic evidence.

The January-compatible pastudan history at commit
`918af885935ec470a31256ecce9a977b12b01f80` corroborates the behavior. Its
readable tool fragment has blob `d775b73efc485dbd44732f3a7ae54ccc349095fa`;
the related source blob `5820b6c80657ad09d1ae711ddf904b362218bcc7`
also contains an assembly workaround, which was rejected under this
campaign's source rules.

## Scope and one-shot discipline

Relevant interface ledgers, target disassembly, declarations, tool
documentation, registered repository trees, and the applicable project and
Claude `CLAUDE.md` files were read before editing. Searches of current trees,
ledgers, and ancestry found no competing or previously emitted body. Where
the Claude material describes a different functional reconstruction workflow,
the stricter byte-exact campaign rules remained authoritative.

The sole candidate compiled exactly once on the production `ui_widget.obj`
edge. Its focused gate preserved all 16 inherited exact functions and reported
this function as the only new residual, with 85 functions still unwritten.
The candidate was then inverse-reverted exactly; the retained source blob is
again `b01532b96b805b24247534bf409471de66977b60`.

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or offset, pointer or union pun, undefined
behavior, synthetic anchor, or byte forcing. Units, Vehicles, Matrix Math, AI
Debug, configuration, semantic exceptions, parks, completion labels, and
build rules were untouched.
