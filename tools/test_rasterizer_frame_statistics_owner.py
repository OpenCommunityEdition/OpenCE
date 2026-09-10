"""Pin January's frame-statistics ABI, storage owner, and Models wrap semantics."""

from contextlib import contextmanager
import faulthandler
from pathlib import Path
import re
import struct
import subprocess

import pytest

from tools import coff_compare as cc


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "xbox/bin/vc7/CL.Exe"
FIXTURE = ROOT / "tools/fixtures/rasterizer_frame_statistics_owner.c"
HEADER = ROOT / "source/rasterizer/rasterizer_frame_statistics.h"
MODELS_SOURCE = ROOT / "source/rasterizer/xbox/rasterizer_xbox_models.c"
BASE_ROOT = ROOT / "build/base"
TARGET_ROOT = ROOT / "build/split"
SYMBOL = "_rasterizer_frame_statistics"

# These offsets are fixed from the pre-migration writers and January's 0x170-byte
# linker-common owner.  They deliberately are not derived from the live header.
FIELDS = (
	("reserved000", 0x000, 0x30, None),
	("shadow_count", 0x030, 4, "unsigned"),
	("shadow_vertex_count", 0x034, 4, "unsigned"),
	("shadow_triangle_count", 0x038, 4, "unsigned"),
	("shadow_draw_count", 0x03C, 4, "unsigned"),
	("environment_dynamic_vertex_count", 0x040, 4, "unsigned"),
	("environment_dynamic_triangle_count", 0x044, 4, "unsigned"),
	("environment_dynamic_draw_count", 0x048, 4, "unsigned"),
	("decal_vertex_count", 0x04C, 4, "unsigned"),
	("decal_triangle_count", 0x050, 4, "unsigned"),
	("decal_draw_count", 0x054, 4, "unsigned"),
	("decal_shader_change_count", 0x058, 4, "unsigned"),
	("decal_texture_change_count", 0x05C, 4, "unsigned"),
	("diffuse_texture_dynamic_vertex_count", 0x060, 4, "unsigned"),
	("diffuse_texture_dynamic_triangle_count", 0x064, 4, "unsigned"),
	("diffuse_texture_dynamic_draw_count", 0x068, 4, "unsigned"),
	("specular_light_dynamic_vertex_count", 0x06C, 4, "unsigned"),
	("specular_light_dynamic_triangle_count", 0x070, 4, "unsigned"),
	("specular_light_dynamic_draw_count", 0x074, 4, "unsigned"),
	("specular_lightmap_dynamic_vertex_count", 0x078, 4, "unsigned"),
	("specular_lightmap_dynamic_triangle_count", 0x07C, 4, "unsigned"),
	("specular_lightmap_dynamic_draw_count", 0x080, 4, "unsigned"),
	("reflection_mask_dynamic_vertex_count", 0x084, 4, "unsigned"),
	("reflection_mask_dynamic_triangle_count", 0x088, 4, "unsigned"),
	("reflection_mask_dynamic_draw_count", 0x08C, 4, "unsigned"),
	("reflection_dynamic_vertex_count", 0x090, 4, "unsigned"),
	("reflection_dynamic_triangle_count", 0x094, 4, "unsigned"),
	("reflection_dynamic_draw_count", 0x098, 4, "unsigned"),
	("transparent_geometry_dynamic_vertex_count", 0x09C, 4, "unsigned"),
	("transparent_geometry_dynamic_triangle_count", 0x0A0, 4, "unsigned"),
	("transparent_geometry_largest_dynamic_triangle_count", 0x0A4, 4, "signed"),
	("transparent_geometry_dynamic_draw_count", 0x0A8, 4, "unsigned"),
	("environment_fog_dynamic_vertex_count", 0x0AC, 4, "unsigned"),
	("environment_fog_dynamic_triangle_count", 0x0B0, 4, "unsigned"),
	("environment_fog_dynamic_draw_count", 0x0B4, 4, "unsigned"),
	("reserved0B8", 0x0B8, 0x1C, None),
	("model_count", 0x0D4, 4, "unsigned"),
	("reserved0D8", 0x0D8, 0x1C, None),
	("model_shadow_count", 0x0F4, 4, "unsigned"),
	("model_shadow_vertex_count", 0x0F8, 4, "unsigned"),
	("model_shadow_triangle_count", 0x0FC, 4, "unsigned"),
	("model_shadow_draw_count", 0x100, 4, "unsigned"),
	("dynamic_unlit_draw_count", 0x104, 4, "unsigned"),
	("dynamic_unlit_triangle_count", 0x108, 4, "unsigned"),
	("largest_dynamic_unlit_triangle_count", 0x10C, 4, "signed"),
	("dynamic_unlit_vertex_count", 0x110, 4, "unsigned"),
	("reserved114", 0x114, 0x1C, None),
	("dynamic_vertex_count", 0x130, 4, "signed"),
	("dynamic_vertex_buffer_count", 0x134, 4, "signed"),
	("dynamic_triangle_count", 0x138, 4, "signed"),
	("dynamic_triangle_buffer_count", 0x13C, 4, "signed"),
	("debug_primitive_count", 0x140, 4, "signed"),
	("reserved144", 0x144, 4, None),
	("dynamic_light_count", 0x148, 4, "signed"),
	("lens_flare_count", 0x14C, 4, "signed"),
	("vertex_shader_skinning_constant_bytes", 0x150, 4, "signed"),
	("vertex_shader_lighting_constant_bytes", 0x154, 4, "signed"),
	("vertex_shader_instruction_count", 0x158, 4, "signed"),
	("pixel_shader_pushbuffer_bytes", 0x15C, 4, "signed"),
	("model_skinning_constant_bytes", 0x160, 4, "unsigned"),
	("model_lighting_constant_bytes", 0x164, 4, "unsigned"),
	("reserved168", 0x168, 8, None),
)

