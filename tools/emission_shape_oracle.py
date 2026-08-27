# -*- coding: utf-8 -*-
"""Corpus retrieval by emission shape.

Every function listed as exact by tools/residual_frontier.py is a proven
(C source, VC7 bytes) pair.  When a residual function differs from January by
one construct, the fastest route to the source spelling is a worked example:
an exact function whose bytes contain the same instruction shape, whose source
can simply be read.  This tool retrieves those examples.

The query is a window of TARGET disassembly (unit, function, byte range).
Instructions are normalised to shape tokens:

- mnemonic kept exact;
- x87 stack operands (st(N)) kept exact -- stack choreography is the signal;
- small immediates (abs < 0x1000) kept exact, large ones wildcarded;
- memory operands classified: [reg+disp] -> m, [disp32 absolute] -> M,
  [esp]-relative -> s, sib -> x;
- general registers kept exact in strict mode, wildcarded to r in loose mode.

Matches are reported as (unit, function, offset) so the proven source can be
read directly.  The tool grants no credit and edits nothing.

Usage:
    python tools/emission_shape_oracle.py --query-unit source/ai/ai_debug \
        --query-function _code_00039990 --lo 0x223 --hi 0x235 [--strict]
        [--frontier research/residual_frontier.json] [--limit 20]
"""
import argparse
import json
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc

try:
    import capstone
except ImportError:  # pragma: no cover
    capstone = None

MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32) if capstone else None

GP32 = ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp')


def normalise_operand(op, strict):
	op = op.strip()
	if op.startswith('st'):
		return op
	m = re.match(r'^(byte|word|dword|qword) ptr \[(.+)\]$', op)
	if m:
		width, inner = m.groups()
		if 'esp' in inner:
			kind = 's'
		elif re.match(r'^0x[0-9a-f]+$|^\d+$', inner):
			kind = 'M'
		elif '*' in inner:
			kind = 'x'
		else:
			kind = 'm'
		return '%s.%s' % (width[0], kind)
	if op in GP32 or op in ('ax', 'bx', 'cx', 'dx', 'si', 'di',
	                        'al', 'bl', 'cl', 'dl', 'ah', 'bh', 'ch', 'dh'):
		return op if strict else 'r'
	m = re.match(r'^(-?)(0x[0-9a-f]+|\d+)$', op)
	if m:
		value = int(m.group(2), 16 if m.group(2).startswith('0x') else 10)
		if m.group(1):
			value = -value
		return str(value) if abs(value) < 0x1000 else 'I'
	return '?'


def tokenise(raw, strict):
	tokens = []
	offsets = []
	for ins in MD.disasm(raw, 0):
		ops = [normalise_operand(o, strict) for o in ins.op_str.split(',')] \
			if ins.op_str else []
		tokens.append(ins.mnemonic + ' ' + ','.join(ops))
		offsets.append(ins.address)
	return tokens, offsets


def exact_inventory(frontier_path, root):
	frontier = json.loads(open(frontier_path, encoding='utf-8').read())
	for unit in frontier['units']:
		if not unit['exact']:
			continue
		path = os.path.join(root, 'build', 'split',
		                    unit['unit'].replace('/', os.sep) + '.obj')
		try:
			coff = cc.load(path)
		except Exception:
			continue
		section_of = {}
		for symbol in coff['symbols']:
			if symbol['value'] == 0 and symbol['section'] > 0:
				try:
					section = coff['sections'][symbol['section'] - 1]
				except Exception:
					continue
				if section['name'].startswith('.text'):
					section_of.setdefault(symbol['name'], section)
		for entry in unit['exact']:
			section = section_of.get(entry['function'])
			if section is None:
				continue
			raw = bytes(cc._section_bytes(coff, section)[:section['size']])
			yield unit['unit'], entry['function'], raw


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('--query-unit', required=True)
	parser.add_argument('--query-function', required=True)
	parser.add_argument('--lo', required=True)
	parser.add_argument('--hi', required=True)
	parser.add_argument('--strict', action='store_true')
	parser.add_argument('--frontier', default='research/residual_frontier.json')
	parser.add_argument('--root', default='.')
	parser.add_argument('--limit', type=int, default=20)
	args = parser.parse_args()

	lo, hi = int(args.lo, 16), int(args.hi, 16)
	path = os.path.join(args.root, 'build', 'split',
	                    args.query_unit.replace('/', os.sep) + '.obj')
	coff = cc.load(path)
	symbol = cc.symbol(coff, args.query_function)
	section = coff['sections'][symbol['section'] - 1]
	raw = bytes(cc._section_bytes(coff, section)[:section['size']])
	tokens, offsets = tokenise(raw, args.strict)
	needle = [t for t, o in zip(tokens, offsets) if lo <= o < hi]
	if not needle:
		raise SystemExit('empty query window')
	print('query (%d tokens):' % len(needle))
	for token in needle:
		print('    ' + token)

	hits = 0
	scanned = 0
	for unit, function, body in exact_inventory(args.frontier, args.root):
		scanned += 1
		hay, hay_offsets = tokenise(body, args.strict)
		n = len(needle)
		for i in range(len(hay) - n + 1):
			if hay[i:i + n] == needle:
				print('HIT  %-44s %-38s at 0x%x' % (unit, function, hay_offsets[i]))
				hits += 1
				break
		if hits >= args.limit:
			break
	print('%d hits across %d exact functions scanned' % (hits, scanned))


if __name__ == '__main__':
	main()
