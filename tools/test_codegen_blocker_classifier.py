import hashlib
import unittest

from tools.codegen_blocker_classifier import classify, parse_disassembly


def instruction(mnemonic, operands="", address=0):
	return {
		"address": address,
		"bytes": "90",
		"mnemonic": mnemonic,
		"operands": operands,
	}


def relocation(address=4, target="_callee"):
	return {"address": address, "type": 20, "target": ["external", target, 0]}


def evidence(instructions, size=32, relocations=None, sha=None):
	relocations = list(relocations or [])
	if sha is None:
		sha = hashlib.sha256(repr(instructions).encode("utf-8")).hexdigest()
	return {
		"size": size,
		"relocation_count": len(relocations),
		"normalized_sha256": sha,
		"relocations": relocations,
		"instructions": instructions,
	}


class CodegenBlockerClassifierTests(unittest.TestCase):
	def test_parse_llvm_objdump_instruction_and_ignore_relocation(self):
		parsed = parse_disassembly("""
00000000 <_f>:
       0: 55                            pushl   %ebp
       1: e8 00 00 00 00                calll   0x6 <_f+0x6>
            00000002: IMAGE_REL_I386_REL32 _callee
""")
		self.assertEqual([item["mnemonic"] for item in parsed], ["pushl", "calll"])
		self.assertEqual(parsed[1]["operands"], "0x6 <_f+0x6>")

	def test_r1_private_abi_frame_signature(self):
		target = evidence([
			instruction("pushl", "%esi"),
			instruction("movl", "%ecx, %esi"),
			instruction("addl", "%edx, %esi"),
			instruction("retl"),
		], relocations=[relocation()])
		candidate = evidence([
			instruction("pushl", "%ebp"),
			instruction("movl", "%esp, %ebp"),
			instruction("subl", "$0x10, %esp"),
			instruction("movl", "0x8(%ebp), %eax"),
			instruction("movl", "0xc(%ebp), %ecx"),
			instruction("retl"),
		], size=48, relocations=[relocation()])
		result = classify(target, candidate)
		self.assertEqual((result["kind"], result["recipe"]), ("RECIPE", "R1"))

	def test_r3_sib_vs_scaled_temporary(self):
		target = evidence([
			instruction("movl", "%edx, _entries(,%ecx,8)"),
			instruction("retl"),
		])
		candidate = evidence([
			instruction("leal", "(,%ecx,8), %eax"),
			instruction("movl", "%edx, _aggregate+4(%eax)"),
			instruction("retl"),
		], size=48)
		result = classify(target, candidate)
		self.assertEqual((result["kind"], result["recipe"]), ("RECIPE", "R3"))
		self.assertIn("ownership", result["stop_rule"])

	def test_r3_does_not_trigger_on_sib_load(self):
		target = evidence([
			instruction("movl", "_entries(,%ecx,8), %edx"),
			instruction("retl"),
		])
		candidate = evidence([
			instruction("leal", "(,%ecx,8), %eax"),
			instruction("movl", "_entries(%eax), %edx"),
			instruction("retl"),
		], size=48)
		self.assertEqual(classify(target, candidate)["kind"], "UNKNOWN")

	def test_r3_accepts_decimal_shl_scale(self):
		target = evidence([
			instruction("movl", "%edx, _entries(,%ecx,8)"),
			instruction("retl"),
		])
		candidate = evidence([
			instruction("shll", "$3, %ecx"),
			instruction("movl", "%edx, _aggregate+4(%ecx)"),
			instruction("retl"),
		], size=48)
		self.assertEqual(classify(target, candidate)["recipe"], "R3")

	def test_r4_x87_fst_fstp_liveness(self):
		target = evidence([
			instruction("flds", "0x8(%ebp)"),
			instruction("fsts", "(%eax)"),
			instruction("retl"),
		], relocations=[relocation()])
		candidate = evidence([
			instruction("flds", "0x8(%ebp)"),
			instruction("fstps", "(%eax)"),
			instruction("retl"),
		], relocations=[relocation()])
		result = classify(target, candidate)
		self.assertEqual((result["kind"], result["recipe"]), ("RECIPE", "R4"))

	def test_r5_early_return_shape(self):
		relocs = [relocation()]
		target = evidence([
			instruction("testl", "%eax, %eax"),
			instruction("je", "<L0>"),
			instruction("retl"),
			instruction("retl"),
		], relocations=relocs)
		candidate = evidence([
			instruction("testl", "%eax, %eax"),
			instruction("je", "<L0>"),
			instruction("retl"),
		], relocations=relocs)
		result = classify(target, candidate)
		self.assertEqual((result["kind"], result["recipe"]), ("RECIPE", "R5"))

	def test_pure_register_mirror_is_stop(self):
		relocs = [relocation()]
		target = evidence([
			instruction("pushl", "%esi"),
			instruction("movl", "0x8(%ebp), %esi"),
			instruction("addl", "%eax, %esi"),
			instruction("popl", "%esi"),
			instruction("retl"),
		], relocations=relocs)
		candidate = evidence([
			instruction("pushl", "%edi"),
			instruction("movl", "0x8(%ebp), %edi"),
			instruction("addl", "%eax, %edi"),
			instruction("popl", "%edi"),
			instruction("retl"),
		], relocations=relocs)
		result = classify(target, candidate)
		self.assertEqual(result["kind"], "STOP")
		self.assertEqual(result["class"], "pure-register-permutation")

	def test_register_mirror_requires_exact_relocation_addresses(self):
		target = evidence([
			instruction("movl", "%esi, %eax"), instruction("retl")],
			relocations=[relocation(address=4)])
		candidate = evidence([
			instruction("movl", "%edi, %eax"), instruction("retl")],
			relocations=[relocation(address=5)])
		self.assertEqual(classify(target, candidate)["kind"], "UNKNOWN")

	def test_independent_adjacent_swap_is_stop(self):
		relocs = [relocation()]
		target = evidence([
			instruction("fmuls", "0x30(%edi)"),
			instruction("addl", "$0x18, %esp"),
			instruction("retl"),
		], relocations=relocs)
		candidate = evidence([
			instruction("addl", "$0x18, %esp"),
			instruction("fmuls", "0x30(%edi)"),
			instruction("retl"),
		], relocations=relocs)
		result = classify(target, candidate)
		self.assertEqual(result["kind"], "STOP")
		self.assertEqual(result["class"], "independent-instruction-scheduling")

	def test_swap_with_register_dependency_refuses(self):
		target = evidence([
			instruction("movl", "(%eax), %ecx"),
			instruction("addl", "%ecx, %edx"),
			instruction("retl"),
		])
		candidate = evidence([
			instruction("addl", "%ecx, %edx"),
			instruction("movl", "(%eax), %ecx"),
			instruction("retl"),
		])
		self.assertEqual(classify(target, candidate)["kind"], "UNKNOWN")

	def test_non_bijective_register_rewrite_refuses(self):
		target = evidence([
			instruction("addl", "%esi, %eax"),
			instruction("subl", "%esi, %ecx"),
			instruction("retl"),
		])
		candidate = evidence([
			instruction("addl", "%edi, %eax"),
			instruction("subl", "%ebx, %ecx"),
			instruction("retl"),
		])
		self.assertEqual(classify(target, candidate)["kind"], "UNKNOWN")

	def test_fst_pair_plus_other_difference_refuses(self):
		target = evidence([
			instruction("fsts", "(%eax)"),
			instruction("movl", "%eax, %ecx"),
			instruction("retl"),
		], relocations=[relocation()])
		candidate = evidence([
			instruction("fstps", "(%eax)"),
			instruction("movl", "%eax, %edx"),
			instruction("retl"),
		], relocations=[relocation()])
		self.assertEqual(classify(target, candidate)["kind"], "UNKNOWN")

	def test_equal_measurements_are_refused_not_credited(self):
		instructions = [instruction("retl")]
		target = evidence(instructions, sha="same")
		candidate = evidence(instructions, sha="same")
		result = classify(target, candidate)
		self.assertEqual(result["kind"], "REFUSE")
		self.assertFalse(result["grants_match_credit"])

	def test_missing_disassembly_is_unknown(self):
		result = classify(evidence([]), evidence([]))
		self.assertEqual(result["kind"], "UNKNOWN")


if __name__ == "__main__":
	unittest.main()
