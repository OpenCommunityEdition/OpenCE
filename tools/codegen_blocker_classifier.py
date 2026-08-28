"""Classify strict COFF codegen differences without granting match credit.

The classifier is deliberately fail-closed.  It maps a small set of measured
MSVC 7.0 code-generation signatures to the production recipes documented by
the XDK 3911 codegen atlas.  Anything that does not satisfy a complete rule is
``UNKNOWN``.  A result is diagnostic evidence only: this tool never changes a
matching ledger or decides that a function is exact.
"""

import argparse
import itertools
import json
import re
import shutil
import subprocess
from pathlib import Path

try:
	from .coff_compare import (
		CoffError,
		load,
		relocation_infos_equal,
		section_info,
		section_infos_equal,
	)
	from . import coff_compare as _coff_compare
except ImportError:
	# Script invocation: python tools/codegen_blocker_classifier.py
	import os
	import sys
	sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
	from coff_compare import (
		CoffError,
		load,
		relocation_infos_equal,
		section_info,
		section_infos_equal,
	)
	import coff_compare as _coff_compare


VERSION = 1
GENERAL_REGISTERS = ("eax", "ebx", "ecx", "edx", "esi", "edi")
REGISTER_ALIASES = {
	"eax": ("eax", "ax", "al", "ah"),
	"ebx": ("ebx", "bx", "bl", "bh"),
	"ecx": ("ecx", "cx", "cl", "ch"),
	"edx": ("edx", "dx", "dl", "dh"),
	"esi": ("esi", "si"),
	"edi": ("edi", "di"),
	"ebp": ("ebp", "bp"),
	"esp": ("esp", "sp"),
}
ALIAS_TO_REGISTER = {
	alias: register
	for register, aliases in REGISTER_ALIASES.items()
	for alias in aliases
}
INSTRUCTION_RE = re.compile(
	r"^\s*([0-9a-fA-F]+):\s+((?:[0-9a-fA-F]{2}\s+)+)"
	r"([A-Za-z][A-Za-z0-9_.]*)\s*(.*?)\s*$")
REGISTER_RE = re.compile(
	r"%(eax|ax|al|ah|ebx|bx|bl|bh|ecx|cx|cl|ch|edx|dx|dl|dh|"
	r"esi|si|edi|di|ebp|bp|esp|sp)\b")


class ClassifierError(RuntimeError):
	pass


def parse_disassembly(text):
	"""Parse llvm-objdump AT&T instruction lines into stable evidence."""
	instructions = []
	for line in text.splitlines():
		match = INSTRUCTION_RE.match(line)
		if not match:
			continue
		instructions.append({
			"address": int(match.group(1), 16),
			"bytes": "".join(match.group(2).split()).lower(),
			"mnemonic": match.group(3).lower(),
			"operands": _normalize_operands(match.group(4)),
		})
	return instructions


def _normalize_operands(operands):
	return re.sub(r"\s+", " ", operands.strip().lower())


def _instruction_key(instruction):
	return instruction["mnemonic"], _normalize_operands(instruction["operands"])


def _split_operands(operands):
	"""Split AT&T operands without treating SIB commas as separators."""
	parts = []
	start = 0
	depth = 0
	for index, character in enumerate(operands):
		if character == "(":
			depth += 1
		elif character == ")":
			depth = max(0, depth - 1)
		elif character == "," and depth == 0:
			parts.append(operands[start:index].strip())
			start = index + 1
	parts.append(operands[start:].strip())
	return parts


def _relocation_identities_equal(left, right):
	"""Compare ordered relocation semantics while allowing address drift."""
	if len(left) != len(right):
		return False
	left_without_addresses = [dict(relocation, address=0) for relocation in left]
	right_without_addresses = [dict(relocation, address=0) for relocation in right]
	return relocation_infos_equal(left_without_addresses, right_without_addresses)


def _metadata(target, candidate):
	target_relocations = target.get("relocations", [])
	candidate_relocations = candidate.get("relocations", [])
	return {
		"size_equal": target.get("size") == candidate.get("size"),
		"relocation_count_equal": (
			target.get("relocation_count") == candidate.get("relocation_count")),
		"relocation_identities_equal": _relocation_identities_equal(
			target_relocations, candidate_relocations),
		"relocations_exact": relocation_infos_equal(
			target_relocations, candidate_relocations),
	}


