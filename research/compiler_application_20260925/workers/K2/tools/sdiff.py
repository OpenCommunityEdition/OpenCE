import sys,re,difflib
def load(p):
    L=[]
    for line in open(p):
        m=re.match(r'\s*([0-9a-f]+)\s+(\S+)\s*(.*)$',line.rstrip())
        if not m: continue
        a=int(m.group(1),16); mn=m.group(2); rest=m.group(3)
        tgt=''
        if ';' in rest:
            rest,c=rest.split(';',1); tgt=c.strip()
            tgt=re.sub(r'^-> ','',tgt)
        rest=rest.strip()
        L.append((a,mn,rest,tgt))
    # strip trailing filler
    while L and L[-1][1] in ('nop','int3'): L.pop()
    return L
regs=r'\b(e?[abcd]x|e?[sd]i|[abcd][lh]|st\(\d\))\b'
def norm(x,level):
    a,mn,rest,tgt=x
    r=re.sub(r'ebp - 0x[0-9a-f]+','ebp-S',rest)
    if mn.startswith('j') or mn=='call':
        r=''
    if level>=2: r=re.sub(regs,'R',r)
    return mn+' '+r+' '+tgt
def _main():
 J=load(sys.argv[1]);O=load(sys.argv[2]);level=int(sys.argv[3]) if len(sys.argv)>3 else 2
 a=[norm(x,level) for x in J]; b=[norm(x,level) for x in O]
 sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
 tot=0
 for op,i1,i2,j1,j2 in sm.get_opcodes():
     if op=='equal': continue
     if op=='replace' and i2-i1==j2-j1 and level>=3: continue
     tot+= (j2-j1)-(i2-i1)
     print('--- %s J[%d:%d] O[%d:%d] @J %x @O %x  d=%+d'%(op,i1,i2,j1,j2,J[i1][0] if i1<len(J) else -1,O[j1][0] if j1<len(O) else -1,(j2-j1)-(i2-i1)))
     for x in J[i1:i2]: print('  J %5x %s %s %s'%x)
     for x in O[j1:j2]: print('  O %5x %s %s %s'%x)
 print('J',len(J),'O',len(O),'net',tot)

if __name__=="__main__": _main()
