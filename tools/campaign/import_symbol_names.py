"""Safely import authoritative symbol names from another git revision.

The worktree's ``config/symbols.json`` is always authoritative for fields other
than a still-placeholder name.  A donor may replace only ``_code_*``,
``_bss_*``, ``_data_*``, or ``_rdata_*`` names at the same file offset.  The
command is a dry-run unless ``--apply`` is given::

    python tools/campaign/import_symbol_names.py some/ref
    python tools/campaign/import_symbol_names.py some/ref --apply

Apply mode also updates exact JSON string values in the semantic ledgers and C
identifier tokens in tracked ``source/**/*.c`` and ``source/**/*.h`` files.
Comments and string/character literals are deliberately not rewritten.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from dataclasses import dataclass
import json
from pathlib import Path
import re
import subprocess
import sys
from typing import Iterable, Mapping, Sequence


SYMBOLS_PATH = Path("config/symbols.json")
EXACT_NAME_JSON_PATHS = (
	Path("config/semantic_matches.json"),
	Path("config/semantic_data_matches.json"),
	Path("config/parked.json"),
)

# COFF cdecl names begin with '_', stdcall names are '_name@N', and fastcall
# names are '@name@N'.  January's address-derived names use a leading
# underscore even when an @N suffix is present.  ``rdata`` belongs to the same
# placeholder family as code, BSS, and writable data.
_PLACEHOLDER_RE = re.compile(
	r"^_(?:code|bss|data|rdata)_[0-9a-f]+(?:@[0-9]+)?$"
)
_CDECL_OR_STDCALL_RE = re.compile(
	r"^_([A-Za-z_][A-Za-z0-9_]*)(?:@[0-9]+)?$"
)
_FASTCALL_RE = re.compile(r"^@([A-Za-z_][A-Za-z0-9_]*)@[0-9]+$")
_UNDECORATED_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_JSON_STRING_RE = re.compile(r'"(?:\\.|[^"\\])*"')
_SYMBOL_OBJECT_RE = re.compile(r"\{[^{}]*\}", re.DOTALL)
_FILE_OFFSET_RE = re.compile(r'"file_offset"\s*:\s*(-?[0-9]+)')
_NAME_FIELD_RE = re.compile(
	r'(?P<prefix>"name"\s*:\s*)(?P<value>"(?:\\.|[^"\\])*")'
)

_C_KEYWORDS = {
	"auto", "break", "case", "char", "const", "continue", "default",
	"do", "double", "else", "enum", "extern", "float", "for", "goto",
	"if", "inline", "int", "long", "register", "restrict", "return",
	"short", "signed", "sizeof", "static", "struct", "switch", "typedef",
	"union", "unsigned", "void", "volatile", "while", "_Bool", "_Complex",
	"_Imaginary",
}


class ImportErrorDetail(RuntimeError):
	"""An input cannot be imported without guessing or rebuilding JSON."""


@dataclass(frozen=True)
class Candidate:
	file_offset: int
	old_name: str
	new_name: str
	add_static: bool = False


@dataclass(frozen=True)
class NameConflict:
	file_offset: int
	canonical_name: str
	donor_name: str


@dataclass(frozen=True)
class SourceCollision:
	path: str
	old_identifier: str
	new_identifier: str | None
	reason: str


@dataclass(frozen=True)
class ImportPlan:
	candidates: tuple[Candidate, ...]
	real_name_conflicts: tuple[NameConflict, ...]


@dataclass(frozen=True)
class CandidateSelection:
	candidates: tuple[Candidate, ...]
	excluded: tuple[Candidate, ...]
	unused_offsets: tuple[int, ...]


def is_placeholder_symbol(name: object) -> bool:
	"""Return whether *name* is an address-derived code/data placeholder."""

	return isinstance(name, str) and _PLACEHOLDER_RE.fullmatch(name) is not None


def parse_file_offset(value: str) -> int:
	"""Parse a non-negative decimal or ``0x``-prefixed hexadecimal offset."""

	try:
		base = 16 if value.lower().startswith("0x") else 10
		offset = int(value, base)
	except ValueError as error:
		raise argparse.ArgumentTypeError(
			f"invalid file offset {value!r}; use decimal or 0x-prefixed hexadecimal"
		) from error
	if offset < 0:
		raise argparse.ArgumentTypeError("file offsets cannot be negative")
	return offset


def symbol_to_c_identifier(name: str) -> str | None:
	"""Convert cdecl/stdcall/fastcall COFF decoration to a C identifier."""

	match = _FASTCALL_RE.fullmatch(name)
	if match:
		return match.group(1)
	match = _CDECL_OR_STDCALL_RE.fullmatch(name)
	if match:
		return match.group(1)
	if _UNDECORATED_RE.fullmatch(name):
		return name
	return None


def _index_by_file_offset(
	entries: Sequence[Mapping[str, object]], label: str
) -> dict[int, list[Mapping[str, object]]]:
	indexed: dict[int, list[Mapping[str, object]]] = defaultdict(list)
	for entry in entries:
		offset = entry.get("file_offset")
		if not isinstance(offset, int) or isinstance(offset, bool):
			raise ImportErrorDetail(f"{label} has a non-integer file_offset: {offset!r}")
		indexed[offset].append(entry)
	return dict(indexed)


def build_import_plan(
	canonical_entries: Sequence[Mapping[str, object]],
	donor_entries: Sequence[Mapping[str, object]],
) -> ImportPlan:
	"""Pair entries by file offset and select only placeholder-to-real renames."""

	canonical_by_offset = _index_by_file_offset(canonical_entries, "canonical symbols")
	donor_by_offset = _index_by_file_offset(donor_entries, "donor symbols")
	candidates: list[Candidate] = []
	conflicts: list[NameConflict] = []

	for offset, canonical_group in canonical_by_offset.items():
		donor_group = donor_by_offset.get(offset)
		if donor_group is None:
			continue
		for entry in [*canonical_group, *donor_group]:
			if not isinstance(entry.get("name"), str) or not entry.get("name"):
				raise ImportErrorDetail(f"invalid name at file_offset {offset}")

		donor_real_entries = [
			entry for entry in donor_group
			if not is_placeholder_symbol(entry["name"])
		]
		donor_real_names = list(dict.fromkeys(entry["name"] for entry in donor_real_entries))
		canonical_real_names = list(
			dict.fromkeys(
				entry["name"] for entry in canonical_group
				if not is_placeholder_symbol(entry["name"])
			)
		)

		for canonical in canonical_group:
			canonical_name = canonical["name"]
			if not is_placeholder_symbol(canonical_name):
				continue
			# Multiple real aliases at one address cannot be paired to a single
			# placeholder without additional provenance.  Leave them untouched.
			if len(donor_real_names) != 1:
				continue
			donor_name = donor_real_names[0]
			donor_static = any(
				entry["name"] == donor_name and entry.get("static") is True
				for entry in donor_real_entries
			)
			candidates.append(
				Candidate(
					file_offset=offset,
					old_name=canonical_name,
					new_name=donor_name,
					add_static=(donor_static and "static" not in canonical),
				)
			)

		# Real aliases shared by both revisions are already reconciled.  Report
		# only canonical real names for which the donor has different real
		# information at that address; never turn those into candidates.
		if donor_real_names:
			for canonical_name in canonical_real_names:
				if canonical_name not in donor_real_names:
					conflicts.append(
						NameConflict(offset, canonical_name, " | ".join(donor_real_names))
					)

	return ImportPlan(tuple(candidates), tuple(conflicts))


def select_candidates(
	candidates: Sequence[Candidate], excluded_offsets: Iterable[int]
) -> CandidateSelection:
	"""Remove explicitly excluded offsets before collision or write planning."""

	excluded_offset_set = set(excluded_offsets)
	selected: list[Candidate] = []
	excluded: list[Candidate] = []
	matched_offsets: set[int] = set()
	for candidate in candidates:
		if candidate.file_offset in excluded_offset_set:
			excluded.append(candidate)
			matched_offsets.add(candidate.file_offset)
		else:
			selected.append(candidate)
	return CandidateSelection(
		tuple(selected), tuple(excluded),
		tuple(sorted(excluded_offset_set - matched_offsets)),
	)


def _insert_static_field(object_text: str) -> str:
	"""Append ``static: true`` while retaining the object's existing layout."""

	if re.search(r'"static"\s*:', object_text):
		return object_text

	closing_line = re.search(r"(\r?\n)([ \t]*)\}\s*$", object_text)
	if closing_line:
		property_indents = re.findall(r"(?:^|\r?\n)([ \t]*)\"", object_text)
		property_indent = property_indents[-1] if property_indents else "\t"
		before_closing_line = object_text[: closing_line.start(1)]
		trailing_space_match = re.search(r"[ \t]*$", before_closing_line)
		trailing_space = trailing_space_match.group(0) if trailing_space_match else ""
		value_end = len(before_closing_line) - len(trailing_space)
		return (
			before_closing_line[:value_end]
			+ ","
			+ trailing_space
			+ closing_line.group(1)
			+ property_indent
			+ '"static": true'
			+ object_text[closing_line.start(1) :]
		)

	closing = re.search(r"([ \t]*)\}$", object_text)
	if closing is None:
		raise ImportErrorDetail("could not locate the end of a symbols.json entry")
	return (
		object_text[: closing.start(1)]
		+ ', "static": true'
		+ object_text[closing.start(1) :]
	)


