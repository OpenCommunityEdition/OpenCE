#!/usr/bin/env python3
"""Reproduce VC7's branch-shortening fixpoint on one function and compare to actual."""
import sys,os
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import capstone
from _cp import parse
md=capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail=False

def getsec(path, fn):
    secs,owner,syms=parse(path)
    for s,(nm,data,rels) in secs.items():
        if owner.get(s)==fn:
            return data,rels
    raise SystemExit('not found '+fn+' in '+path)

def codelimit(rels, n=None):
    """start of the trailing jump-table block (runs of >=3 dir32 relocs, 4 apart), else n"""
    addrs=sorted(rels)
    if n is None: n=(max(addrs)+4) if addrs else 0
    runs=[];cur=[]
    for a in addrs:
        if cur and a-cur[-1]==4: cur.append(a)
        else:
            if len(cur)>=3: runs.append(cur)
            cur=[a]
    if len(cur)>=3: runs.append(cur)
    for r in runs[::-1]:
        if r[-1]+4>=n-16: return r[0]
    return n
def isbr(b):
    """return (kind, shortsize, nearsize) or None"""
    if len(b)==2 and 0x70<=b[0]<=0x7f: return ('short',2,6)
    if len(b)==2 and b[0]==0xeb:       return ('short',2,5)
    if len(b)==6 and b[0]==0x0f and 0x80<=b[1]<=0x8f: return ('near',2,6)
    if len(b)==5 and b[0]==0xe9:       return ('near',2,5)
    return None

def disasm(data, limit):
    ins=[]
    for i in md.disasm(bytes(data[:limit]),0):
        ins.append(i)
    return ins

def build(ins, rels=None):
    rels=rels or {}
    a2i={i.address:k for k,i in enumerate(ins)}
    items=[]
    for i in ins:
        r=isbr(i.bytes)
        if r:
            try: tgt=int(i.op_str,16)
            except ValueError: tgt=None
            if any((i.address+o) in rels for o in range(1,i.size)): tgt=None
            items.append(dict(br=True,addr=i.address,mn=i.mnemonic,act=r[0],cur='near',
                              short=r[1],near=r[2],tidx=a2i.get(tgt),tgt=tgt))
        else:
            items.append(dict(br=False,addr=i.address,size=len(i.bytes)))
    return items

def layout(items):
    a=0; out=[]
    for it in items:
        out.append(a)
        a += (it['short'] if it['cur']=='short' else it['near']) if it['br'] else it['size']
    out.append(a); return out

def sim(items, maxpass=64):
    passes=0
    for p in range(maxpass):
        passes+=1
        ad=layout(items); changed=False
        for k,it in enumerate(items):
            if not it['br'] or it['cur']=='short' or it['tidx'] is None: continue
            it['cur']='short'; ad2=layout(items)
            d=ad2[it['tidx']]-(ad2[k]+it['short'])
            if -128<=d<=127:
                changed=True; ad=ad2
            else:
                it['cur']='near'
        if not changed: break
    return passes

def report(lbl,path,fn):
    data,rels=getsec(path,fn)
    lim=codelimit(rels,len(data))
    ins=disasm(data,lim)
    items=build(ins,rels)
    nbr=sum(1 for it in items if it['br'])
    nact_short=sum(1 for it in items if it['br'] and it['act']=='short')
    p=sim(items)
    ad=layout(items)
    bad=[(it,k) for k,it in enumerate(items) if it['br'] and it['tidx'] is not None and it['cur']!=it['act']]
    print('%s %s: codelimit=%#x insns=%d branches=%d actual-short=%d  sim passes=%d  simlen=%#x  mismatches=%d'
          %(lbl,fn,lim,len(ins),nbr,nact_short,p,ad[-1],len(bad)))
    for it,k in bad:
        ad2=layout(items)
        # span at ACTUAL layout
        print('    @%-6x %-4s -> %-6x  act=%-5s sim=%-5s  actual-span=%d'
              %(it['addr'],it['mn'],it['tgt'] if it['tgt'] is not None else -1,it['act'],it['cur'],
                (it['tgt']-it['addr']) if it['tgt'] is not None else 0))
    unres=[it for it in items if it['br'] and it['tidx'] is None]
    if unres: print('    (%d branches with unresolved target index)'%len(unres))
    return items,ins

if __name__=='__main__':
    R='C:/halo-worktrees/claude-compiler-application-20260925/'
    fn=sys.argv[1] if len(sys.argv)>1 else '_ai_debug_render_actor'
    unit=sys.argv[2] if len(sys.argv)>2 else 'ai_debug'
    ourobj=sys.argv[3] if len(sys.argv)>3 else R+'scratch/res8/branch-fixpoint/floor.obj'
    report('JAN',R+'build/split/source/ai/%s.obj'%unit,fn)
    report('OUR',ourobj,fn)
