import sys,re,difflib
sys.path.insert(0,'C:/halo-worktrees/claude-compiler-application-20260925/research/compiler_application_20260925/workers/K2/tools')
from sdiff import load
def n0(x):
    a,mn,rest,tgt=x
    if mn.startswith('j') or mn=='call': rest=''
    tgt=re.sub(r'\$L\d+|_ai_debug_render_actor_jmptable','JT',tgt)
    rest=re.sub(r'\[eax\*4( \+ 0x[0-9a-f]+)?\]','[eax*4+JT]',rest) if 'JT' in tgt else rest
    return mn+' '+rest+' '+tgt
J=[x for x in load(sys.argv[1]) if x[0]<0x5ffd]
res=[]
for p in sys.argv[2:]:
    O=load(p)
    # cut ours at its real code end: last ret
    lastret=max(i for i,x in enumerate(O) if x[1]=='ret')
    O=O[:lastret+1]
    J2=J[:max(i for i,x in enumerate(J) if x[1]=='ret')+1]
    a=[n0(x) for x in J2]; b=[n0(x) for x in O]
    sm=difflib.SequenceMatcher(None,a,b,autojunk=False)
    eq=sum(i2-i1 for op,i1,i2,j1,j2 in sm.get_opcodes() if op=='equal')
    print('%-40s J %d O %d equal %d  diffJ %d diffO %d'%(p.split('/')[-1],len(a),len(b),eq,len(a)-eq,len(b)-eq))