IMPORTERS = frozenset({
	"source/rasterizer/rasterizer_debug.obj",
	"source/rasterizer/rasterizer_frame_statistics.obj",
	"source/rasterizer/rasterizer_lights.obj",
	"source/rasterizer/xbox/rasterizer_xbox.obj",
	"source/rasterizer/xbox/rasterizer_xbox_decals.obj",
	"source/rasterizer/xbox/rasterizer_xbox_draw_primitives.obj",
	"source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.obj",
	"source/rasterizer/xbox/rasterizer_xbox_environment.obj",
	"source/rasterizer/xbox/rasterizer_xbox_environment_fog.obj",
	"source/rasterizer/xbox/rasterizer_xbox_models.obj",
	"source/rasterizer/xbox/rasterizer_xbox_shadows.obj",
	"source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_runtime.obj",
})


class FrameStatisticsLayoutMismatch(AssertionError):
	pass


def compile_fixture(output, override_root=None, check=True):
	include_roots = []
	if override_root is not None:
		include_roots.append(Path(override_root))
	include_roots.extend((ROOT / "source", ROOT / "source/cseries", ROOT / "xbox/include"))
	command = [str(COMPILER), "/nologo", "/c", "/Dxbox", "/W1", "/WX"]
	command.extend("/I" + str(path) for path in include_roots)
	command.extend(("/Fo" + str(output), str(FIXTURE)))
	result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
	if check:
		assert result.returncode == 0, result.stdout + result.stderr
		assert output.is_file() and output.stat().st_size > 0
	return result


@pytest.fixture(scope="module")
def compiled_fixture(tmp_path_factory):
	if not COMPILER.is_file():
		pytest.skip("the locally supplied VC7/XDK compiler is unavailable")
	output = tmp_path_factory.mktemp("frame-statistics-owner") / "owner.obj"
	compile_fixture(output)
	return cc.load(output)


def words(obj, name, count):
	owners = [symbol for symbol in obj["symbols"]
		if symbol["name"] == name and symbol["section"] > 0]
	assert len(owners) == 1
	owner = owners[0]
	section = obj["sections"][owner["section"] - 1]
	assert owner["value"] + count * 4 <= section["size"]
	raw = cc._section_bytes(obj, section)
	return struct.unpack_from("<" + "I" * count, raw, owner["value"])


def require_layout(obj):
	values = words(obj, "_rasterizer_frame_statistics_layout", 1 + 2 * len(FIELDS))
	if values[0] != 0x170:
		raise FrameStatisticsLayoutMismatch(
			f"structure size is 0x{values[0]:X}, expected 0x170")
	for index, (name, offset, size, _) in enumerate(FIELDS):
		actual = values[1 + 2 * index:3 + 2 * index]
		if actual != (offset, size):
			raise FrameStatisticsLayoutMismatch(
				f"{name} is (0x{actual[0]:X}, 0x{actual[1]:X}), "
				f"expected (0x{offset:X}, 0x{size:X})")


