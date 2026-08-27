import sys, os
sys.path.insert(0, r'C:/Users/isabe/AppData/Local/Temp/claude')
import gate
UNIT='source/networking/telnet_console'
def main(bp_vas):
    flags=gate.cflags_for(UNIT)
    out=os.path.join(os.getcwd(),'dbg_telnet.obj')
    argv=[gate.CL]+flags+['/Fo'+out, UNIT+'.c']
    cmd=' '.join(('"%s"'%a if (' ' in a and not a.startswith('"')) else a) for a in argv)
    open('dbg_cmd.u16','wb').write((cmd+'\0').encode('utf-16-le'))
    open('dbg_dir.u16','wb').write((gate.ROOT+'\0').encode('utf-16-le'))
    open('dbg_bp.txt','wb').write((' '.join(bp_vas)+'\n').encode())
    print('wrote telnet config;',len(bp_vas),'bps:',bp_vas)
if __name__=='__main__':
    main(sys.argv[1:])
