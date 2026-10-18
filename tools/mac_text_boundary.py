#!/usr/bin/env python3
"""Generate actual CPU font services and endian-neutral font byte arrays."""
from pathlib import Path
import argparse,json
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
source=Path('port/linux/src/text_hires.c').read_text()
marker='unsigned int text_hires_atlas_texture(unsigned long data)'
if source.count(marker)!=1:raise SystemExit('Expected one GL atlas upload boundary')
upload=source[source.index(marker):].replace('text_hires_atlas_texture(', 'mac_text_gl_upload_shared(', 1).replace('xgpu_gl_state_invalidate()', 'mac_text_gl_invalidate()')
(a.output/'mac_text_gl_body.h').write_text('/* Shared atlas upload function; native invalidation callback only. */\n'+upload)
source=source[:source.index(marker)]
for before,after in [('#include "platform.h"','#include "engine_text.h"'),('#include "xgpu.h"',''),('static GLuint atlas_texture;',''),('#include "../../third_party/stb/stb_truetype.h"','#include "port/third_party/stb/stb_truetype.h"'),('platform_log(', 'mac_text_log('),('halo_screen_pixel_scale()', 'mac_text_pixel_scale()')]:
    if before not in source:raise SystemExit('Missing expected CPU text boundary: '+before)
    source=source.replace(before,after)
(a.output/'mac_text_body.h').write_text('#line 1 "port/linux/src/text_hires.c"\n'+source)
root=Path('port/assets/fonts');fonts=json.loads((root/'fonts.json').read_text())['fonts'];files=list(dict.fromkeys(f['file'] for f in fonts));out=['#include "text_hires.h"']
for i,name in enumerate(files):
    data=(root/name).read_bytes();out.append('static const unsigned char mac_font%d[] __attribute__((aligned(4))) = {'%i)
    for offset in range(0,len(data),20):out.append(','.join('0x%02x'%b for b in data[offset:offset+20])+',')
    out.append('};')
out.append('const struct text_hires_embedded text_hires_embedded[] = {')
for f in fonts:
    i=files.index(f['file']);size=(root/f['file']).stat().st_size
    out.append('{%s,%s,(const unsigned int *)mac_font%d,%d},'%(json.dumps(f['tag']),json.dumps(f['file']),i,size))
out+=['};','const unsigned int text_hires_embedded_count = %d;'%len(fonts)]
(a.output/'mac_text_fonts.c').write_text('\n'.join(out)+'\n')