def update_symbols_text(text: str, candidates: Sequence[Candidate]) -> str:
	"""Apply candidate entries without serializing or reformatting the JSON."""

	by_entry = {
		(candidate.file_offset, candidate.old_name): candidate for candidate in candidates
	}
	if len(by_entry) != len(candidates):
		raise ImportErrorDetail("candidate list repeats a file_offset/name pair")
	seen: set[tuple[int, str]] = set()

	def replace_object(match: re.Match[str]) -> str:
		object_text = match.group(0)
		offset_match = _FILE_OFFSET_RE.search(object_text)
		if offset_match is None:
			return object_text
		offset = int(offset_match.group(1))
		name_match = _NAME_FIELD_RE.search(object_text)
		if name_match is None:
			return object_text
		current_name = json.loads(name_match.group("value"))
		candidate = by_entry.get((offset, current_name))
		if candidate is None:
			return object_text
		new_name_literal = json.dumps(candidate.new_name, ensure_ascii=False)
		updated = (
			object_text[: name_match.start("value")]
			+ new_name_literal
			+ object_text[name_match.end("value") :]
		)
		if candidate.add_static:
			updated = _insert_static_field(updated)
		seen.add((offset, current_name))
		return updated

	updated_text = _SYMBOL_OBJECT_RE.sub(replace_object, text)
	missing = sorted(set(by_entry) - seen)
	if missing:
		raise ImportErrorDetail(
			"could not locate symbols.json entries "
			+ ", ".join(f"{offset}:{name}" for offset, name in missing)
		)
	return updated_text


