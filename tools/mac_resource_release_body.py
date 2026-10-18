#!/usr/bin/env python
"""Extract the real shared Release implementation for owner-contract tests."""
import sys
source = open('port/linux/src/d3d8_resources.c').read()
start = source.index('ULONG WINAPI D3DResource_Release(')
end = source.index('\nBOOL WINAPI D3DResource_IsBusy', start)
body = source[start:end].replace('ULONG WINAPI D3DResource_Release(', 'static ULONG WINAPI mac_shared_resource_release(', 1)
with open(sys.argv[1], 'w') as out:
    out.write('/* Generated from port/linux/src/d3d8_resources.c; do not edit. */\n')
    out.write(body)
