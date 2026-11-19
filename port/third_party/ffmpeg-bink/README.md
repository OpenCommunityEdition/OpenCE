# FFmpeg's Bink tables

`binkdata.h` is FFmpeg's `libavcodec/binkdata.h`, unchanged: the Bink video
decoder's tables (its block scan order, Huffman trees, run patterns and
quantizers), by Konstantin Shishkov. `COPYING.LGPLv2.1` is FFmpeg's copy of
the GNU Lesser General Public License, version 2.1, under which FFmpeg
releases it (version 2.1 or, at your option, any later version).

Upstream: https://github.com/FFmpeg/FFmpeg, tag `n7.1`.

| File | SHA-256 |
| --- | --- |
| `binkdata.h` | `e549d4f3df3debd0de047593db32ce9d671de210475c76aefd1a741b815b4080` |
| `COPYING.LGPLv2.1` | `b634ab5640e258563c536e658cad87080553df6f34f62269a21d554844e58bfe` |

`port/linux/src/bink_decoder.c` includes the header by relative path; no
build change is needed. That file is adapted from the same tag's
`libavcodec/bink.c`, `binkdsp.c` and `binkaudio.c` and `libavformat/bink.c`
(copyright Konstantin Shishkov, Peter Ross and Daniel Verkamp), keeps their
notices and is under the same licence. `port/linux/src/bink_rad.c` plays the
Xbox's movies with it. The releases carry the licence
(`ffmpeg-bink-COPYING.LGPLv2.1.txt`, and the web site's `licenses.txt`).
