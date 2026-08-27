# -*- coding: utf-8 -*-
"""Context diff between two emission sites.

When the SAME source spelling produces different bytes at two sites (the
vehicle_stuck / code_0003a910 fold pair), the lever is in the context, not the
spelling.  This tool measures the context of a window in two functions and
reports the deltas: instruction mix before/after, x87 depth at entry, address
materialisations (lea/push of stack slots) preceding the window, and distance
to the nearest call on each side.  It proposes nothing; it narrows where to
look.

Usage:
  python tools/context_diff_oracle.py \
      --a-unit source/units/vehicles --a-function _vehicle_stuck --a-lo 0xe5 --a-hi 0xff \
      --b-unit source/ai/ai_debug --b-function _code_0003a910 --b-lo 0x201 --b-hi 0x21b
"""
import argparse
import os
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
import capstone

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

X87_PUSH = ('fld', 'fild', 'fld1', 'fldz')
X87_POP = ('fstp', 'fistp', 'faddp', 'fsubp', 'fsubrp', 'fmulp', 'fdivp',
           'fdivrp', 'fcompp')


def load_function(unit, function, root='.'):
	path = os.path.join(root, 'build', 'split', unit.replace('/', os.sep) + '.obj')
	coff = cc.load(path)
	symbol = cc.symbol(coff, function)
	section = coff['sections'][symbol['section'] - 1]
	raw = bytes(cc._section_bytes(coff, section)[:section['size']])
	return list(MD.disasm(raw, 0))


def x87_depth_at(instructions, offset):
	depth = 0
	for ins in instructions:
		if ins.address >= offset:
			break
		m = ins.mnemonic
		if m.startswith(X87_PUSH) and not m.startswith('fstp'):
			depth += 1
		if m.startswith(X87_POP) or m == 'fcompp':
			depth -= 1
			if m == 'fcompp':
				depth -= 1
		if m == 'fdivr' or m == 'fdiv' or m == 'fmul' or m == 'fadd' or m == 'fsub':
			pass  # memory-operand forms keep depth
	return depth


def describe(instructions, lo, hi, tag):
	before = [i for i in instructions if lo - 0x30 <= i.address < lo]
	window = [i for i in instructions if lo <= i.address < hi]
	after = [i for i in instructions if hi <= i.address < hi + 0x18]
	report = {'tag': tag}
	report['x87_depth_at_entry'] = x87_depth_at(instructions, lo)
	report['before_mix'] = Counter(i.mnemonic for i in before)
	report['after_mix'] = Counter(i.mnemonic for i in after)
	report['addr_escapes_before'] = [
		'%04x %s %s' % (i.address, i.mnemonic, i.op_str)
		for i in before if i.mnemonic in ('lea', 'push') and 'ebp -' in i.op_str]
	calls_before = [i.address for i in before if i.mnemonic == 'call']
	calls_after = [i.address for i in after if i.mnemonic == 'call']
	report['nearest_call'] = (
		(lo - calls_before[-1]) if calls_before else None,
		(calls_after[0] - hi) if calls_after else None)
	report['window'] = ['%04x %-7s %s' % (i.address, i.mnemonic, i.op_str)
	                    for i in window]
	return report


def main():
	parser = argparse.ArgumentParser()
	for side in 'ab':
		parser.add_argument('--%s-unit' % side, required=True)
		parser.add_argument('--%s-function' % side, required=True)
		parser.add_argument('--%s-lo' % side, required=True)
		parser.add_argument('--%s-hi' % side, required=True)
	parser.add_argument('--root', default='.')
	args = parser.parse_args()

	reports = []
	for side in 'ab':
		unit = getattr(args, '%s_unit' % side)
		function = getattr(args, '%s_function' % side)
		lo = int(getattr(args, '%s_lo' % side), 16)
		hi = int(getattr(args, '%s_hi' % side), 16)
		instructions = load_function(unit, function, args.root)
		reports.append(describe(instructions, lo, hi, '%s:%s' % (unit, function)))

	a, b = reports
	print('=== A', a['tag'])
	print('=== B', b['tag'])
	print('x87 depth at entry: A=%d B=%d' % (
		a['x87_depth_at_entry'], b['x87_depth_at_entry']))
	print('nearest call (before, after): A=%s B=%s' % (
		a['nearest_call'], b['nearest_call']))
	print('address materialisations before window:')
	print('  A:', a['addr_escapes_before'] or 'none')
	print('  B:', b['addr_escapes_before'] or 'none')
	mix_delta = Counter(a['before_mix'])
	mix_delta.subtract(b['before_mix'])
	print('before-mix delta (A minus B):',
	      {k: v for k, v in mix_delta.items() if v} or 'identical')
	mix_delta = Counter(a['after_mix'])
	mix_delta.subtract(b['after_mix'])
	print('after-mix delta (A minus B):',
	      {k: v for k, v in mix_delta.items() if v} or 'identical')


if __name__ == '__main__':
	main()
