"""Exercise the real Windows page watch, including cross-reservation fallback."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.skipif(os.name != 'nt', reason='Windows virtual memory API')
def test_batched_page_protection(tmp_path):
    compiler = shutil.which('clang')
    if not compiler:
        pytest.skip('clang required')
    source = r'''
#include <windows.h>
#include <assert.h>
#include <stdarg.h>
static unsigned protection_calls;
static BOOL counted_protect(void *p, SIZE_T bytes, DWORD flags, DWORD *previous) {
    protection_calls++;
    return VirtualProtect(p, bytes, flags, previous);
}
void platform_log(char const *format, ...) { (void)format; }
#define VirtualProtect counted_protect
'''
    source += (ROOT/'port/windows/src/win32_memory_watch.c').read_text()
    source += r'''
int main(void) {
    unsigned long base=PLATFORM_CONTIGUOUS_BASE, before, calls;
    unsigned i;
    volatile unsigned char *bytes=(void *)base;
    assert(VirtualAlloc((void *)base,0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)==(void *)base);
    assert(VirtualAlloc((void *)(base+0x10000),0x10000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE)==(void *)(base+0x10000));
    memory_watch_initialize();assert(watch_active);
    memory_watch_protect(base+3,0x8000-3);assert(protection_calls==1);
    calls=protection_calls;memory_watch_protect(base,0x8000);assert(protection_calls==calls);
    for(i=0;i<8;i++) {
        before=memory_watch_generation(base+i*0x1000,0x1000);
        bytes[i*0x1000]=7;
        assert(memory_watch_generation(base+i*0x1000,0x1000)>before);
    }
    calls=protection_calls;
    memory_watch_protect(base,0x20000);
    assert(protection_calls==calls+33); /* combined request plus 32 page fallback */
    for(i=0;i<32;i++) {
        before=memory_watch_generation(base+i*0x1000,0x1000);
        bytes[i*0x1000]=8;
        assert(memory_watch_generation(base+i*0x1000,0x1000)>before);
    }
    memory_watch_protect(base,0x10000);
    before=memory_watch_serial();memory_watch_prepare_write((void *)(base+0x2000),0x2000);
    assert(memory_watch_generation(base+0x2000,0x2000)>before);
    calls=protection_calls;memory_watch_protect(base,0x10000);assert(protection_calls==calls+1);
    before=memory_watch_serial();memory_watch_forget((void *)base,0x20000);
    assert(memory_watch_generation(base,0x20000)>before);
    VirtualFree((void *)base,0,MEM_RELEASE);VirtualFree((void *)(base+0x10000),0,MEM_RELEASE);
    return 0;
}
'''
    path = tmp_path/'watch.c'
    path.write_text(source)
    binary = tmp_path/'watch.exe'
    result = subprocess.run([compiler, '--target=i686-pc-windows-msvc', '-fuse-ld=lld',
                             '-std=gnu89', str(path), '-o', str(binary),
                             '-Wl,/largeaddressaware'], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    subprocess.run([str(binary)], check=True)