def _result(kind, blocker_class, recipe, confidence, evidence, metadata,
			stop_rule, preconditions=None):
	return {
		"version": VERSION,
		"kind": kind,
		"class": blocker_class,
		"recipe": recipe,
		"confidence": confidence,
		"evidence": evidence,
		"strict_metadata": metadata,
		"preconditions": preconditions or [],
		"stop_rule": stop_rule,
		"grants_match_credit": False,
	}


def classify(target, candidate):
	"""Return one conservative codegen classification for two evidence dicts."""
	metadata = _metadata(target, candidate)
	target_instructions = target.get("instructions", [])
	candidate_instructions = candidate.get("instructions", [])
	if not target_instructions or not candidate_instructions:
		return _unknown(metadata, "both disassemblies are required")

	if target.get("normalized_sha256") == candidate.get("normalized_sha256") \
			and metadata["size_equal"] and metadata["relocations_exact"]:
		return _result(
			"REFUSE", "no-codegen-blocker", None, "PROVEN",
			["strict measurements are already equal"], metadata,
			"Use the ordinary strict comparator; this classifier does not grant credit.")

	classification = _classify_return_shape(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	classification = _classify_private_abi(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	classification = _classify_alias_shape(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	classification = _classify_x87_liveness(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	classification = _classify_register_permutation(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	classification = _classify_scheduling_tie(
		target_instructions, candidate_instructions, metadata)
	if classification:
		return classification
	return _unknown(metadata, "no proven recipe or strict STOP signature matched")


def _unknown(metadata, reason):
	return _result(
		"UNKNOWN", "unclassified", None, "NONE", [reason], metadata,
		"Gather stronger disassembly/source evidence; do not infer a recipe or park class.")


def _ret_count(instructions):
	return sum(instruction["mnemonic"].startswith("ret") for instruction in instructions)


def _classify_return_shape(target, candidate, metadata):
	target_count = _ret_count(target)
	candidate_count = _ret_count(candidate)
	if target_count == candidate_count or not metadata["relocation_identities_equal"]:
		return None
	return _result(
		"RECIPE", "control-flow-return-shape", "R5", "PROVEN", [
			f"target ret count={target_count}; candidate ret count={candidate_count}",
			"ordered relocation identities agree",
		], metadata,
		"Stop after the return count matches; any residual needs a fresh classification.", [
			"Preserve the target's early-return versus single-exit structure.",
		])


def _has_frame(instructions):
	keys = [_instruction_key(instruction) for instruction in instructions[:8]]
	push_ebp = any(mnemonic.startswith("push") and operands == "%ebp"
					for mnemonic, operands in keys)
	mov_frame = any(mnemonic.startswith("mov") and operands == "%esp, %ebp"
					for mnemonic, operands in keys)
	sub_stack = any(mnemonic.startswith("sub") and operands.endswith(", %esp")
					for mnemonic, operands in keys)
	return push_ebp and mov_frame and sub_stack


def _classify_private_abi(target, candidate, metadata):
	if _has_frame(target) or not _has_frame(candidate):
		return None
	target_head = " ".join(instruction["operands"] for instruction in target[:12])
	candidate_head = " ".join(instruction["operands"] for instruction in candidate[:12])
	if not ("%ecx" in target_head or "%edx" in target_head):
		return None
	if not re.search(r"0x(?:8|c)\(%ebp\)", candidate_head):
		return None
	if not metadata["relocation_identities_equal"]:
		return None
	return _result(
		"RECIPE", "private-abi-frame-convention", "R1", "STRONG", [
			"target is frameless and consumes ECX/EDX near entry",
			"candidate builds an EBP frame and reads stack arguments",
			"ordered relocation identities agree",
		], metadata,
		"Stop if the helper still requires a stack frame after shortening live ranges.", [
			"Confirm the source helper is static.",
			"Confirm its address is never taken and it has a same-TU caller.",
		])


def _has_sib_scale_eight(instructions):
	for instruction in instructions:
		if not instruction["mnemonic"].startswith("mov"):
			continue
		operands = _split_operands(instruction["operands"])
		if operands and re.search(r"\([^)]*,[^)]*,8\)", operands[-1]):
			return True
	return False


def _has_scaled_temporary(instructions):
	return any(
		(instruction["mnemonic"].startswith("lea")
		 and re.search(r"\([^)]*,[^)]*,8\)", instruction["operands"]))
		or (instruction["mnemonic"].startswith("shl")
			and re.search(r"\$(?:0x)?3,", instruction["operands"]))
		for instruction in instructions)


def _classify_alias_shape(target, candidate, metadata):
	if not _has_sib_scale_eight(target) or not _has_scaled_temporary(candidate):
		return None
	return _result(
		"RECIPE", "alias-global-ownership", "R3", "PROVEN", [
			"target contains direct scale-8 SIB stores",
			"candidate materializes a scale-8 address with LEA/SHL",
		], metadata,
		"Stop and classify as ownership-blocked if target relocations use one aggregate anchor.", [
			"Audit target COFF ownership before splitting an aggregate global.",
			"After any source change, require strict relocation semantics as well as code bytes.",
		])


def _fst_kind(mnemonic):
	if mnemonic.startswith("fstp"):
		return "fstp", mnemonic[4:]
	if mnemonic.startswith("fst"):
		return "fst", mnemonic[3:]
	return None


def _classify_x87_liveness(target, candidate, metadata):
	if len(target) != len(candidate) or not metadata["relocation_identities_equal"]:
		return None
	differences = []
	for index, (target_instruction, candidate_instruction) in enumerate(zip(target, candidate)):
		if _instruction_key(target_instruction) == _instruction_key(candidate_instruction):
			continue
		target_kind = _fst_kind(target_instruction["mnemonic"])
		candidate_kind = _fst_kind(candidate_instruction["mnemonic"])
		if (not target_kind or not candidate_kind
				or target_kind[0] == candidate_kind[0]
				or target_kind[1] != candidate_kind[1]
				or target_instruction["operands"] != candidate_instruction["operands"]):
			return None
		differences.append((index, target_kind[0], candidate_kind[0]))
	if not differences:
		return None
	return _result(
		"RECIPE", "x87-value-liveness", "R4", "PROVEN", [
			"all instruction differences are FST/FSTP opcode pairs",
			"ordered relocation identities agree",
			*[f"instruction {index}: target {left}, candidate {right}"
			  for index, left, right in differences],
		], metadata,
		"Stop after x87 stack depth and subsequent instruction addresses realign.", [
			"Target FST requires keeping the computed value live after the store.",
			"Target FSTP requires letting the computed value die at the store.",
		])


def _replace_registers(operands, mapping):
	def replace(match):
		alias = match.group(1)
		register = ALIAS_TO_REGISTER[alias]
		mapped = mapping.get(register, register)
		aliases = REGISTER_ALIASES[mapped]
		original_aliases = REGISTER_ALIASES[register]
		try:
			index = original_aliases.index(alias)
		except ValueError:
			return match.group(0)
		if index >= len(aliases):
			return match.group(0)
		return "%" + aliases[index]
	return REGISTER_RE.sub(replace, operands)


def _classify_register_permutation(target, candidate, metadata):
	if (not metadata["size_equal"] or not metadata["relocations_exact"]
			or len(target) != len(candidate)):
		return None
	if [item["mnemonic"] for item in target] != [item["mnemonic"] for item in candidate]:
		return None
	target_keys = [_instruction_key(instruction) for instruction in target]
	candidate_keys = [_instruction_key(instruction) for instruction in candidate]
	differing = [
		index for index, pair in enumerate(zip(target_keys, candidate_keys))
		if pair[0] != pair[1]
	]
	if not differing:
		return None
	# A register live range can begin and end inside a function.  Permit one
	# contiguous local window, but refuse disconnected differences because a
	# single causal permutation is then unproven.
	if differing != list(range(differing[0], differing[-1] + 1)):
		return None
	window = range(differing[0], differing[-1] + 1)
	for permutation in itertools.permutations(GENERAL_REGISTERS):
		mapping = dict(zip(GENERAL_REGISTERS, permutation))
		if all(mapping[register] == register for register in GENERAL_REGISTERS):
			continue
		if all(
			_replace_registers(candidate_instruction["operands"], mapping)
			== target_instruction["operands"]
			for target_instruction, candidate_instruction in
			((target[index], candidate[index]) for index in window)
		):
			changed = [
				f"{source}->{destination}"
				for source, destination in mapping.items()
				if source != destination
			]
			return _result(
				"STOP", "pure-register-permutation", None, "PROVEN", [
					"equal padded size and exact relocation semantics",
					f"one contiguous instruction window {differing[0]}..{differing[-1]} "
					"becomes identical under one bijective GPR mapping",
					"mapping: " + ", ".join(changed),
				], metadata,
				"No measured legal-C control moved a pure register permutation; keep parked.")
	return None


def _registers(instruction):
	return {
		ALIAS_TO_REGISTER[match.group(1)]
		for match in REGISTER_RE.finditer(instruction["operands"])
	}


def _memory_role(instruction):
	operands = instruction["operands"]
	if "(" not in operands:
		return "none"
	mnemonic = instruction["mnemonic"]
	if mnemonic.startswith("lea"):
		return "none"
	parts = _split_operands(operands)
	if mnemonic.startswith("mov") and parts and "(" in parts[-1]:
		return "write"
	if mnemonic.startswith("fst"):
		return "write"
	return "read"


def _flag_role(mnemonic):
	if mnemonic.startswith(("j", "set", "cmov", "adc", "sbb")):
		return "read"
	if mnemonic.startswith((
		"add", "sub", "cmp", "test", "and", "or", "xor", "inc", "dec",
		"shl", "shr", "sar", "sal", "neg", "imul")):
		return "write"
	return "none"


def _independent(left, right):
	if left["mnemonic"].startswith(("j", "call", "ret", "loop", "int", "push", "pop")):
		return False
	if right["mnemonic"].startswith(("j", "call", "ret", "loop", "int", "push", "pop")):
		return False
	if left["mnemonic"].startswith("f") and right["mnemonic"].startswith("f"):
		return False
	if _registers(left) & _registers(right):
		return False
	left_memory = _memory_role(left)
	right_memory = _memory_role(right)
	if "write" in (left_memory, right_memory) and "none" not in (left_memory, right_memory):
		return False
	left_flags = _flag_role(left["mnemonic"])
	right_flags = _flag_role(right["mnemonic"])
	if left_flags == "read" or right_flags == "read":
		return False
	if left_flags == right_flags == "write":
		return False
	return True


def _classify_scheduling_tie(target, candidate, metadata):
	if (not metadata["size_equal"] or not metadata["relocations_exact"]
			or len(target) != len(candidate)):
		return None
	target_keys = [_instruction_key(instruction) for instruction in target]
	candidate_keys = [_instruction_key(instruction) for instruction in candidate]
	differing = [
		index for index, pair in enumerate(zip(target_keys, candidate_keys))
		if pair[0] != pair[1]
	]
	if len(differing) != 2 or differing[1] != differing[0] + 1:
		return None
	first, second = differing
	if not (target_keys[first] == candidate_keys[second]
			and target_keys[second] == candidate_keys[first]):
		return None
	if not _independent(target[first], target[second]):
		return None
	return _result(
		"STOP", "independent-instruction-scheduling", None, "PROVEN", [
			"equal padded size and exact relocation semantics",
			f"instructions {first} and {second} are one adjacent transposition",
			"the pair has disjoint registers, no control transfer, and no memory/flag dependency",
		], metadata,
		"No measured legal-C control moved an independent scheduling tie; keep parked.")


_DECIMAL_IMMEDIATE_RE = re.compile(r"\$(-?)(\d+)\b")
_DECIMAL_DISPLACEMENT_RE = re.compile(r"(^|[\s,])(-?)(\d+)\(")
_PAREN_SPACE_RE = re.compile(r"\(([^)]*)\)")


def _llvm_style_operands(operands):
	"""Rewrite capstone AT&T operands into llvm-objdump conventions.

	The measured rule signatures were written against llvm-objdump output:
	hex immediates and displacements (``$0x10``, ``0x8(%ebp)``) and no spaces
	inside SIB expressions (``(%eax,%ebx,8)``).  capstone prints decimal and
	spaced forms.  Only the spelling is changed; SIB scales stay decimal in
	both producers.
	"""
	operands = _PAREN_SPACE_RE.sub(
		lambda match: "(" + match.group(1).replace(", ", ",") + ")", operands)
	operands = _DECIMAL_IMMEDIATE_RE.sub(
		lambda match: "$" + match.group(1) + hex(int(match.group(2))), operands)
	operands = _DECIMAL_DISPLACEMENT_RE.sub(
		lambda match: match.group(1) + match.group(2)
		+ hex(int(match.group(3))) + "(", operands)
	return operands


def _capstone_instructions(obj, function_name):
	"""Disassemble one function COMDAT with capstone, relocation slots masked.

	Masking the four addend bytes at every relocation address before decoding
	keeps csplit targets (which store linked addresses) and rebuilt candidates
	(which store link-time addends) from producing spurious operand
	differences.  Both sides are masked identically, so equality remains
	meaningful; the relocation identities themselves are still compared by the
	strict metadata.
	"""
	try:
		import capstone
	except ImportError as error:
		raise ClassifierError(
			"neither llvm-objdump nor capstone is available") from error
	fn = None
	for item in obj["symbols"]:
		if item["name"] == function_name and item["section"] > 0:
			fn = item
			break
	if fn is None:
		raise ClassifierError(
			f"section-relative symbol {function_name!r} not found")
	section = obj["sections"][fn["section"] - 1]
	raw = bytearray(_coff_compare._section_bytes(obj, section))[:section["size"]]
	for index in range(section["reloc_count"]):
		offset = section["reloc"] + index * 10
		address = int.from_bytes(obj["data"][offset:offset + 4], "little")
		if address + 4 <= len(raw):
			raw[address:address + 4] = b"\0\0\0\0"
	engine = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
	engine.syntax = capstone.CS_OPT_SYNTAX_ATT
	engine.skipdata = True
	instructions = []
	for decoded in engine.disasm(bytes(raw), 0):
		instructions.append({
			"address": decoded.address,
			"bytes": decoded.bytes.hex(),
			"mnemonic": decoded.mnemonic.lower(),
			"operands": _normalize_operands(
				_llvm_style_operands(decoded.op_str)),
		})
	if not instructions:
		raise ClassifierError(f"no instructions found for {function_name!r}")
	return instructions


def _run_objdump(objdump, object_path, function_name):
	command = [
		str(objdump), "-d", "-r", "--symbolize-operands",
		f"--disassemble-symbols={function_name}", str(object_path),
	]
	completed = subprocess.run(
		command, capture_output=True, text=True, check=False)
	if completed.returncode:
		raise ClassifierError(completed.stderr.strip() or "llvm-objdump failed")
	instructions = parse_disassembly(completed.stdout)
	if not instructions:
		raise ClassifierError(f"no instructions found for {function_name!r}")
	return instructions


def evidence_from_object(object_path, function_name, objdump=None, backend="auto"):
	"""Measure one COFF function and attach disassembly evidence.

	``backend`` selects the disassembler: ``"objdump"`` requires llvm-objdump,
	``"capstone"`` uses the in-process capstone port, and ``"auto"`` prefers
	llvm-objdump when present and falls back to capstone.  Both producers of a
	comparison must use the same backend; mixing them is not meaningful.
	"""
	object_path = Path(object_path)
	try:
		obj = load(object_path)
		measurement = section_info(obj, function_name)
	except (CoffError, OSError) as error:
		raise ClassifierError(str(error)) from error
	if backend != "capstone":
		objdump = objdump or shutil.which("llvm-objdump") or shutil.which("llvm-objdump.exe")
	if backend == "objdump" and not objdump:
		raise ClassifierError("llvm-objdump was not found; pass --objdump")
	if backend != "capstone" and objdump:
		measurement["instructions"] = _run_objdump(objdump, object_path, function_name)
	else:
		measurement["instructions"] = _capstone_instructions(obj, function_name)
	return measurement


def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("target")
	parser.add_argument("candidate")
	parser.add_argument("function")
	parser.add_argument("--objdump")
	parser.add_argument(
		"--backend", choices=("auto", "objdump", "capstone"), default="auto")
	parser.add_argument("--output", type=Path)
	args = parser.parse_args(argv)
	try:
		target = evidence_from_object(
			args.target, args.function, objdump=args.objdump,
			backend=args.backend)
		candidate = evidence_from_object(
			args.candidate, args.function, objdump=args.objdump,
			backend=args.backend)
		result = classify(target, candidate)
	except ClassifierError as error:
		parser.error(str(error))
	serialized = json.dumps(result, indent=2) + "\n"
	if args.output:
		args.output.parent.mkdir(parents=True, exist_ok=True)
		args.output.write_text(serialized, encoding="utf-8")
	print(serialized, end="")
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
