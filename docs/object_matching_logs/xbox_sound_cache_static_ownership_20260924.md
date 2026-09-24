# `xbox_sound_cache.obj` private ownership and held debug renderer (2026-09-24)

Six symbols that January's `cachebeta.pdb` omits from its public list are
file-private: four cache callbacks/helpers, `render_inverse_transform_screen_point`,
and `xbox_sound_cache_globals`. The three non-static function definitions and
the global are now `static` in their owning `.c` file; the other two definitions
already were. The six `config/symbols.json` entries now carry `static: true`.
No other translation unit imports these symbols. This is a storage correction,
not a code-byte gain.

`_sound_cache_debug_render` remains parked, and the object remains
`NonMatching` at **17/18** exact functions. A scratch reconstruction using the
later `/Od` graph's real `point_from_line3d` call, plus four coordinate locals
(`column`, `page_row`, `top`, `bottom`), gates 18/18 and passes the object and
selected-provider link audits. It is **not admitted**: the four locals exactly
satisfy a previously measured compiler IL-count oracle, while the first-party
`/Od` graph writes those expressions directly. January has no local records
to authenticate them. Byte equality here does not establish original source.
The held scratch copy is `scratch/xbox_sound_cache_exact_but_unadmitted.c`
(SHA-256 `404877deed86c9e9c41ae739e67a1192dcc68d43226811ca2ec123029a0c15ef`),
at zero credit. See `research/fifty_objects_20260925/w/xbox_sound_cache/LEDGER.md`
for the broader negative-probe inventory and reopen criterion.

Storage-only validation: full build, gate 17/18 with no row movement, whole-
object symbol audit (43 January symbols correctly owned), stable sweep with
zero regressions, parks valid, fake scan clean, and tools pytest passing.
