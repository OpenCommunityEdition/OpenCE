import struct
def parse(path):
    d=open(path,'rb').read()
    nsec=struct.unpack_from('<H',d,2)[0]; ptr=struct.unpack_from('<I',d,8)[0]
    n=struct.unpack_from('<I',d,12)[0]; opt=struct.unpack_from('<H',d,16)[0]
    strtab=ptr+n*18
    syms=[]; i=0
    while i<n:
        off=ptr+i*18; raw=d[off:off+8]
        value,secnum,typ,sclass,naux=struct.unpack_from('<IhHBB',d,off+8)
        if raw[:4]==b'\x00\x00\x00\x00':
            so=struct.unpack_from('<I',raw,4)[0]; end=d.index(b'\x00',strtab+so)
            nm=d[strtab+so:end].decode('ascii','replace')
        else: nm=raw.rstrip(b'\x00').decode('ascii','replace')
        syms.append((nm,secnum,sclass,value))
        for _ in range(naux): syms.append(None)
        i+=1+naux
    base=20+opt; out={}
    for s in range(nsec):
        off=base+s*40
        nm=d[off:off+8].rstrip(b'\x00').decode('ascii','replace')
        if nm.startswith('/'):
            so=int(nm[1:]); end=d.index(b'\x00',strtab+so); nm=d[strtab+so:end].decode()
        vsize,vaddr,rawsize,sptr,preloc,plnum,nreloc,nln,chars=struct.unpack_from('<IIIIIIHHI',d,off+8)
        rels={}
        for r in range(nreloc):
            ro=preloc+r*10
            ra,si,rt=struct.unpack_from('<IIH',d,ro)
            sy=syms[si] if si<len(syms) else None
            rels[ra]=(sy[0] if sy else '?', rt, (sy[3] if sy else 0), (sy[1] if sy else 0))
        out[s+1]=(nm, d[sptr:sptr+rawsize] if sptr else b'', rels)
    owner={}
    allsyms=[x for x in syms if x]
    for x in allsyms:
        nm,sn,cl,v=x
        if sn>0 and v==0 and not nm.startswith('.') and sn not in owner: owner[sn]=nm
    return out, owner, allsyms
