"""Find loops whose induction-variable width differs from the reference.

Detector for the measured loop-index rule (docs/house_rules.md §6): with a
narrow (``short``/``word``) index MSVC keeps a separate ``dec reg / jne``
downcounter alongside the strength-reduced offset; with a wide (``long``)
index it drops the downcounter and compares an offset against an end
constant.  Corpus support (research/codegen_dictionary.json,
``loop-index-width``): 29 exact functions pair a downcounter with a narrow
index against 2 wide counterexamples, so a *downcounter count* difference
between reference and candidate ranks an index-width recovery first.  The
inverse signature (end-compare implies wide) measured non-discriminative —
28 narrow counterexamples — and is deliberately not reported.

Usage:
    python tools/loop_index_width_scan.py <reference.obj> <candidate.obj> [symbol ...]

With no symbols it scans every .text owner the two objects share.

Validate on a known positive before trusting a zero -- a broken scan and a
clean object look identical.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
import capstone

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


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
	addends; masking both sides identically keeps the instruction streams
	comparable and stops data tails from decoding as spurious loops.
	"""
	section = obj["sections"][index - 1]
	raw = bytearray(cc._section_bytes(obj, section))[:section["size"]]
	for entry in range(section["reloc_count"]):
		offset = section["reloc"] + entry * cc.RELOC_ENTRY_SIZE
		address = int.from_bytes(obj["data"][offset:offset + 4], "little")
		if address + 4 <= len(raw):
			raw[address:address + 4] = b"\0\0\0\0"
	return bytes(raw)


def downcounters(code):
	"""Return [(dec_address, register)] for every dec reg / jne-backward pair."""
	found = []
	previous = None
	for insn in _md.disasm(code, 0):
		if previous is not None and insn.mnemonic == "jne" \
				and previous.mnemonic == "dec" and "[" not in previous.op_str:
			try:
				if int(insn.op_str, 16) <= previous.address:
					found.append((previous.address, previous.op_str))
			except ValueError:
				pass
		previous = insn
	return found


def scan_function(reference, candidate, name, ref_index, cand_index):
	ref_loops = downcounters(_section_bytes(reference, ref_index))
	cand_loops = downcounters(_section_bytes(candidate, cand_index))
	if len(ref_loops) == len(cand_loops):
		return None
	return (name, ref_loops, cand_loops)


def main(argv):
	if len(argv) < 3:
		print(__doc__)
		return 2
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
		hit = scan_function(reference, candidate, name,
							ref_owners[name], cand_owners[name])
		if hit:
			hits.append(hit)

	hits.sort(key=lambda item: -abs(len(item[1]) - len(item[2])))
	for name, ref_loops, cand_loops in hits:
		if len(ref_loops) > len(cand_loops):
			advice = "reference keeps a downcounter we lost: suspect an index that should be short/word"
		else:
			advice = "candidate keeps a downcounter the reference lacks: suspect an index that should be long"
		print("%-34s reference %d downcounter(s) %s  candidate %d %s"
			  % (name, len(ref_loops),
				 [hex(address) for address, _ in ref_loops],
				 len(cand_loops),
				 [hex(address) for address, _ in cand_loops]))
		print("    %s" % advice)
	print("scanned %d shared functions, %d downcounter-count differences"
		  % (scanned, len(hits)))
	return 0


if __name__ == "__main__":
	sys.exit(main(sys.argv))