def replace_exact_json_strings(text: str, names: Mapping[str, str]) -> tuple[str, int]:
	"""Replace JSON string tokens only when their decoded value is an old name."""

	count = 0

	def replace_string(match: re.Match[str]) -> str:
		nonlocal count
		value = json.loads(match.group(0))
		new_value = names.get(value)
		if new_value is None:
			return match.group(0)
		count += 1
		return json.dumps(new_value, ensure_ascii=False)

	return _JSON_STRING_RE.sub(replace_string, text), count


def _is_identifier_start(character: str) -> bool:
	return character == "_" or "A" <= character <= "Z" or "a" <= character <= "z"


def _is_identifier_continue(character: str) -> bool:
	return _is_identifier_start(character) or "0" <= character <= "9"


def iter_c_identifiers(text: str) -> Iterable[tuple[int, int, str]]:
	"""Yield C identifier spans, excluding comments and string/char literals."""

	index = 0
	length = len(text)
	while index < length:
		if text.startswith("//", index):
			newline = text.find("\n", index + 2)
			index = length if newline < 0 else newline + 1
			continue
		if text.startswith("/*", index):
			end = text.find("*/", index + 2)
			index = length if end < 0 else end + 2
			continue
		character = text[index]
		if character in {'"', "'"}:
			quote = character
			index += 1
			while index < length:
				if text[index] == "\\":
					index = min(index + 2, length)
				elif text[index] == quote:
					index += 1
					break
				else:
					index += 1
			continue
		if _is_identifier_start(character):
			end = index + 1
			while end < length and _is_identifier_continue(text[end]):
				end += 1
			yield index, end, text[index:end]
			index = end
			continue
		index += 1


