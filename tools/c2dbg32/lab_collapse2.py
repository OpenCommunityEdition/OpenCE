"""Round 2: faithful repro of the enter_seat tail, then sweep for the flip.

Faithful shape (matching the real function):
  d = defn_get(...)                      -> callee-saved reg, homed to a slot
  g = graph_get(d->index)                -> first load of d->index
  seat = TAG_BLOCK(g, ...)               -> intervening call
  if (seat->count > K) { ai = table[..]; if (ai != NONE) {
      start_interp(...)                  -> call
      gi = d->index;                     -> THE TIE LOAD (2nd)
      ai = choose(1, gi, ai);            -> call, then d reloaded from slot
      gi = d->index;                     -> 3rd load via reloaded d
      apply(unit, gi, ai); } }
"""
import subprocess, os, re, sys
sys.path.insert(0, 'tools')
import coff_compare as cc
import capstone

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
CL = r'C:\halo-worktrees\claude-untried-objects\xbox\bin\vc7\CL.Exe'

HEAD = r'''
typedef unsigned short word;
typedef char boolean;
struct defn { char pad[0x44]; long index; };
struct seatblk { long count; void *addr; };
struct graph { char pad[0xc]; struct seatblk seats; };
struct entry { short animation_index; short pad; };
struct defn *defn_get(long index);
struct graph *graph_get(long index);
void *block_element(struct seatblk *b, long i, long size);
struct entry *table_get(struct seatblk *b);
void start_interp(long object_index, short frames);
short choose(long render, long graph_index, short animation_index);
void apply(long unit_index, long graph_index, short animation_index);
void tail(long unit_index);
'''

BODY = r'''
void f(long unit_index, long target_index, short seat_index)
{
	struct defn *d;
	struct graph *g;
	struct seatblk *sb;
%DECL%
	d = defn_get(unit_index);
	g = graph_get(d->index);
	sb = (struct seatblk *)block_element(&g->seats, seat_index, 0x64);
	if (sb->count > 7)
	{
		short ai = table_get(sb)[0].animation_index;

		if (ai != -1)
		{
%BODYSTMTS%
		}
	}
	tail(unit_index);
}
'''

V = {}


def mk(decl, stmts):
    return HEAD + BODY.replace('%DECL%', decl).replace('%BODYSTMTS%', stmts)


# v0: current real-source shape (two assignments of gi)
V['v0_two_assign'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v1: gi declared in the inner block (live range starts later)
V['v1_inner_decl'] = mk(
    "",
    "\t\t\tlong gi;\n\n"
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v2: first use direct-expression, second via local
V['v2_first_direct'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tai = choose(1, d->index, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v3: both direct
V['v3_both_direct'] = mk(
    "",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tai = choose(1, d->index, ai);\n"
    "\t\t\tapply(unit_index, d->index, ai);")

# v4: interp call AFTER the first gi load (moves the temp's start earlier)
V['v4_load_before_interp'] = mk(
    "\tlong gi;\n",
    "\t\t\tgi = d->index;\n"
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v5: ai reloaded/rebound right before the call (changes arg temp order)
V['v5_ai_local_first'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\t{\n"
    "\t\t\t\tshort cur = ai;\n\n"
    "\t\t\t\tgi = d->index;\n"
    "\t\t\t\tai = choose(1, gi, cur);\n"
    "\t\t\t}\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v6: single gi assignment feeding both calls
V['v6_single_assign'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v7: d re-derived for the third use (explicit refresh)
V['v7_d_refresh'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\td = defn_get(unit_index);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v8: second use through a separate pointer local bound from d
V['v8_alias_ptr'] = mk(
    "\tlong gi;\n\tstruct defn *d2;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tai = choose(1, gi, ai);\n"
    "\t\t\td2 = d;\n"
    "\t\t\tgi = d2->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v9: gi is short-typed (different node width class)
V['v9_gi_short_first'] = mk(
    "\tlong gi;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tai = choose(1, d->index, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, ai);")

# v10: the choose result feeds a new local (splits ai's range at the call)
V['v10_result_local'] = mk(
    "\tlong gi;\n\tshort chosen;\n",
    "\t\t\tstart_interp(unit_index, 6);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tchosen = choose(1, gi, ai);\n"
    "\t\t\tgi = d->index;\n"
    "\t\t\tapply(unit_index, gi, chosen);")


def emit(name, src):
    cpath = f'scratch/mini/{name}.c'
    opath = f'scratch/mini/{name}.obj'
    open(cpath, 'w', newline='\n').write(src)
    if os.path.exists(opath):
        os.remove(opath)
    r = subprocess.run([CL, '/nologo', '/c', '/O2', '/Oy-', '/DDEBUG', '/Dxbox',
                        '/Fo' + opath, cpath], capture_output=True, text=True)
    if not os.path.exists(opath):
        return None, r.stdout.strip()[-160:]
    o = cc.load(open(opath, 'rb').read())
    secs = o['sections']
    for s in o['symbols']:
        if s['name'] == '_f' and s['section'] > 0 and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text':
            return list(md.disasm(bytes(cc._section_bytes(o, secs[s['section'] - 1])), 0)), None
    return None, 'no _f'


os.makedirs('scratch/mini', exist_ok=True)
for name in sorted(V):
    ins, err = emit(name, V[name])
    if ins is None:
        print(f'{name:22s} FAIL {err}')
        continue
    hits = [f'{i.mnemonic} {i.op_str}' for i in ins
            if i.mnemonic == 'mov' and '+ 0x44]' in i.op_str and not i.op_str.startswith('dword')]
    collapse = [h for h in hits if re.match(r'mov (e[a-z]{2}), dword ptr \[\1 \+ 0x44\]', h)]
    tag = f'COLLAPSE({len(collapse)})' if collapse else 'no-collapse'
    print(f'{name:22s} {tag:14s} {hits}')
