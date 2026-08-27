from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import load, section_info, symbol


def normalized_bytes(path, function_name):
    obj = load(path)
    owner = symbol(obj, function_name)
    section = obj["sections"][owner["section"] - 1]
    raw = bytearray(
        obj["data"][section["raw"]:section["raw"] + section["size"]])
    for relocation_index in range(section["reloc_count"]):
        relocation_offset = section["reloc"] + relocation_index * 10
        address = struct.unpack_from("<L", obj["data"], relocation_offset)[0]
        raw[address:address + 4] = b"\0\0\0\0"
    return bytes(raw), section_info(obj, function_name)


target_bytes, target_info = normalized_bytes(sys.argv[1], sys.argv[4])
for candidate_path in sys.argv[2:4]:
    candidate_bytes, candidate_info = normalized_bytes(candidate_path, sys.argv[4])
    overlap = min(len(target_bytes), len(candidate_bytes))
    differing = sum(
        left != right
        for left, right in zip(target_bytes[:overlap], candidate_bytes[:overlap]))
    differing += abs(len(target_bytes) - len(candidate_bytes))

    def destination_equal(left, right):
        if left["type"] != right["type"]:
            return False
        if left["target"] == right["target"]:
            return True
        left_symbolic = left.get("symbolic_target")
        if left_symbolic is None and left["target"][0] == "symbol":
            left_symbolic = left["target"]
        right_symbolic = right.get("symbolic_target")
        if right_symbolic is None and right["target"][0] == "symbol":
            right_symbolic = right["target"]
        return left_symbolic is not None and left_symbolic == right_symbolic

    destination_sequence_equal = (
        len(candidate_info["relocations"]) == len(target_info["relocations"])
        and all(
            destination_equal(left, right)
            for left, right in zip(
                target_info["relocations"], candidate_info["relocations"])
        )
    )
    address_sequence_equal = [
        item["address"] for item in candidate_info["relocations"]
    ] == [item["address"] for item in target_info["relocations"]]
    print(
        candidate_path,
        f"size={len(candidate_bytes)}",
        f"differing={differing}",
        f"relocations={candidate_info['relocation_count']}",
        f"destination_sequence_equal={destination_sequence_equal}",
        f"address_sequence_equal={address_sequence_equal}",
        f"hash={candidate_info['normalized_sha256']}",
    )
