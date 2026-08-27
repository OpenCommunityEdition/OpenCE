import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from tools.coff_compare import _section_bytes, load, symbol


for path in (
    "build/split/source/ai/action_uncover.obj",
    "build/audit/action_uncover_short_return_corrected_candidate_20260826.obj",
):
    obj = load(path)
    function = symbol(obj, "_action_uncover_perform")
    section = obj["sections"][function["section"] - 1]
    code = bytes(_section_bytes(obj, section))
    print("===", path, "===")
    for instruction in Cs(CS_ARCH_X86, CS_MODE_32).disasm(code, 0):
        if 140 <= instruction.address <= 230:
            print(
                f"{instruction.address:04x}: "
                f"{instruction.bytes.hex(' '):<24} "
                f"{instruction.mnemonic:<8} {instruction.op_str}")
