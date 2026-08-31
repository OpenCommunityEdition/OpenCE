"""Find source constructs that deserve a fake-match or logic review.

This is a conservative lexical review aid, not an admission gate or a proof of
bad source.  It deliberately reports the location and the reason for review;
the January object, first-party names, credible source precedent, and program
semantics remain authoritative.

By default the scanner walks ``source`` and skips known vendored trees.  Paths
may name individual files or directories.

Exit codes:
	0  scan completed (including scans with findings)
	1  findings were present and --fail-on-findings was requested
	2  invalid input or an unreadable source file
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = frozenset((".c", ".cc", ".cpp", ".h", ".hh", ".hpp", ".inl"))
IGNORED_DIRECTORY_NAMES = frozenset((
	".git",
	".pytest_cache",
	"__pycache__",
	"build",
	"build_debug",
	"dist",
	"node_modules",
	"projects",
	"scratch",
))
VENDORED_DIRECTORY_NAMES = frozenset(("third_party", "vendor", "vendored"))
VENDORED_PATH_SUFFIXES = (
	("source", "bitmaps", "libtiff"),
	("source", "memory", "zlib"),
)
DISCLAIMER = (
	"Review leads only: a finding does not prove fake source, a semantic bug, "
	"or an invalid byte match."
)


@dataclass(frozen=True)
class Rule:
	rule_id: str
	priority: str
	message: str


RULES = (
	Rule(
		"self-assignment",
		"high",
		"an ordinary self-assignment is inert; verify volatile/alias semantics and provenance",
	),
	Rule(
		"goto-next-label",
		"high",
		"the jump targets the immediately following label; verify that this is not a codegen nudge",
	),
	Rule(
		"empty-if",
		"normal",
		"the body is empty and there is no else; only condition side effects can remain",
	),
	Rule(
		"empty-then-else",
		"normal",
		"the true arm is empty; this can be legitimate CFG shape, so verify both semantics and target evidence",
	),
	Rule(
		"empty-else",
		"normal",
		"the else arm is empty; verify why the polarity is expressed this way",
	),
	Rule(
		"identical-branches",
		"high",
		"both branches have the same token sequence; the condition appears irrelevant",
	),
	Rule(
		"fixed-boolean-condition",
		"high",
		"a simple value and its negation make this condition fixed if the read is stable",
	),
	Rule(
		"self-order-comparison",
		"high",
		"a simple value is strictly ordered against itself, so the comparison appears always false",
	),
	Rule(
		"volatile-reinterpret-cast",
		"high",
		"a volatile address reinterpretation can force loads or lifetimes; require real semantic provenance",
	),
	Rule(
		"inline-assembly",
		"normal",
		"inline assembly needs first-party or target-shape justification; low-level/math helpers may be legitimate",
	),
	Rule(
		"raw-byte-emission",
		"high",
		"raw instruction-byte emission is exceptionally unlikely to be credible reconstruction source",
	),
	Rule(
		"codegen-directive",
		"normal",
		"an explicit optimization/inlining directive may be steering bytes rather than expressing program intent",
	),
)
RULE_BY_ID = {rule.rule_id: rule for rule in RULES}
RULE_ORDER = {rule.rule_id: index for index, rule in enumerate(RULES)}


@dataclass(frozen=True)
class Token:
	value: str
	start: int
	end: int
	line: int
	column: int


@dataclass(frozen=True)
class Finding:
	path: str
	line: int
	column: int
	rule: str
	priority: str
	message: str
	snippet: str


@dataclass(frozen=True)
class ScanSummary:
	files_scanned: int
	findings: tuple[Finding, ...]
	skipped_vendored: tuple[str, ...]


TOKEN_RE = re.compile(
	r"[A-Za-z_]\w*"
	r"|0[xX][0-9A-Fa-f]+"
	r"|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?[A-Za-z]*"
	r"|->|==|!=|<=|>=|&&|\|\||\+\+|--|<<|>>|\+=|-=|\*=|/=|%=|&=|\|=|\^="
	r"|[{}()\[\];:,.!*&+\-/%<>=?|~^#]"
)


def _mask_comments_and_literals(text: str) -> str:
	"""Replace comments and quoted literals with spaces while retaining lines."""
	result = list(text)
	index = 0
	state = "code"
	while index < len(text):
		character = text[index]
		next_character = text[index + 1] if index + 1 < len(text) else ""

		if state == "code":
			if character == "/" and next_character == "/":
				result[index] = result[index + 1] = " "
				index += 2
				state = "line-comment"
				continue
			if character == "/" and next_character == "*":
				result[index] = result[index + 1] = " "
				index += 2
				state = "block-comment"
				continue
			if character == '"':
				result[index] = " "
				index += 1
				state = "string"
				continue
			if character == "'":
				result[index] = " "
				index += 1
				state = "character"
				continue
			index += 1
			continue

		if state == "line-comment":
			if character == "\n":
				state = "code"
			else:
				result[index] = " "
			index += 1
			continue

		if state == "block-comment":
			if character == "*" and next_character == "/":
				result[index] = result[index + 1] = " "
				index += 2
				state = "code"
				continue
			if character != "\n":
				result[index] = " "
			index += 1
			continue

		if character == "\\" and next_character:
			result[index] = " "
			if next_character != "\n":
				result[index + 1] = " "
			index += 2
			continue
		if (state == "string" and character == '"') or (
			state == "character" and character == "'"
		):
			result[index] = " "
			index += 1
			state = "code"
			continue
		if character != "\n":
			result[index] = " "
		index += 1

	return "".join(result)


def _line_starts(text: str) -> list[int]:
	return [0] + [match.end() for match in re.finditer("\n", text)]


def _line_and_column(starts: Sequence[int], offset: int) -> tuple[int, int]:
	line_index = bisect.bisect_right(starts, offset) - 1
	return line_index + 1, offset - starts[line_index] + 1


def _tokens(masked: str) -> list[Token]:
	starts = _line_starts(masked)
	result = []
	for match in TOKEN_RE.finditer(masked):
		line, column = _line_and_column(starts, match.start())
		result.append(Token(match.group(0), match.start(), match.end(), line, column))
	return result


def _matching_tokens(tokens: Sequence[Token], opener: str, closer: str) -> dict[int, int]:
	stack = []
	pairs = {}
	for index, token in enumerate(tokens):
		if token.value == opener:
			stack.append(index)
		elif token.value == closer and stack:
			opening_index = stack.pop()
			pairs[opening_index] = index
	return pairs


def _strip_outer_parentheses(values: Sequence[str]) -> tuple[str, ...]:
	result = tuple(values)
	while len(result) >= 2 and result[0] == "(" and result[-1] == ")":
		depth = 0
		closes_at_end = False
		for index, value in enumerate(result):
			if value == "(":
				depth += 1
			elif value == ")":
				depth -= 1
				if depth == 0:
					closes_at_end = index == len(result) - 1
					break
		if not closes_at_end:
			break
		result = result[1:-1]
	return result


def _simple_value(values: Sequence[str]) -> tuple[str, ...] | None:
	values = _strip_outer_parentheses(values)
	if not values or not re.fullmatch(r"[A-Za-z_]\w*", values[0]):
		return None
	index = 1
	while index < len(values):
		if index + 1 >= len(values) or values[index] not in (".", "->"):
			return None
		if not re.fullmatch(r"[A-Za-z_]\w*", values[index + 1]):
			return None
		index += 2
	return tuple(values)


def _simple_lvalue(values: Sequence[str]) -> tuple[str, ...] | None:
	"""Recognize a side-effect-free lvalue spelling for self-assignment checks."""
	values = _strip_outer_parentheses(values)
	if not values:
		return None
	index = 0
	while index < len(values) and values[index] == "*":
		index += 1
	if index >= len(values) or not re.fullmatch(r"[A-Za-z_]\w*", values[index]):
		return None
	index += 1
	while index < len(values):
		if values[index] in (".", "->"):
			if index + 1 >= len(values) or not re.fullmatch(r"[A-Za-z_]\w*", values[index + 1]):
				return None
			index += 2
			continue
		if values[index] == "[":
			depth = 1
			index += 1
			content_start = index
			while index < len(values) and depth:
				if values[index] == "[":
					depth += 1
				elif values[index] == "]":
					depth -= 1
				index += 1
			if depth or index == content_start + 1:
				return None
			content = values[content_start:index - 1]
			if any(value in ("++", "--", "=", "(", ")") for value in content):
				return None
			continue
		return None
	return tuple(values)


def _semantic_fingerprint(text: str) -> tuple[str, ...]:
	"""Tokenize code while retaining literal contents and ignoring comments."""
	result = []
	index = 0
	while index < len(text):
		if text[index].isspace():
			index += 1
			continue
		if text.startswith("//", index):
			newline = text.find("\n", index + 2)
			index = len(text) if newline < 0 else newline + 1
			continue
		if text.startswith("/*", index):
			closing = text.find("*/", index + 2)
			index = len(text) if closing < 0 else closing + 2
			continue
		if text[index] in ('"', "'"):
			quote = text[index]
			start = index
			index += 1
			while index < len(text):
				if text[index] == "\\" and index + 1 < len(text):
					index += 2
					continue
				if text[index] == quote:
					index += 1
					break
				index += 1
			result.append(text[start:index])
			continue
		match = TOKEN_RE.match(text, index)
		if match:
			result.append(match.group(0))
			index = match.end()
			continue
		result.append(text[index])
		index += 1
	return tuple(result)


def _top_level_operator(values: Sequence[str], operators: frozenset[str]) -> tuple[int, str] | None:
	depth = 0
	match = None
	for index, value in enumerate(values):
		if value in ("(", "["):
			depth += 1
		elif value in (")", "]"):
			depth -= 1
		elif depth == 0 and value in operators:
			if match is not None:
				return None
			match = (index, value)
	return match


def _fixed_boolean_condition(values: Sequence[str]) -> bool:
	values = _strip_outer_parentheses(values)
	operator = _top_level_operator(values, frozenset(("&&", "||")))
	if operator is None:
		return False
	index, _ = operator
	left = _strip_outer_parentheses(values[:index])
	right = _strip_outer_parentheses(values[index + 1:])
	if left[:1] == ("!",):
		left_value = _simple_value(left[1:])
		right_value = _simple_value(right)
		return left_value is not None and left_value == right_value
	if right[:1] == ("!",):
		right_value = _simple_value(right[1:])
		left_value = _simple_value(left)
		return right_value is not None and right_value == left_value
	return False


def _self_order_comparison(values: Sequence[str]) -> bool:
	values = _strip_outer_parentheses(values)
	operator = _top_level_operator(values, frozenset(("<", ">")))
	if operator is None:
		return False
	index, _ = operator
	left = _simple_value(values[:index])
	right = _simple_value(values[index + 1:])
	return left is not None and left == right


def _snippet(text: str, line: int) -> str:
	lines = text.splitlines()
	if not 1 <= line <= len(lines):
		return ""
	value = " ".join(lines[line - 1].strip().split())
	return value if len(value) <= 160 else value[:157] + "..."


def _finding(path: str, text: str, token: Token, rule_id: str) -> Finding:
	rule = RULE_BY_ID[rule_id]
	return Finding(
		path=path,
		line=token.line,
		column=token.column,
		rule=rule.rule_id,
		priority=rule.priority,
		message=rule.message,
		snippet=_snippet(text, token.line),
	)


def scan_text(text: str, path: str = "<memory>") -> list[Finding]:
	"""Return deterministic review leads for one C-family source string."""
	masked = _mask_comments_and_literals(text)
	tokens = _tokens(masked)
	parentheses = _matching_tokens(tokens, "(", ")")
	braces = _matching_tokens(tokens, "{", "}")
	findings = []

	for index, token in enumerate(tokens):
		if token.value != "if" or index + 1 >= len(tokens) or tokens[index + 1].value != "(":
			continue
		condition_end = parentheses.get(index + 1)
		if condition_end is None:
			continue

		condition = [item.value for item in tokens[index + 2:condition_end]]
		if _fixed_boolean_condition(condition):
			findings.append(_finding(path, text, token, "fixed-boolean-condition"))
		if _self_order_comparison(condition):
			findings.append(_finding(path, text, token, "self-order-comparison"))

		body_start = condition_end + 1
		if body_start >= len(tokens):
			continue
		if tokens[body_start].value == "{":
			body_end = braces.get(body_start)
			if body_end is None:
				continue
			then_empty = body_end == body_start + 1
			then_values = _semantic_fingerprint(
				text[tokens[body_start].end:tokens[body_end].start]
			)
		elif tokens[body_start].value == ";":
			body_end = body_start
			then_empty = True
			then_values = ()
		else:
			continue

		after_body = body_end + 1
		has_else = after_body < len(tokens) and tokens[after_body].value == "else"
		if not has_else:
			if then_empty:
				findings.append(_finding(path, text, token, "empty-if"))
			continue

		else_start = after_body + 1
		else_is_block = else_start < len(tokens) and tokens[else_start].value == "{"
		else_end = braces.get(else_start) if else_is_block else None
		else_empty = else_end == else_start + 1 if else_end is not None else False
		if then_empty:
			findings.append(_finding(path, text, token, "empty-then-else"))
		elif else_empty:
			findings.append(_finding(path, text, token, "empty-else"))
		elif else_end is not None:
			else_values = _semantic_fingerprint(
				text[tokens[else_start].end:tokens[else_end].start]
			)
			if then_values == else_values:
				findings.append(_finding(path, text, token, "identical-branches"))

	for index, token in enumerate(tokens):
		if token.value == "goto" and index + 4 < len(tokens):
			label = tokens[index + 1].value
			if (
				re.fullmatch(r"[A-Za-z_]\w*", label)
				and tokens[index + 2].value == ";"
				and tokens[index + 3].value == label
				and tokens[index + 4].value == ":"
			):
				findings.append(_finding(path, text, token, "goto-next-label"))

		if token.value != "=":
			continue
		statement_end = index + 1
		while statement_end < len(tokens) and tokens[statement_end].value not in (";", "{", "}"):
			statement_end += 1
		if statement_end >= len(tokens) or tokens[statement_end].value != ";":
			continue
		right = _strip_outer_parentheses(
			[item.value for item in tokens[index + 1:statement_end]]
		)
		if not right or index < len(right):
			continue
		left_start = index - len(right)
		left = tuple(item.value for item in tokens[left_start:index])
		preceding = tokens[left_start - 1].value if left_start else None
		left_is_complete = preceding is None or preceding in ("{", "}", ";", ":", "(", ")")
		if left_is_complete and left == right and _simple_lvalue(right) is not None:
			findings.append(_finding(path, text, tokens[left_start], "self-assignment"))

	regex_rules = (
		(
			"volatile-reinterpret-cast",
			re.compile(r"\*\s*\(\s*[^()\n;]*\bvolatile\b[^()\n;]*\*\s*\)\s*&"),
		),
		("inline-assembly", re.compile(r"\b(?:__asm|_asm|__asm__)\b")),
		("raw-byte-emission", re.compile(r"\b(?:__emit|_emit)\b")),
		(
			"codegen-directive",
			re.compile(
				r"(?im)^[ \t]*#[ \t]*pragma[ \t]+(?:optimize|inline_depth|inline_recursion|auto_inline)\b"
				r"|\b__forceinline\b"
				r"|\b__declspec\s*\(\s*(?:noinline|naked)\s*\)"
			),
		),
	)
	starts = _line_starts(masked)
	for rule_id, pattern in regex_rules:
		for match in pattern.finditer(masked):
			line, column = _line_and_column(starts, match.start())
			findings.append(_finding(
				path,
				text,
				Token(match.group(0), match.start(), match.end(), line, column),
				rule_id,
			))

	deduplicated = {
		(finding.path, finding.line, finding.column, finding.rule): finding
		for finding in findings
	}
	return sorted(
		deduplicated.values(),
		key=lambda finding: (
			finding.path.casefold(),
			finding.line,
			finding.column,
			RULE_ORDER[finding.rule],
		),
	)


def _contains_path_sequence(parts: Sequence[str], sequence: Sequence[str]) -> bool:
	if len(parts) < len(sequence):
		return False
	return any(
		tuple(parts[index:index + len(sequence)]) == tuple(sequence)
		for index in range(len(parts) - len(sequence) + 1)
	)


def _is_vendored(path: Path) -> bool:
	parts = tuple(part.casefold() for part in path.parts)
	if any(part in VENDORED_DIRECTORY_NAMES for part in parts):
		return True
	return any(_contains_path_sequence(parts, suffix) for suffix in VENDORED_PATH_SUFFIXES)


def _display_path(path: Path) -> str:
	resolved = path.resolve()
	for root in (REPOSITORY_ROOT, Path.cwd().resolve()):
		try:
			return resolved.relative_to(root).as_posix()
		except ValueError:
			pass
	return resolved.as_posix()


def _collect_files(
	paths: Iterable[Path],
	include_vendored: bool,
) -> tuple[list[Path], tuple[str, ...], list[str]]:
	files = {}
	skipped_vendored = set()
	errors = []

	for input_path in paths:
		path = input_path.expanduser()
		if not path.exists():
			errors.append("path does not exist: %s" % input_path)
			continue
		if path.is_file():
			if path.suffix.casefold() not in SOURCE_SUFFIXES:
				errors.append("not a supported C-family source file: %s" % input_path)
				continue
			if _is_vendored(path) and not include_vendored:
				skipped_vendored.add(_display_path(path))
				continue
			files[str(path.resolve()).casefold()] = path.resolve()
			continue
		if not path.is_dir():
			errors.append("not a regular file or directory: %s" % input_path)
			continue
		if _is_vendored(path) and not include_vendored:
			skipped_vendored.add(_display_path(path))
			continue

		for root, directory_names, file_names in os.walk(path):
			root_path = Path(root)
			if root_path != path and _is_vendored(root_path) and not include_vendored:
				skipped_vendored.add(_display_path(root_path))
				directory_names[:] = []
				continue

			kept_directories = []
			for directory_name in sorted(directory_names, key=str.casefold):
				child = root_path / directory_name
				if directory_name.casefold() in IGNORED_DIRECTORY_NAMES:
					continue
				if _is_vendored(child) and not include_vendored:
					skipped_vendored.add(_display_path(child))
					continue
				kept_directories.append(directory_name)
			directory_names[:] = kept_directories

			for file_name in sorted(file_names, key=str.casefold):
				candidate = root_path / file_name
				if candidate.suffix.casefold() not in SOURCE_SUFFIXES:
					continue
				if _is_vendored(candidate) and not include_vendored:
					skipped_vendored.add(_display_path(candidate))
					continue
				files[str(candidate.resolve()).casefold()] = candidate.resolve()

	return (
		sorted(files.values(), key=lambda value: _display_path(value).casefold()),
		tuple(sorted(skipped_vendored, key=str.casefold)),
		sorted(errors, key=str.casefold),
	)


def scan_paths(
	paths: Iterable[Path],
	include_vendored: bool = False,
	rule_ids: frozenset[str] | None = None,
) -> tuple[ScanSummary, list[str]]:
	"""Scan files/directories and return the summary plus deterministic errors."""
	files, skipped_vendored, errors = _collect_files(paths, include_vendored)
	findings = []
	files_scanned = 0
	for path in files:
		try:
			data = path.read_bytes()
		except OSError as error:
			errors.append("could not read %s: %s" % (_display_path(path), error))
			continue
		try:
			text = data.decode("utf-8-sig")
		except UnicodeDecodeError:
			text = data.decode("latin-1")
		files_scanned += 1
		for finding in scan_text(text, _display_path(path)):
			if rule_ids is None or finding.rule in rule_ids:
				findings.append(finding)

	findings.sort(key=lambda finding: (
		finding.path.casefold(),
		finding.line,
		finding.column,
		RULE_ORDER[finding.rule],
	))
	return ScanSummary(files_scanned, tuple(findings), skipped_vendored), sorted(errors)


def _print_text(summary: ScanSummary) -> None:
	print(DISCLAIMER)
	for finding in summary.findings:
		print(
			"%s:%d:%d: [%s] %s: %s"
			% (
				finding.path,
				finding.line,
				finding.column,
				finding.priority,
				finding.rule,
				finding.message,
			)
		)
		if finding.snippet:
			print("    %s" % finding.snippet)
	print(
		"Scanned %d file(s); %d review lead(s); skipped %d vendored path(s)."
		% (summary.files_scanned, len(summary.findings), len(summary.skipped_vendored))
	)


def _print_json(summary: ScanSummary) -> None:
	payload = {
		"disclaimer": DISCLAIMER,
		"files_scanned": summary.files_scanned,
		"findings": [asdict(finding) for finding in summary.findings],
		"skipped_vendored": list(summary.skipped_vendored),
	}
	print(json.dumps(payload, indent=2, sort_keys=True))


def _parser() -> argparse.ArgumentParser:
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument(
		"paths",
		nargs="*",
		type=Path,
		help="C-family file or directory (default: repository source/)",
	)
	parser.add_argument(
		"--format",
		choices=("text", "json"),
		default="text",
		help="deterministic output format (default: text)",
	)
	parser.add_argument(
		"--fail-on-findings",
		action="store_true",
		help="return exit code 1 when one or more review leads are found",
	)
	parser.add_argument(
		"--include-vendored",
		action="store_true",
		help="scan known vendored trees such as libtiff and zlib",
	)
	parser.add_argument(
		"--rule",
		action="append",
		choices=tuple(rule.rule_id for rule in RULES),
		help="report only this rule (repeatable)",
	)
	parser.add_argument(
		"--list-rules",
		action="store_true",
		help="list rule identifiers and exit",
	)
	return parser


def main(argv: Sequence[str] | None = None) -> int:
	arguments = _parser().parse_args(argv)
	if arguments.list_rules:
		for rule in RULES:
			print("%-27s %-6s %s" % (rule.rule_id, rule.priority, rule.message))
		return 0

	paths = arguments.paths or [REPOSITORY_ROOT / "source"]
	rule_ids = frozenset(arguments.rule) if arguments.rule else None
	summary, errors = scan_paths(paths, arguments.include_vendored, rule_ids)
	if errors:
		for error in errors:
			print("error: %s" % error, file=sys.stderr)
		return 2

	if arguments.format == "json":
		_print_json(summary)
	else:
		_print_text(summary)
	if arguments.fail_on_findings and summary.findings:
		return 1
	return 0


if __name__ == "__main__":
	sys.exit(main())