def write_override(tmp_path, old, new):
	source = HEADER.read_text(encoding="utf-8")
	assert source.count(old) == 1
	override = tmp_path / "include/rasterizer/rasterizer_frame_statistics.h"
	override.parent.mkdir(parents=True)
	override.write_text(source.replace(old, new), encoding="utf-8")
	return override.parents[1]


def symbol_records(root):
	records = {}
	for path in root.rglob("*.obj"):
		obj = cc.load(path)
		matches = [symbol for symbol in obj["symbols"] if symbol["name"] == SYMBOL]
		if matches:
			relative = path.relative_to(root).as_posix()
			records[relative] = (obj, matches)
	return records


def test_complete_layout_matches_independent_january_offsets(compiled_fixture):
	assert FIELDS[0][1] == 0
	for previous, current in zip(FIELDS, FIELDS[1:]):
		assert previous[1] + previous[2] == current[1]
	assert FIELDS[-1][1] + FIELDS[-1][2] == 0x170
	require_layout(compiled_fixture)


def test_vc7_type_contract_and_fixture_emit_only_an_import(compiled_fixture):
	records = [symbol for symbol in compiled_fixture["symbols"]
		if symbol["name"] == SYMBOL]
	assert len(records) == 1
	assert (records[0]["value"], records[0]["section"], records[0]["type"],
		records[0]["storage"]) == (0, 0, 0, 2)


def test_source_has_one_extern_declaration_and_no_tentative_storage():
	pattern = re.compile(
		r"^\s*(extern\s+)?struct\s+rasterizer_frame_statistics_globals\s+"
		r"rasterizer_frame_statistics\s*;",
		re.MULTILINE)
	records = []
	for path in (ROOT / "source").rglob("*"):
		if path.suffix not in {".c", ".h"}:
			continue
		for match in pattern.finditer(path.read_text(encoding="utf-8", errors="replace")):
			records.append((path.relative_to(ROOT).as_posix(), bool(match.group(1))))
	assert records == [("source/rasterizer/rasterizer_frame_statistics.h", True)]


def test_january_has_one_0x170_bss_owner_and_exact_importer_set():
	if not (TARGET_ROOT / "source/linker_common.obj").is_file():
		pytest.skip("the locally supplied January split objects are unavailable")
	target = symbol_records(TARGET_ROOT)
	assert set(target) == IMPORTERS | {"source/linker_common.obj"}
	for path in IMPORTERS:
		_, records = target[path]
		assert len(records) == 1
		assert (records[0]["value"], records[0]["section"], records[0]["type"],
			records[0]["storage"]) == (0, 0, 0, 2), path

	owner_object, owner_records = target["source/linker_common.obj"]
	assert len(owner_records) == 1
	owner = owner_records[0]
	assert (owner["value"], owner["section"], owner["type"], owner["storage"]) == (
		0, 58, 0, 2)
	section = owner_object["sections"][57]
	assert (section["name"], section["size"], section["reloc_count"],
		section["flags"]) == (".bss", 0x170, 0, 0xC0600080)
	assert section["raw"] > 0  # csplit materializes January's zero-filled BSS.
	assert cc._section_bytes(owner_object, section) == bytearray(0x170)


def test_rebuilt_consumers_import_without_common_or_defined_storage():
	if not BASE_ROOT.is_dir():
		pytest.skip("the locally built base objects are unavailable")
	base = symbol_records(BASE_ROOT)
	assert set(base) == IMPORTERS
	for path, (_, records) in base.items():
		assert len(records) == 1
		assert (records[0]["value"], records[0]["section"], records[0]["type"],
			records[0]["storage"]) == (0, 0, 0, 2), path


def test_models_keeps_unsigned_delta_and_accumulator_contract():
	source = MODELS_SOURCE.read_text(encoding="utf-8")
	start = source.index("void _rasterizer_model_begin(")
	body = " ".join(source[start:].split())
	assert body.count("unsigned long skinning_work;") == 1
	assert body.count("unsigned long lighting_work;") == 1
	assert body.count(
		"skinning_work = rasterizer_frame_statistics."
		"skinning_work;") == 1
	assert body.count(
		"skinning_work = rasterizer_frame_statistics."
		"skinning_work - skinning_work;") == 1
	assert body.count(
		"lighting_work = rasterizer_frame_statistics."
		"lighting_work;") == 1
	assert body.count(
		"rasterizer_frame_statistics.skinning_work_accumulated += "
		"skinning_work;") == 1
	assert body.count(
		"rasterizer_frame_statistics.lighting_work_accumulated += "
		"rasterizer_frame_statistics.lighting_work - "
		"lighting_work;") == 1


