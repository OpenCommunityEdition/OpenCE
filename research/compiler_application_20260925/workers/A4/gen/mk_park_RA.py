"""Line-surgical re-baseline of the PARKED __rasterizer_model_draw entry for the R-A header hunk.
    python mk_park_RA.py <out_parked.json>"""
import sys

WT = 'C:/halo-worktrees/claude-compiler-application-20260925'
raw = open(WT + '/config/parked.json', 'rb').read().decode('utf-8')
assert '\r\n' in raw
lines = raw.split('\r\n')
idx = [i for i, l in enumerate(lines) if l.strip() == '"function": "__rasterizer_model_draw",']
assert len(idx) == 1, idx
i = idx[0]
ev = i + 2
assert lines[i - 1].strip() == '"unit": "source/rasterizer/xbox/rasterizer_xbox_models",'
assert lines[ev].strip().startswith('"evidence": "') and lines[ev].endswith('",'), lines[ev][:80]
lines[ev] = lines[ev][:-2] + (
    ' 2026-09-26 (compiler-application lane, worker A4, packet R-A): removing the stale extern prototype of'
    ' rasterizer_filthy_bitmap_default_initialize from rasterizer_xbox_internal.h (the function becomes the'
    ' January-static rasterizer_filthy_bitmap_defaults_initialize of rasterizer_xbox.c) changes only this unchanged,'
    ' already-fuzzy body\'s allocation hash at identical size and relocations (5168/348, sha cbfa8585...;'
    ' objdiff 95.07629 as measured for the same hash on 2026-09-24), a header declaration-count effect; every other'
    ' section of this object and of the six other includers is identical. No source change.",')
base = [j for j in range(i, i + 12) if '"base": { "size": 5168, "relocation_count": 348, "normalized_sha256": "89b0d7ea77a71d69d21eda1926e973006f54ed531bf346aaa87a18718e225c01" },' in lines[j]]
assert len(base) == 1
lines[base[0]] = lines[base[0]].replace('89b0d7ea77a71d69d21eda1926e973006f54ed531bf346aaa87a18718e225c01',
                                        'cbfa85852c836af123fe93a3f15f22f2f43ba574fbff9add3c28f0e0275ed4d4')
od = [j for j in range(i, i + 12) if lines[j].strip() == '"objdiff_percent": 95.08453']
assert len(od) == 1
lines[od[0]] = lines[od[0]].replace('95.08453', '95.07629')
open(sys.argv[1], 'wb').write('\r\n'.join(lines).encode('utf-8'))
print('ok')
