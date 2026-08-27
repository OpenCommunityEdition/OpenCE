"""Find memory accesses whose operand width differs from the reference.

Detector for the measured data-model rule (docs/house_rules.md §6, "csplit
erases source object boundaries and access width remembers them"): a 32-bit
load where the January target reads 16 bits means a two-byte source subobject
has been flattened, and the remedy is a named subobject of a properly typed
record — never a packing pragma or a pointer cast.  The reverse direction
(target wider than candidate) marks a value the source should pass through a
wider local (the MIN-over-short rule).

Corpus support (research/codegen_dictionary.json, ``word-width-evidence``):
1,696 16-bit compare sites and 1,482 word-extension loads across 619 exact
functions — width is everywhere, and in matched source it always traces to a
declared narrow type.

The two instruction streams are aligned with mnemonic size suffixes,
register widths, and constants erased, so a width mismatch aligns as a
near-equal pair instead of desynchronizing the diff.  Aligned pairs whose
original widths disagree are reported, ranked widest-delta first.

Usage:
    python tools/access_width_scan.py <reference.obj> <candidate.obj> [symbol ...]

With no symbols it scans every .text owner the two objects share.

Validate on a known positive before trusting a zero -- a broken scan and a
clean object look identical.
"""
import difflib
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
import capstone

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
_hex = re.compile(r"0x[0-9a-f]+|\b\d+\b")
_WIDTH_SUFFIX = re.compile(
	r"^(mov|cmp|test|add|sub|and|or|xor|inc|dec|lea|imul|shl|shr|sar|neg|not)"
	r"(zx|sx)?$")
_NARROW_REGISTER = {
	"ax": "eax", "bx": "ebx", "cx": "ecx", "dx": "edx",
	"si": "esi", "di": "edi", "bp": "ebp", "sp": "esp",
	"al": "eax", "bl": "ebx", "cl": "ecx", "dl": "edx",
	"ah": "eax", "bh": "ebx", "ch": "ecx", "dh": "edx",
}
_REGISTER_RE = re.compile(
	r"\b(eax|ebx|ecx|edx|esi|edi|ebp|esp|ax|bx|cx|dx|si|di|bp|sp|"
	r"al|bl|cl|dl|ah|bh|ch|dh)\b")


def _owners(obj):
	"""Map owner symbol name -> section number for every .text COMDAT."""
	preferred = {}
	for sym in obj["symbols"]:
		if sym["section"] <= 0 or sym["value"] != 0:
			continue
		if sym["name"].startswith(("$", ".")):
			continue
		section = obj["sections"][sym["section"] - 1]
		if not section["name"].startswith(".text"):
			continue
		is_external = sym["storage"] == cc.IMAGE_SYM_CLASS_EXTERNAL
		current = preferred.get(sym["section"])
		if current is None or (is_external and not current[1]):
			preferred[sym["section"]] = (sym["name"], is_external)
	owners = {}
	for section_number, (name, _) in preferred.items():
		owners.setdefault(name, section_number)
	return owners


def _section_bytes(obj, index):
	"""Section bytes with relocation slots zeroed on both sides.

	csplit targets store linked addresses where rebuilt candidates store
	addends; leaving them in makes data tails (jump tables) decode
	differently and produce spurious width hits.  Masking both sides
	identically keeps equality meaningful.
	"""
	section = obj["sections"][index - 1]
	raw = bytearray(cc._section_bytes(obj, section))[:section["size"]]
	for entry in range(section["reloc_count"]):
		offset = section["reloc"] + entry * cc.RELOC_ENTRY_SIZE
		address = int.from_bytes(obj["data"][offset:offset + 4], "little")
		if address + 4 <= len(raw):
			raw[address:address + 4] = b"\0\0\0\0"
	return bytes(raw)


def _memory_width(insn):
	"""Return the memory operand width in bytes, or None without one."""
	for operand in insn.operands:
		if operand.type == capstone.x86.X86_OP_MEM:
			return operand.size
	return None


def _width_blind_token(insn):
	"""Token with size suffixes, register widths, and constants erased."""
	mnemonic = insn.mnemonic
	if mnemonic.startswith("j") or mnemonic == "call":
		return mnemonic + " @"
	base = re.sub(r"(movs|movz)x?", "mov", mnemonic)
	operands = _REGISTER_RE.sub(
		lambda match: _NARROW_REGISTER.get(match.group(1), match.group(1)),
		insn.op_str)
	operands = re.sub(r"\b(byte|word|dword) ptr\b", "ptr", operands)
	operands = _hex.sub("#", operands)
	return base + " " + operands


def scan_function(reference, candidate, name, ref_index, cand_index):
	a = list(_md.disasm(_section_bytes(reference, ref_index), 0))
	b = list(_md.disasm(_section_bytes(candidate, cand_index), 0))
	matcher = difflib.SequenceMatcher(
		None, [_width_blind_token(i) for i in a],
		[_width_blind_token(i) for i in b], autojunk=False)
	hits = []
	for op, i1, i2, j1, j2 in matcher.get_opcodes():
		if op != "equal":
			continue
		for k in range(i2 - i1):
			x, y = a[i1 + k], b[j1 + k]
			x_width = _memory_width(x)
			y_width = _memory_width(y)
			if x_width is None or y_width is None or x_width == y_width:
				continue
			hits.append((name, x, y, x_width, y_width))
	return hits


def main(argv):
	if len(argv) < 3:
		print(__doc__)
		return 2
	_md.detail = True
	reference = cc.load(argv[1])
	candidate = cc.load(argv[2])
	wanted = set(argv[3:])

	ref_owners = _owners(reference)
	cand_owners = _owners(candidate)

	scanned = 0
	hits = []
	for name in sorted(set(ref_owners) & set(cand_owners)):
		if wanted and name not in wanted:
			continue
		scanned += 1
		hits.extend(scan_function(reference, candidate, name,
								  ref_owners[name], cand_owners[name]))

	hits.sort(key=lambda item: -abs(item[3] - item[4]))
	for name, x, y, x_width, y_width in hits:
		if x_width < y_width:
			advice = "candidate reads wider than January: a narrow subobject was flattened"
		else:
			advice = "candidate reads narrower than January: value should pass through a wider local"
		print("%-34s reference %05x %-24s (%d bytes)  candidate %05x %-24s (%d bytes)"
			  % (name, x.address, x.mnemonic + " " + x.op_str, x_width,
				 y.address, y.mnemonic + " " + y.op_str, y_width))
		print("    %s" % advice)
	print("scanned %d shared functions, %d aligned width mismatches"
		  % (scanned, len(hits)))
	return 0


if __name__ == "__main__":
	sys.exit(main(sys.argv))