@contextmanager
def quiet_unicorn_windows_exceptions():
	enabled = faulthandler.is_enabled()
	if enabled:
		faulthandler.disable()
	try:
		yield
	finally:
		if enabled:
			faulthandler.enable()


def run_u32_function(obj, name, arguments):
	u = pytest.importorskip("unicorn")
	from unicorn import x86_const as x

	owner = cc.symbol(obj, name)
	section = obj["sections"][owner["section"] - 1]
	end = min((symbol["value"] for symbol in obj["symbols"]
		if symbol["section"] == owner["section"] and symbol["type"] == 0x20
		and symbol["value"] > owner["value"]), default=section["size"])
	for index in range(section["reloc_count"]):
		address = struct.unpack_from("<I", obj["data"], section["reloc"] + index * 10)[0]
		assert not owner["value"] <= address < end
	raw = bytes(cc._section_bytes(obj, section))
	code = 0x00100000
	code_size = ((len(raw) + 0xFFF) & ~0xFFF) + 0x1000
	stop = code + code_size - 0x10
	stack = 0x00200000
	esp = stack + 0x800
	with quiet_unicorn_windows_exceptions():
		cpu = u.Uc(u.UC_ARCH_X86, u.UC_MODE_32)
		cpu.mem_map(code, code_size)
		cpu.mem_write(code, raw)
		cpu.mem_map(stack, 0x1000)
		cpu.mem_write(esp, struct.pack("<" + "I" * (1 + len(arguments)),
			stop, *(argument & 0xFFFFFFFF for argument in arguments)))
		cpu.reg_write(x.UC_X86_REG_ESP, esp)
		cpu.emu_start(code + owner["value"], stop, count=10000)
		assert cpu.reg_read(x.UC_X86_REG_EIP) == stop
		return cpu.reg_read(x.UC_X86_REG_EAX) & 0xFFFFFFFF


def test_compiled_models_arithmetic_is_modulo_2_to_32(compiled_fixture):
	delta_cases = (
		(10, 3),
		(0x80000000, 0x7FFFFFFF),
		(0x7FFFFFFF, 0x80000000),
		(0, 0xFFFFFFFF),
	)
	for after, before in delta_cases:
		expected = (after - before) & 0xFFFFFFFF
		assert run_u32_function(
			compiled_fixture, "_rasterizer_frame_statistics_model_delta",
			(after, before)) == expected

	accumulation_cases = (
		(0xFFFFFFFF, 0x80000000, 0x7FFFFFFF),
		(1, 0x7FFFFFFF, 0x80000000),
		(0x80000000, 0, 0xFFFFFFFF),
	)
	for accumulator, after, before in accumulation_cases:
		expected = (accumulator + after - before) & 0xFFFFFFFF
		assert run_u32_function(
			compiled_fixture, "_rasterizer_frame_statistics_model_accumulate",
			(accumulator, after, before)) == expected


def test_offset_negative_control_detects_shifted_writer(tmp_path):
	if not COMPILER.is_file():
		pytest.skip("the locally supplied VC7/XDK compiler is unavailable")
	override = write_override(tmp_path, "byte reserved000[0x30];", "byte reserved000[0x2C];")
	output = tmp_path / "shifted.obj"
	compile_fixture(output, override)
	with pytest.raises(FrameStatisticsLayoutMismatch, match="structure size|reserved000"):
		require_layout(cc.load(output))


@pytest.mark.parametrize(("old", "new"), (
	("long vertex_shader_skinning_constant_bytes;",
		"unsigned long vertex_shader_skinning_constant_bytes;"),
	("unsigned long model_skinning_constant_bytes;",
		"long model_skinning_constant_bytes;"),
))
def test_signedness_negative_control_fails_vc7_type_contract(tmp_path, old, new):
	if not COMPILER.is_file():
		pytest.skip("the locally supplied VC7/XDK compiler is unavailable")
	override = write_override(tmp_path, old, new)
	result = compile_fixture(tmp_path / "wrong-signedness.obj", override, check=False)
	assert result.returncode != 0
	assert "C4057" in result.stdout + result.stderr
