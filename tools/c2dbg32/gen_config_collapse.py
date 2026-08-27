# -*- coding: utf-8 -*-
"""Config generator for the integer base-collapse tie (units unit_enter_seat).

Unlike the ai_debug/telnet generators this does not need the ad/gate harness:
it points dbg32 at a *minimal repro* compile, so a full debugger run costs
milliseconds and a differential (collapse vs no-collapse) is cheap.

Usage:
  python gen_config_collapse.py <repro.c> <bp-va> [more VAs...]
then run dbg32.exe in this directory; results land in dbg_result.txt.
"""
import sys, os

CL = r'C:\halo-worktrees\claude-untried-objects\xbox\bin\vc7\CL.Exe'


def main(src, bp_vas):
    here = os.getcwd()
    src = os.path.abspath(src)
    out = os.path.join(here, 'dbg_repro.obj')
    argv = [CL, '/nologo', '/c', '/O2', '/Oy-', '/DDEBUG', '/Dxbox',
            '/Fo' + out, src]
    cmdline = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a)
                       for a in argv)
    open('dbg_cmd.u16', 'wb').write((cmdline + '\0').encode('utf-16-le'))
    open('dbg_dir.u16', 'wb').write((here + '\0').encode('utf-16-le'))
    open('dbg_bp.txt', 'wb').write((' '.join(bp_vas) + '\n').encode())
    print('src :', src)
    print('bps :', bp_vas)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    main(sys.argv[1], sys.argv[2:])
