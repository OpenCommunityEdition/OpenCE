import sys, os
sys.path.insert(0, r'C:/Users/isabe/AppData/Local/Temp/claude')
import gate
# Single-function TU: repro_00198fd0 is the ONLY function, so no per-function
# gating is needed - every c2 breakpoint hit belongs to this compile.
UNIT = 'tools/c2dbg32/repro_00198fd0'
def main(bp_vas):
    flags = gate.cflags_for('source/units/units')  # borrow units.c flags (repro not in ninja)
    out = os.path.join(os.getcwd(), 'dbg_repro198.obj')
    argv = [gate.CL] + flags + ['/Fo' + out, UNIT + '.c']
    cmd = ' '.join(('"%s"' % a if (' ' in a and not a.startswith('"')) else a) for a in argv)
    open('dbg_cmd.u16', 'wb').write((cmd + '\0').encode('utf-16-le'))
    open('dbg_dir.u16', 'wb').write((gate.ROOT + '\0').encode('utf-16-le'))
    open('dbg_bp.txt', 'wb').write((' '.join(bp_vas) + '\n').encode())
    # no gate file for a single-function TU
    if os.path.exists('dbg_gate.txt'):
        os.remove('dbg_gate.txt')
    print('wrote repro198 config;', len(bp_vas), 'bps:', bp_vas)
if __name__ == '__main__':
    main(sys.argv[1:])
