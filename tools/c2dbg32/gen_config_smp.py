# -*- coding: utf-8 -*-
"""Config generator for a c2 live-debug/sample of the stack_memory_pool compile.

Writes dbg_cmd.u16/dbg_dir.u16/dbg_bp.txt (for dbg32.exe) and dbg_cmd.json
(for acc_sampler.py). Usage: python gen_config_smp.py [VA ...]
"""
import sys, os, json
sys.path.insert(0, r'C:\Users\isabe\AppData\Local\Temp\claude')
import gate

UNIT = 'source/memory/stack_memory_pool'


def main(bp_vas):
    flags = gate.cflags_for(UNIT)
    out = os.path.join(os.getcwd(), 'dbg_smp.obj')
    argv = [gate.CL] + flags + ['/Fo' + out, UNIT + '.c']
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    open('dbg_cmd.u16', 'wb').write((cmdline + '\0').encode('utf-16-le'))
    open('dbg_dir.u16', 'wb').write((gate.ROOT + '\0').encode('utf-16-le'))
    open('dbg_bp.txt', 'wb').write((' '.join(bp_vas) + '\n').encode())
    json.dump({'argv': argv, 'root': gate.ROOT}, open('dbg_cmd.json', 'w'))
    print('wrote smp config; %d bps:' % len(bp_vas), bp_vas)


if __name__ == '__main__':
    main(sys.argv[1:])
