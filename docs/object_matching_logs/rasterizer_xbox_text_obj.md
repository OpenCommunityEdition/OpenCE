# `source/rasterizer/xbox/rasterizer_xbox_text.obj` strategy ledger

## Strict result

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with the repository flags
  `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: `build/split/source/rasterizer/xbox/rasterizer_xbox_text.obj`.
- All 12 functions pass `tools/coff_compare.py`: 3,232 padded code bytes and
  186 relocations, including relocation destinations and addends.
- All five target-owned `.rdata` sections pass strict comparison: 450 logical
  bytes and zero relocations. The object owns no target `.data` or `.bss`.

## Final residual and resolution

The final residual was `_IDirect3DDevice8_SetRenderState@12`. A local
`#define D3DINLINE static` made XDK emit a 16-byte thunk while January owns the
full 544-byte wrapper. Removing that override restores the stock XDK
`D3DINLINE` definition (`static __forceinline`), and the same compiler emits
the January wrapper exactly.

This is source-authentic SDK emission. No wrapper body was handwritten, and no
assembly, `volatile`, undefined behavior, byte patch, or compiler-flag change
is involved.

## Preserved experiment history

- Stock `<xtl.h>` with its default `D3DINLINE`: all nine XDK wrapper bodies
  exact, including the 544-byte render-state wrapper.
- Local `#define D3DINLINE static`: eight wrappers retained their code bytes,
  but the 544-byte wrapper collapsed to the wrong 16-byte thunk.
- Taking a wrapper's address: rejected; it forces a different standard
  `__stdcall` body and ABI shape.
- Handwritten Microsoft dispatcher: rejected as unnecessary and contrary to
  the source-reconstruction standard.

The stock-header change also moves eight already-exact wrappers from COMDAT
selection 1 to the target's selection 2 and changes one compiler debug
section. Those expected evidence changes are hash-pinned in
`config/regression_adjudications_xdk_text.json`; every function remains strict
exact and no game-data section changes.

## Reopen rule

Do not reintroduce a local `D3DINLINE` override or hand-maintain copies of XDK
dispatcher code. If a future XDK/header change affects this object, require a
12-function strict sweep, all five owned data sections, and the whole-TU
regression gate before accepting it.
