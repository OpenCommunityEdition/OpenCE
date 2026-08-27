import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from capstone import CS_ARCH_X86, CS_MODE_32, Cs

from tools import coff_compare


obj = coff_compare.load(sys.argv[1])
name = sys.argv[2]
owner = coff_compare.symbol(obj, name)
section = obj["sections"][owner["section"] - 1]
data = bytes(coff_compare._section_bytes(obj, section))
info = coff_compare.section_info(obj, name)
relocations = {item["address"]: item for item in info["relocations"]}

decoder = Cs(CS_ARCH_X86, CS_MODE_32)
for instruction in decoder.disasm(data, 0):
    annotations = []
    for offset in range(instruction.address, instruction.address + instruction.size):
        relocation = relocations.get(offset)
        if relocation is not None:
            annotations.append(
                "rel+%04x type=%04x target=%r"
                % (offset, relocation["type"], relocation["target"])
            )
    suffix = " ; " + ", ".join(annotations) if annotations else ""
    print(
        "%04x  %-20s %-7s %s%s"
        % (
            instruction.address,
            instruction.bytes.hex(" "),
            instruction.mnemonic,
            instruction.op_str,
            suffix,
        )
    )
