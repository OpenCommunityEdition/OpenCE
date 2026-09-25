"""Read-only hardware watchpoints for the computed-left frame record."""
import json
import sys
import frame_trace as F

def run(label):
    source=str(F.OUT/(label+'.c'))
    work=F.OUT/(label+'_watch')
    work.mkdir(exist_ok=True)
    k,n=F.T.locate(source,F.UNIT,F.FN,str(work))
    F.T.CHAINS='4 24;5 24;0 24;3 24'
    events,obj=F.T.run(source,str(work),F.UNIT,gate=(k-1,k),
        raw_bps=[0x10715dc7,0x10814497],
        dwp=[(1,36,4,0x38,4,1),(1,36,4,0x24,4,1),(1,36,4,0x04,4,1),(1,36,5,0x18,4,1)],
        extra_cflags=['/FAsc','/Fa'+str(work/'listing.asm')])
    check=F.stock_check(work)
    lines=(work/'dbg_result.txt').read_text().splitlines()
    watches=[s for s in lines if s.startswith(('WP','DWP'))]
    (work/'watch.json').write_text(json.dumps(dict(label=label,events=watches,stock_check=check),indent=2))
    print(label,*watches,sep='\n')

if __name__=='__main__':
    F.setup()
    for label in sys.argv[1:] or ['scalar_left','aggregate_left']:
        run(label)
