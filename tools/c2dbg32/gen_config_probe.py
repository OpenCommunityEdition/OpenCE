# -*- coding: utf-8 -*-
"""Point dbg32 at an arbitrary probe .c compile (default the rwprobe)."""
import sys, os
sys.path.insert(0, r'C:\Users\isabe\AppData\Local\Temp\claude')
import gate
CL=gate.CL
T=r'C:\Users\isabe\AppData\Local\Temp\claude'
def main(bp_vas, src=None):
    src=src or os.path.join(T,'rwprobe.c')
    out=os.path.join(os.getcwd(),'dbg_probe.obj')
    argv=[CL,'/nologo','/c','/O2','/Oy-','/Fo'+out, src]
    cmd=' '.join(('"%s"'%a if (' ' in a and not a.startswith('"')) else a) for a in argv)
    open('dbg_cmd.u16','wb').write((cmd+'\0').encode('utf-16-le'))
    open('dbg_dir.u16','wb').write((T+'\0').encode('utf-16-le'))
    open('dbg_bp.txt','wb').write((' '.join(bp_vas)+'\n').encode())
    print('wrote probe config;',len(bp_vas),'bps:',bp_vas)
if __name__=='__main__':
    main(sys.argv[1:])
