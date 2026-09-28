# extract-xiso

An Xbox disc image (xdvdfs) extraction and creation tool by in
<in@fishtank.com>, under a modified BSD license (see `LICENSE.TXT`, copied
unchanged from upstream).

Upstream: https://github.com/XboxDev/extract-xiso, commit
3f5b62cfe68f000b0e3c8a30104973f3a297948e.

None of its code is built as it is: `port/linux/src/xiso.c`, which copies
the maps folder out of a disc image when a desktop port starts without game
data, reads the format as `extract-xiso.c` does and carries its notice. The
desktop builds' artifacts include `LICENSE.TXT` as
`extract-xiso-LICENSE.txt`.

This product includes software developed by in <in@fishtank.com>.
