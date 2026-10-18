#!/usr/bin/env python
"""Checked native view: an LRU's embedded data array is not a heap allocation."""
import sys
source = open('source/memory/lruv_cache.c').read()
old = '\tlruv_cache_verify(cache, TRUE);\n\tdata_dispose(cache->blocks);\n\tcsmemset(cache, 0, sizeof(*cache));'
if source.count(old) != 1 or source.count('struct data_array *blocks = (struct data_array *)(cache + 1);') != 1:
    raise SystemExit('LRU ownership source boundary changed')
new = '''\tlruv_cache_verify(cache, TRUE);
\t/* port: lruv_new allocates the header and data array together;
\t   verify and clear the embedded array, then free only the parent. */
\tmatch_assert(__FILE__, __LINE__, cache->blocks == (struct data_array *)(cache + 1));
\tdata_verify(cache->blocks);
\tcsmemset(cache->blocks, 0, sizeof(*cache->blocks));
\tcsmemset(cache, 0, sizeof(*cache));'''
if len(sys.argv) != 2:
    raise SystemExit('usage: mac_lruv_view.py output.c')
with open(sys.argv[1], 'w') as output:
    output.write(source.replace(old,new))