def replace_c_identifiers(
	text: str, names: Mapping[str, str]
) -> tuple[str, Counter[str]]:
	"""Replace exact C tokens and return replacement counts by old identifier."""

	pieces: list[str] = []
	last = 0
	counts: Counter[str] = Counter()
	for start, end, identifier in iter_c_identifiers(text):
		new_identifier = names.get(identifier)
		if new_identifier is None:
			continue
		pieces.append(text[last:start])
		pieces.append(new_identifier)
		last = end
		counts[identifier] += 1
	if not pieces:
		return text, counts
	pieces.append(text[last:])
	return "".join(pieces), counts


def _candidate_maps(
	candidates: Sequence[Candidate],
) -> tuple[dict[str, str], dict[str, str], list[SourceCollision]]:
	"""Create unambiguous symbol/C mappings and report mapping-level hazards."""

	symbol_names: dict[str, str] = {}
	c_names: dict[str, str] = {}
	issues: list[SourceCollision] = []
	for candidate in candidates:
		previous_symbol = symbol_names.get(candidate.old_name)
		if previous_symbol is not None and previous_symbol != candidate.new_name:
			issues.append(
				SourceCollision(
					"<mapping>", candidate.old_name, candidate.new_name,
					f"one placeholder maps to both {previous_symbol!r} and {candidate.new_name!r}",
				)
			)
		else:
			symbol_names[candidate.old_name] = candidate.new_name

		old_identifier = symbol_to_c_identifier(candidate.old_name)
		new_identifier = symbol_to_c_identifier(candidate.new_name)
		if old_identifier is None:
			issues.append(
				SourceCollision(
					"<mapping>", candidate.old_name, new_identifier,
					"placeholder decoration cannot be represented as a C identifier",
				)
			)
			continue
		if new_identifier is None or new_identifier in _C_KEYWORDS:
			# This is only fatal when the placeholder occurs in source.  Record the
			# missing mapping here and refine it after scanning source.
			continue
		previous_identifier = c_names.get(old_identifier)
		if previous_identifier is not None and previous_identifier != new_identifier:
			issues.append(
				SourceCollision(
					"<mapping>", old_identifier, new_identifier,
					f"one C identifier maps to both {previous_identifier!r} and {new_identifier!r}",
				)
			)
		else:
			c_names[old_identifier] = new_identifier
	return symbol_names, c_names, issues


def detect_source_collisions(
	source_texts: Mapping[str, str], candidates: Sequence[Candidate]
) -> list[SourceCollision]:
	"""Find same-file identifier merges or unsupported decorated donor names."""

	_, c_names, collisions = _candidate_maps(candidates)
	candidate_identifiers: list[tuple[str, str | None]] = []
	for candidate in candidates:
		old_identifier = symbol_to_c_identifier(candidate.old_name)
		if old_identifier is None:
			continue
		candidate_identifiers.append(
			(old_identifier, symbol_to_c_identifier(candidate.new_name))
		)

	for path, text in source_texts.items():
		identifiers = Counter(identifier for _, _, identifier in iter_c_identifiers(text))
		for old_identifier, new_identifier in candidate_identifiers:
			if identifiers[old_identifier] == 0:
				continue
			if new_identifier is None or new_identifier in _C_KEYWORDS:
				collisions.append(
					SourceCollision(
						path, old_identifier, new_identifier,
						"donor symbol has no safe C identifier spelling",
					)
				)
			elif new_identifier != old_identifier and identifiers[new_identifier]:
				collisions.append(
					SourceCollision(
						path, old_identifier, new_identifier,
						"old and new identifiers already coexist in this source file",
					)
				)

		# Two placeholders becoming one identifier in a single TU can silently
		# merge declarations/definitions even if the final name was absent before.
		olds_by_new: dict[str, list[str]] = defaultdict(list)
		for old_identifier, new_identifier in c_names.items():
			if identifiers[old_identifier]:
				olds_by_new[new_identifier].append(old_identifier)
		for new_identifier, old_identifiers in olds_by_new.items():
			if len(old_identifiers) > 1:
				collisions.append(
					SourceCollision(
						path, ", ".join(sorted(old_identifiers)), new_identifier,
						"multiple placeholders would merge into one identifier in this file",
					)
				)

	# Keep diagnostics deterministic and remove duplicates caused by repeated
	# symbol entries that have the same old/new spellings.
	return list(dict.fromkeys(collisions))


