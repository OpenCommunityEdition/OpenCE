"""Branch-width census for one function: January vs ours.
Reports: aligned branch-width byte delta, near-but-fits per side, width-normalised code length
(every branch re-relaxed to the final-layout fixpoint), and same-span flips."""
import sys,os,re,difflib
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from relax import getsec, codelimit, isbr, md, build, sim, layout
R='C:/halo-worktrees/claude-compiler-application-20260925/'
fn,unit,ourp=sys.argv[1],sys.argv[2],sys.argv[3]
jp=R+'build/split/source/ai/%s.obj'%unit
def load(p):
    data,rels=getsec(p,fn); lim=codelimit(rels,len(data))
    if lim>=len(data)-4: lim=len(data)
    ins=list(md.disasm(bytes(data[:lim]),0))
    # real code end: strip trailing int3/nop filler
    end=len(ins)
    while end>0 and (ins[end-1].mnemonic in ('int3','nop') or (ins[end-1].mnemonic=='lea' and ins[end-1].op_str.split(',')[0].strip() in ins[end-1].op_str.split(',')[1])): end-=1
    ins=ins[:end]
    N=[]
    for i in ins:
        s=i.mnemonic+' '+i.op_str
        if isbr(i.bytes) or i.mnemonic=='call': s=i.mnemonic
        s=re.sub(r'ebp - 0x[0-9a-f]+','ebp-S',s)
        N.append(s)
    return ins,N,rels,len(data)
def nbf(ins,rels):
    out=[]
    for i in ins:
        if not isbr(i.bytes) or i.size==2: continue
        if any((i.address+o) in rels for o in range(1,i.size)): continue
        t=int(i.op_str,16)
        if t>i.address and (t-(i.size-2))-(i.address+2)<=127: out.append(i.address)
    return out
def normlen(ins,rels):
    items=build(ins,rels); sim(items); return layout(items)[-1]
JI,JN,JR,JS=load(jp); OI,ON,OR,OS=load(ourp)
jend=JI[-1].address+JI[-1].size; oend=OI[-1].address+OI[-1].size
sm=difflib.SequenceMatcher(None,JN,ON,autojunk=False)
wd=0; flips=[]
for tag,i1,i2,j1,j2 in sm.get_opcodes():
    if tag!='equal': continue
    for k in range(i2-i1):
        a=JI[i1+k]; b=OI[j1+k]
        if isbr(a.bytes) and isbr(b.bytes) and a.size!=b.size:
            wd+=b.size-a.size; flips.append((a.address,b.address,a.mnemonic,a.size,b.size))
jn=normlen(JI,JR); on=normlen(OI,OR)
print('%s: section J %d / O %d (%+d)   real code end J %#x / O %#x (%+d)'%(fn,JS,OS,OS-JS,jend,oend,oend-jend))
print('   aligned branch-width delta (ours-jan): %+d bytes over %d branches'%(wd,len(flips)))
for f in flips: print('      J@%-5x O@%-5x %-4s J%d O%d'%f)
print('   near-but-fits: J %d  O %d'%(len(nbf(JI,JR)),len(nbf(OI,OR))))
print('   width-normalised code length: J %#x  O %#x  (%+d)   <- real-code delta with branch widths factored out'%(jn,on,on-jn))
