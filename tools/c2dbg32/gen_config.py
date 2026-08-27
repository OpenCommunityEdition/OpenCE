# -*- coding: utf-8 -*-
"""Generate dbg32.exe's config files for a c2.dll live-debug of ai_debug.

Writes (in the current directory, next to dbg32.exe):
  dbg_cmd.u16  UTF-16LE CL.Exe command line (null-terminated)
  dbg_dir.u16  UTF-16LE working directory (null-terminated)
  dbg_bp.txt   ASCII whitespace-separated breakpoint VAs (c2 is at 0x10700000)

Usage:
  python gen_config.py 0x10701400 [more VAs...]
then run dbg32.exe; results land in dbg_result.txt.
"""
import sys, os
sys.path.insert(0, r'C:\Users\isabe\AppData\Local\Temp\claude')  # ad/gate harness
import gate, ad

def main(bp_vas):
    flags = gate.cflags_for(ad.UNIT)
    out = os.path.join(os.getcwd(), 'dbg_ai_debug.obj')
    argv = [gate.CL] + flags + ['/Fo' + out, ad.UNIT + '.c']
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    open('dbg_cmd.u16', 'wb').write((cmdline + '\0').encode('utf-16-le'))
    open('dbg_dir.u16', 'wb').write((gate.ROOT + '\0').encode('utf-16-le'))
    open('dbg_bp.txt', 'wb').write((' '.join(bp_vas) + '\n').encode())
    print('wrote config; %d breakpoints:' % len(bp_vas), bp_vas)

if __name__ == '__main__':
    main(sys.argv[1:] or ['0x10701400'])