def _run_git(repo: Path, arguments: Sequence[str]) -> bytes:
	process = subprocess.run(
		["git", *arguments], cwd=repo, stdout=subprocess.PIPE,
		stderr=subprocess.PIPE, check=False,
	)
	if process.returncode:
		raise ImportErrorDetail(
			process.stderr.decode("utf-8", errors="replace").strip()
			or f"git {' '.join(arguments)} failed"
		)
	return process.stdout


def _repo_root(path: Path) -> Path:
	root = _run_git(path, ["rev-parse", "--show-toplevel"])
	return Path(root.decode("utf-8", errors="surrogateescape").strip())


def _load_json_entries(raw: bytes, label: str) -> list[Mapping[str, object]]:
	try:
		value = json.loads(raw.decode("utf-8-sig"))
	except (UnicodeDecodeError, json.JSONDecodeError) as error:
		raise ImportErrorDetail(f"could not parse {label}: {error}") from error
	if not isinstance(value, list) or not all(isinstance(item, dict) for item in value):
		raise ImportErrorDetail(f"{label} must be an array of objects")
	return value


def _tracked_source_paths(repo: Path) -> list[Path]:
	raw = _run_git(repo, ["ls-files", "-z", "--", "source"])
	paths = []
	for encoded_path in raw.split(b"\0"):
		if not encoded_path:
			continue
		relative = encoded_path.decode("utf-8", errors="surrogateescape")
		if Path(relative).suffix.lower() in {".c", ".h"}:
			paths.append(Path(relative))
	return paths


def _read_lossless(path: Path) -> str:
	return path.read_bytes().decode("latin-1")


def _write_lossless(path: Path, text: str) -> None:
	path.write_bytes(text.encode("latin-1"))


def _print_limited(title: str, lines: Sequence[str], verbose: bool) -> None:
	print(f"{title}: {len(lines)}")
	limit = len(lines) if verbose else min(len(lines), 40)
	for line in lines[:limit]:
		print(f"  {line}")
	if limit < len(lines):
		print(f"  ... {len(lines) - limit} more (use --verbose)")


def _candidate_report_lines(candidates: Sequence[Candidate]) -> list[str]:
	return [
		f"0x{item.file_offset:08x}: {item.old_name} -> {item.new_name}"
		+ (" [add static]" if item.add_static else "")
		for item in candidates
	]


def print_import_report(
	candidates: Sequence[Candidate],
	excluded: Sequence[Candidate],
	unused_exclusions: Sequence[int],
	conflicts: Sequence[NameConflict],
	collisions: Sequence[SourceCollision],
	verbose: bool = False,
) -> None:
	"""Print the complete dry-run/apply admission report."""

	_print_limited(
		"placeholder-to-real candidates", _candidate_report_lines(candidates), verbose
	)
	# Explicit exclusions are always enumerated: they are an audit trail, not a
	# quiet filter, so this list is never truncated.
	_print_limited(
		"excluded donor candidates", _candidate_report_lines(excluded), True
	)
	_print_limited(
		"unused exclusion offsets",
		[f"0x{offset:08x}" for offset in unused_exclusions],
		verbose,
	)
	_print_limited(
		"preserved both-real conflicts",
		[
			f"0x{item.file_offset:08x}: kept {item.canonical_name}; donor has {item.donor_name}"
			for item in conflicts
		],
		verbose,
	)
	_print_limited(
		"unsafe source collisions",
		[
			f"{item.path}: {item.old_identifier} -> {item.new_identifier}: {item.reason}"
			for item in collisions
		],
		verbose,
	)


