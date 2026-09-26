import sys,re,difflib,collections
sys.path.insert(0,'scratch/lane/w/ai__ai_debug')
from sdiff import load,norm
J=load(sys.argv[1]);O=load(sys.argv[2])
a=[norm(x,2) for x in J]; b=[norm(x,2) for x in O]
sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
pairs=collections.defaultdict(collections.Counter)
where=collections.defaultdict(list)
for op,i1,i2,j1,j2 in sm.get_opcodes():
    if op=='equal' or (op=='replace' and i2-i1==j2-j1):
        for k in range(i2-i1):
            x=J[i1+k];y=O[j1+k]
            sx=re.findall(r'ebp - (0x[0-9a-f]+)',x[2]); sy=re.findall(r'ebp - (0x[0-9a-f]+)',y[2])
            if len(sx)==len(sy):
                for p,q in zip(sx,sy):
                    pairs[int(p,16)][int(q,16)]+=1
                    if p!=q: where[(int(p,16),int(q,16))].append(x[0])
for p in sorted(pairs):
    c=pairs[p]
    if len(c)==1 and p in c: continue
    print('J -0x%-5x -> '%p + ', '.join('O -0x%x x%d'%(q,n) for q,n in c.most_common()))
print('==== reverse')
rev=collections.defaultdict(collections.Counter)
for p,c in pairs.items():
    for q,n in c.items(): rev[q][p]+=n
for q in sorted(rev):
    c=rev[q]
    if len(c)==1 and q in c: continue
    print('O -0x%-5x <- '%q + ', '.join('J -0x%x x%d'%(p,n) for p,n in c.most_common()))
if len(sys.argv)>3:
    for k in sys.argv[3:]:
        p,q=[int(v,16) for v in k.split(':')]
        print(k, ' '.join('%x'%w for w in where[(p,q)]))
