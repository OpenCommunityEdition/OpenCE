import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from tools.coff_compare import _section_bytes, load, symbol


function_name = sys.argv[1]
for path in sys.argv[2:]:
    obj = load(path)
    function = symbol(obj, function_name)
    section = obj["sections"][function["section"] - 1]
    code = bytes(_section_bytes(obj, section))
    print("===", path, function_name, "===")
    for instruction in Cs(CS_ARCH_X86, CS_MODE_32).disasm(code, 0):
        print(
            f"{instruction.address:04x}: "
            f"{instruction.bytes.hex(' '):<24} "
            f"{instruction.mnemonic:<8} {instruction.op_str}")
