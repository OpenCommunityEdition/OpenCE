# `libs/libcmt/wcscat.obj` — MATCHING

**Status: byte-exact. Both symbols, whole object.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `Matching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** `/O1 /Gy /I"xbox/include"`
**Start state:** 0.00% — object `MISSING`, no source.

| symbol | target | ours | exact |
|---|--:|--:|:--:|
| `_wcscat` | 42 B / 0 rel | 42 B / 0 rel | **yes** |
| `_wcscpy` | 28 B / 0 rel | 28 B / 0 rel | **yes** |

`section_infos_equal` passes for both. Normalized SHA-256:
`_wcscat` `2794a8ea379ab443…`, `_wcscpy` `400344dea9c2f17d…`.
With zero relocations in either function the `.text` sections are literally
byte-identical, not merely equal under masking.

---

## 1. Two functions, one translation unit

The object defines **both** `_wcscat` and `_wcscpy`, in two `/Gy` COMDAT `.text`
sections. There is no `libs/libcmt/wcscpy` unit in `config/config.json` — the CRT
keeps `wcscpy` in `WCSCAT.C`, and the object confirms it. Section order is
`wcscat` then `wcscpy`, which is the source order.

No externals and no data sections: the whole object is these two leaf functions.

## 2. Shapes recovered from the disassembly

**`_wcscat`** — note the scan loop is *rotated*: the target tests `*cp` once up
front (`cmp word ptr [eax],0` / `je`) and then runs a `do`-style body
(`inc; inc; cmp; jne`). That is what a plain `while (*cp) cp++;` compiles to at
`/O1`, so no special shaping was needed.

```
 0  mov eax,[esp+4]      ; dst -- also the return value, never clobbered
 4  cmp word ptr [eax],0
 8  mov edx,eax          ; cp = dst
 a  je 0x14
 c  inc edx / inc edx    ; cp++
 e  cmp word ptr [edx],0
12  jne 0xc
14  push esi
15  mov esi,[esp+0xc]    ; src (esp+8, +4 for the push)
19  mov cx,[esi]         ; while (*cp++ = *src++)
1c  mov [edx],cx
1f  inc edx / inc edx
21  inc esi / inc esi
23  test cx,cx
26  jne 0x19
28  pop esi
29  ret
```

The return value needs no code at all: `dst` is loaded into EAX at offset 0 and
EAX is never touched afterwards (the function works in EDX/ECX/ESI), so `return
dst;` costs nothing. Writing it any other way — caching `dst` in a local, or
returning `cp` — would have produced a reload.

**`_wcscpy`** is the same copy loop without the scan, and does reload
`mov eax,[esp+4]` at the end because EAX is used as the character temp.

Neither function uses a frame pointer; both address arguments off ESP directly.

## 3. Source

```c
typedef unsigned short wchar_t;

wchar_t * __cdecl wcscat(wchar_t *dst, const wchar_t *src)
{
	wchar_t *cp = dst;

	while (*cp)
		cp++;					/* find end of dst */

	while (*cp++ = *src++)
		;						/* Copy src to end of dst */

	return dst;
}

wchar_t * __cdecl wcscpy(wchar_t *dst, const wchar_t *src)
{
	wchar_t *cp = dst;

	while (*cp++ = *src++)
		;						/* Copy src over dst */

	return dst;
}
```

`wchar_t` is declared locally as `unsigned short` — in C mode MSVC does not treat
it as a keyword, and pulling in a header for it is unnecessary here since the
object has no other dependencies.

Byte-exact on the first build; no iteration was required.

## 4. Notes for the rest of libcmt

* The `/O1 /Gy` recipe established by the `stream` work in this lane is what makes
  these objects buildable; `wcscat` is the first libcmt object to come out fully
  matching.
* Leaf string functions like this are the cheapest wins in the project: no
  relocations, no externals, no data, and the whole shape is legible from a
  40-byte disassembly. The neighbouring `wcs*` units (`wcslen`, `wcschr`,
  `wcscmp`, `wcsncat`, `wcsncpy`, `wcsrchr`, `wcsspn`, …) are all still `MISSING`
  and are very likely the same profile.
* Watch for units that quietly contain more than one function, as this one does.
  Check the target's symbol table before assuming a one-to-one mapping between
  the config entry and the function.

> Note: `ninja` cannot launch `xbox/bin/vc7/CL.Exe` in this checkout
> (`CreateProcess failed`); this is pre-existing and reproduces in
> `campaign-integration`. Verification was done by invoking CL directly with the
> exact cflags ninja generates.
