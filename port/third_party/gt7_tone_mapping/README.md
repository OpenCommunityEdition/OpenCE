# GT7 tone mapping

Polyphony Digital's sample implementation of the Gran Turismo 7 tone mapping
operator, from "Driving Toward Reality: Physically Based Tone Mapping and
Perceptual Fidelity in Gran Turismo 7" (Kenichiro Yasutomi, Kentaro Suzuki
and Hajime Uchimura, SIGGRAPH 2025 course "Physically Based Shading in
Theory and Practice"), MIT licensed (see `LICENSE`).

Upstream:
https://blog.selfshadow.com/publications/s2025-shading-course/pdi/supplemental/gt7_tone_mapping.cpp,
version 1.0 (2025-08-10), SHA-256
94df7e7e310b9423ebb5aaaa29416f376ebbae66e0948fb259f8688314b29822. `gt7_tone_mapping.cpp` is copied unchanged.

It is not compiled. `display.hdr_tone_mapping = "gt7"` (the default) runs
its operator, ICtCp variant, written again in GLSL in
`port/linux/src/xgpu_post.c`'s tone mapping pass: GTToneMappingCurveV2 and
GT7ToneMapping's parameters as they are here, applied to the game's linear
light in Rec.2020 at its brightness in nits (`display.hdr_paper_white`), to
an HDR display's peak (`display.hdr_peak_brightness`), else as its SDR
(250 nits).
