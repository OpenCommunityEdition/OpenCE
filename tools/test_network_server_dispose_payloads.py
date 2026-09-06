"""Compile real disposal and prove its two call-site payloads are initialized.

This is a deliberately narrow machine-code argument-block check, not a full
network test or a general x86 interpreter. Unknown instructions fail closed.
"""
from pathlib import Path
import subprocess
import sys

import capstone
from capstone import x86
import pytest

from tools import coff_compare as cc

ROOT = Path(__file__).resolve().parents[1]
UNIT = "source/networking/network_server_manager"
SOURCE = ROOT / (UNIT + ".c")
DISPOSE = "_network_game_server_dispose"
CREATE = "_create_network_game_message"
KINDS = {9: "pregame", 31: "postgame"}


class PayloadNotInitialized(AssertionError):
    """Only a successfully analyzed call with a nonzero/unknown payload."""


def payload_calls(obj):
    symbol = cc.symbol(obj, DISPOSE)
    assert symbol["section"] > 0 and symbol["value"] == 0
    section = obj["sections"][symbol["section"] - 1]
    raw = bytes(cc._section_bytes(obj, section))
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    instructions = list(md.disasm(raw, 0))
    assert instructions and sum(i.size for i in instructions) == len(raw)
    by_address = {i.address: i for i in instructions}
    starts = {0}
    for ins in instructions:
        if ins.group(capstone.CS_GRP_JUMP):
            assert len(ins.operands) == 1 and ins.operands[0].type == x86.X86_OP_IMM
            starts.add(ins.operands[0].imm)
            starts.add(ins.address + ins.size)
        elif ins.group(capstone.CS_GRP_RET):
            starts.add(ins.address + ins.size)
    calls = [r for r in cc.section_info(obj, DISPOSE)["relocations"]
             if r.get("symbolic_target", r["target"]) == ["symbol", CREATE, 0]]
    assert len(calls) == 2
    result = {}
    for rel in calls:
        assert rel["type"] == 20
        call = by_address[rel["address"] - 1]
        assert call.mnemonic == "call" and call.size == 5
        start = max(s for s in starts if s <= call.address)
        registers, memory, pushes = {}, {}, []

        def slot(op):
            assert op.type == x86.X86_OP_MEM
            assert op.mem.base == x86.X86_REG_EBP and op.mem.index == 0
            assert op.mem.segment == 0
            return op.mem.disp

        for ins in instructions:
            if not start <= ins.address < call.address:
                continue
            ops = ins.operands
            if ins.mnemonic == "push" and len(ops) == 1:
                assert ops[0].size == 4
                if ops[0].type == x86.X86_OP_IMM:
                    pushes.append(("constant", ops[0].imm))
                else:
                    assert ops[0].type == x86.X86_OP_REG and ops[0].size == 4
                    assert ops[0].reg in registers
                    pushes.append(registers[ops[0].reg])
            elif ins.mnemonic == "lea" and len(ops) == 2:
                assert ops[0].type == x86.X86_OP_REG and ops[0].size == 4
                assert ops[0].reg not in (x86.X86_REG_EBP, x86.X86_REG_ESP)
                registers[ops[0].reg] = ("stack-address", slot(ops[1]))
            elif ins.mnemonic == "mov" and len(ops) == 2:
                assert ops[0].size == 4 and ops[1].type == x86.X86_OP_IMM
                address = slot(ops[0])
                # Partial/overlapping/indirect stores are not modeled.
                assert address % 4 == 0
                memory[address] = ops[1].imm
            else:
                raise AssertionError(("unmodeled argument-block instruction",
                                      ins.address, ins.mnemonic, ins.op_str))
        assert len(pushes) == 3
        size, pointer, message_type = pushes
        assert size == ("constant", 4)
        assert pointer[0] == "stack-address"
        assert message_type[0] == "constant" and message_type[1] in KINDS
        kind = message_type[1]
        assert kind not in result
        result[kind] = {"zero": memory.get(pointer[1]) == 0,
                        "slot": pointer[1], "call_offset": call.address}
    assert set(result) == set(KINDS)
    return result


def require_zero(proof, kind):
    if not proof[kind]["zero"]:
        raise PayloadNotInitialized(kind)


@pytest.fixture(scope="module")
def compiler(tmp_path_factory):
    compiler_path = ROOT / "xbox/bin/vc7/CL.Exe"
    if not compiler_path.is_file() or not (ROOT / ("build/split/" + UNIT + ".obj")).is_file():
        pytest.skip("locally supplied VC7 compiler/January target is unavailable")
    folder = tmp_path_factory.mktemp("server-dispose-payload")
    source = SOURCE.read_bytes()

    def compile_one(name, remove_kind=None):
        code = source
        if remove_kind is not None:
            declaration = ("struct message_server_graceful_game_exit_"
                           + KINDS[remove_kind] + " message_packet").encode()
            needle = declaration + b" = { 0 };"
            assert code.count(needle) == 1
            code = code.replace(needle, declaration + b";", 1)
        input_path, output_path = folder / (name + ".c"), folder / (name + ".obj")
        input_path.write_bytes(code)
        completed = subprocess.run(
            [sys.executable, "-B", "tools/campaign/gate.py", UNIT,
             "--source", str(input_path), "--out", str(output_path)],
            cwd=ROOT, capture_output=True, text=True,
        )
        assert completed.returncode == 0, completed.stdout + completed.stderr
        assert SOURCE.read_bytes() == source, "source changed during test"
        return payload_calls(cc.load(output_path))

    positive = compile_one("actual")
    for kind in KINDS:
        require_zero(positive, kind)
    return compile_one


def test_both_actual_payloads_are_zero(compiler):
    assert callable(compiler)


@pytest.mark.parametrize("kind", [9, 31])
def test_missing_initializer_is_rejected_after_successful_compile(compiler, kind):
    proof = compiler("missing-" + KINDS[kind], kind)
    require_zero(proof, next(k for k in KINDS if k != kind))
    # Compilation and unsupported instructions cannot satisfy this control.
    with pytest.raises(PayloadNotInitialized) as error:
        require_zero(proof, kind)
    assert error.value.args == (kind,)
