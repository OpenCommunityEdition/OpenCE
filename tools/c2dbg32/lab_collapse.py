"""Minimal-repro lab for the enter_seat base-collapse tie.

Target shape (January):   mov ebx, [ebx+0x44]   (load collapses into the dying
                                                 base register)
Our shape:                mov eax, [ebx+0x44]

Sweeps source variants of the surrounding statement structure and reports which
register the load takes, so the source property that drives the collapse can be
identified without exhaustive guessing.
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
struct datum { char pad[0x100]; short seat_index; };
struct defn *get_defn(long index);
struct datum *unit_get(long index);
void start_interp(long object_index, short frames);
short choose(long render, long graph_index, short animation_index);
void apply(long unit_index, long graph_index, short animation_index);
void tail(long unit_index);
'''

# v0 = current source shape: two separate assignments of the graph index
V = {}

V['v0_two_assign'] = HEAD + r'''
void f(long unit_index, short animation_index)
{
	struct defn *d;
	long gi;

	d = get_defn(unit_index);
	if (animation_index != -1)
	{
		start_interp(unit_index, 6);
		gi = d->index;
		animation_index = choose(1, gi, animation_index);
		gi = d->index;
		apply(unit_index, gi, animation_index);
	}
	tail(unit_index);
}
'''

V['v1_direct_first'] = V['v0_two_assign'].replace(
    "\t\tgi = d->index;\n\t\tanimation_index = choose(1, gi, animation_index);",
    "\t\tanimation_index = choose(1, d->index, animation_index);")

V['v2_direct_both'] = V['v0_two_assign'].replace(
    "\t\tgi = d->index;\n\t\tanimation_index = choose(1, gi, animation_index);\n\t\tgi = d->index;\n\t\tapply(unit_index, gi, animation_index);",
    "\t\tanimation_index = choose(1, d->index, animation_index);\n\t\tapply(unit_index, d->index, animation_index);")

V['v3_single_assign'] = V['v0_two_assign'].replace(
    "\t\tgi = d->index;\n\t\tanimation_index = choose(1, gi, animation_index);\n\t\tgi = d->index;\n\t\tapply(unit_index, gi, animation_index);",
    "\t\tgi = d->index;\n\t\tanimation_index = choose(1, gi, animation_index);\n\t\tapply(unit_index, gi, animation_index);")

# the interesting axis per the reachability doc: live-range START order.
# introduce a second long whose range starts before/after gi.
V['v4_extra_before'] = V['v0_two_assign'].replace(
    "\tstruct defn *d;\n\tlong gi;",
    "\tstruct defn *d;\n\tlong seat;\n\tlong gi;").replace(
    "\t\tstart_interp(unit_index, 6);",
    "\t\tseat = unit_get(unit_index)->seat_index;\n\t\tstart_interp(unit_index, (short)seat);")

V['v5_defn_reload'] = V['v0_two_assign'].replace(
    "\t\tgi = d->index;\n\t\tapply(unit_index, gi, animation_index);",
    "\t\td = get_defn(unit_index);\n\t\tgi = d->index;\n\t\tapply(unit_index, gi, animation_index);")

# d used AFTER the apply call -> keeps d live past the tie (should force non-collapse)
V['v6_d_live_after'] = V['v0_two_assign'].replace(
    "\t\tapply(unit_index, gi, animation_index);",
    "\t\tapply(unit_index, gi, animation_index);\n\t\ttail(d->index);")

# d's LAST use is the tie load itself, and gi feeds both calls (no reload)
V['v7_last_use_is_tie'] = HEAD + r'''
void f(long unit_index, short animation_index)
{
	struct defn *d;
	long gi;

	d = get_defn(unit_index);
	if (animation_index != -1)
	{
		start_interp(unit_index, 6);
		gi = d->index;
		animation_index = choose(1, gi, animation_index);
		apply(unit_index, gi, animation_index);
	}
	tail(unit_index);
}
'''

# structure with the graph pointer ALSO derived from d (as in the real function,
# where animation_graph = animation_graph_definition_get(d->index) happens first)
V['v8_graph_first'] = HEAD + r'''
struct graph { char pad[0x10]; long count; };
struct graph *graph_get(long index);
void f(long unit_index, short animation_index)
{
	struct defn *d;
	struct graph *g;
	long gi;

	d = get_defn(unit_index);
	g = graph_get(d->index);
	if (g->count > 0 && animation_index != -1)
	{
		start_interp(unit_index, 6);
		gi = d->index;
		animation_index = choose(1, gi, animation_index);
		gi = d->index;
		apply(unit_index, gi, animation_index);
	}
	tail(unit_index);
}
'''


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
            data = cc._section_bytes(o, secs[s['section'] - 1])
            return list(md.disasm(bytes(data), 0)), None
    return None, 'no _f'


os.makedirs('scratch/mini', exist_ok=True)
for name in sorted(V):
    ins, err = emit(name, V[name])
    if ins is None:
        print(f'{name:22s} FAIL {err}')
        continue
    # find loads of the +0x44 field
    hits = [f'{i.mnemonic} {i.op_str}' for i in ins
            if i.mnemonic == 'mov' and '+ 0x44]' in i.op_str and not i.op_str.startswith('dword')]
    collapse = any(re.match(r'mov (e[a-z]{2}), dword ptr \[\1 \+ 0x44\]', h) for h in hits)
    print(f'{name:22s} {"COLLAPSE" if collapse else "no-collapse"}  {hits}')