def run(
	ref: str,
	repo_arg: Path,
	apply: bool,
	verbose: bool,
	excluded_offsets: Iterable[int] = (),
) -> int:
	repo = _repo_root(repo_arg.resolve())
	symbols_path = repo / SYMBOLS_PATH
	canonical_raw = symbols_path.read_bytes()
	donor_raw = _run_git(repo, ["show", f"{ref}:{SYMBOLS_PATH.as_posix()}"])
	canonical_entries = _load_json_entries(canonical_raw, str(SYMBOLS_PATH))
	donor_entries = _load_json_entries(donor_raw, f"{ref}:{SYMBOLS_PATH.as_posix()}")
	plan = build_import_plan(canonical_entries, donor_entries)
	selection = select_candidates(plan.candidates, excluded_offsets)

	source_paths = _tracked_source_paths(repo)
	source_texts = {
		path.as_posix(): _read_lossless(repo / path) for path in source_paths
	}
	symbol_names, c_names, mapping_issues = _candidate_maps(selection.candidates)
	collisions = detect_source_collisions(source_texts, selection.candidates)
	# detect_source_collisions includes mapping issues, but retain any future
	# issue type even if the detector implementation changes.
	collisions = list(dict.fromkeys([*mapping_issues, *collisions]))

	print_import_report(
		selection.candidates,
		selection.excluded,
		selection.unused_offsets,
		plan.real_name_conflicts,
		collisions,
		verbose,
	)

	if not selection.candidates:
		print("nothing to import")
		return 0
	if apply and collisions:
		print("refusing --apply because source renames are not collision-safe", file=sys.stderr)
		return 2

	outputs: dict[Path, str] = {}
	canonical_text = canonical_raw.decode("latin-1")
	outputs[SYMBOLS_PATH] = update_symbols_text(canonical_text, selection.candidates)
	for relative_path in EXACT_NAME_JSON_PATHS:
		path = repo / relative_path
		text = _read_lossless(path)
		outputs[relative_path], _ = replace_exact_json_strings(text, symbol_names)
	for relative_path, text in source_texts.items():
		outputs[Path(relative_path)], _ = replace_c_identifiers(text, c_names)

	changed = [
		path for path, new_text in outputs.items()
		if _read_lossless(repo / path) != new_text
	]
	print(f"files that would change: {len(changed)}")
	if verbose:
		for path in changed:
			print(f"  {path.as_posix()}")

	if not apply:
		print("dry-run only; pass --apply to write these changes")
		return 0

	for relative_path in changed:
		_write_lossless(repo / relative_path, outputs[relative_path])
	print(
		f"applied {len(selection.candidates)} symbol-name imports to {len(changed)} files"
	)
	return 0


def main(argv: Sequence[str] | None = None) -> int:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("ref", help="git revision containing config/symbols.json")
	parser.add_argument(
		"--repo", type=Path,
		default=Path(__file__).resolve().parents[2],
		help="repository worktree (default: inferred from this script)",
	)
	parser.add_argument(
		"--apply", action="store_true",
		help="write collision-safe imports (the default is a dry-run)",
	)
	parser.add_argument(
		"--verbose", action="store_true",
		help="show every candidate/conflict and every affected file",
	)
	parser.add_argument(
		"--exclude-offset",
		type=parse_file_offset,
		action="append",
		default=[],
		metavar="OFFSET",
		help=(
			"exclude a donor candidate by decimal or 0x-prefixed file offset; "
			"repeat for multiple offsets"
		),
	)
	arguments = parser.parse_args(argv)
	try:
		return run(
			arguments.ref,
			arguments.repo,
			arguments.apply,
			arguments.verbose,
			arguments.exclude_offset,
		)
	except (ImportErrorDetail, OSError) as error:
		print(f"error: {error}", file=sys.stderr)
		return 2


if __name__ == "__main__":
	raise SystemExit(main())
