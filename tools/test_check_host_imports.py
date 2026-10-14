"""Tests of tools/check_host_imports.py: the repository's import lists pass,
and each kind of problem it looks for is found."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_host_imports as checker  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


def problems_of(declaration, extra_lists=None, other=""):
    """The problems with one list holding the declared names."""
    prototypes = checker.parse_prototypes(declaration, "guest.h")
    names = [prototype.name for prototype in prototypes if prototype.name.startswith("host_")]
    lists = {"imports.list": names, **(extra_lists or {})}
    return checker.check(lists, {"guest.h": prototypes}, checker.parse_prototypes(other, "other.c"), [])


def test_repository_passes():
    assert checker.check_repository(ROOT) == []


def test_parse_prototypes():
    text = """
/* host_commented(int a); */
int host_one(unsigned int a, const char *b);
void host_two(void) __attribute__((noreturn));
long long host_three(long long number,
	long long a);
static int host_static(int a);
int host_defined(int a)
{
	host_call(a);
	return 0;
}
"""
    prototypes = checker.parse_prototypes(text, "x.c")
    assert [p.name for p in prototypes] == ["host_one", "host_two", "host_three"]
    assert prototypes[0].parameters == ["unsigned int a", "const char *b"]
    assert prototypes[1].parameters == []
    assert prototypes[2].where == "x.c:5"


def test_classify():
    assert checker.classify("const char *name")[0] == "pointer"
    assert checker.classify("void (*callback)(void *)")[0] == "pointer"
    assert checker.classify("int values[4]")[0] == "pointer"
    assert checker.classify("unsigned int size")[0] == "int32"
    assert checker.classify("unsigned int")[0] == "int32"
    assert checker.classify("posix_ulong size")[0] == "int32"
    assert checker.classify("long long a")[0] == "int64"
    assert checker.classify("unsigned long long")[0] == "int64"
    assert checker.classify("float x")[0] == "float"
    assert checker.classify("enum mode m")[0] == "int32"
    assert checker.classify("long a")[0] == "bad"
    assert checker.classify("size_t size")[0] == "bad"
    assert checker.classify("struct pose pose")[0] == "bad"
    assert checker.classify("...")[0] == "bad"
    assert checker.classify("mystery_t m")[0] == "bad"


def test_pointer_after_the_sixth_argument():
    # the message box as it was: two pointers on x86-64's stack
    problems = problems_of("int host_sdl_show_message_box(unsigned int flags, const char *title, "
                           "const char *message, int button_count, const int *button_flags, const int *button_ids, "
                           "const char *button_texts, int *chosen);")
    assert len(problems) == 2
    assert "argument 7" in problems[0] and "argument 8" in problems[1]


def test_scalars_after_the_sixth_argument():
    # 64-bit and 32-bit integers on the stack cross (host_syscall, posix_socket_select)
    assert problems_of("long long host_syscall(long long number, long long a, long long b, long long c, "
                       "long long d, long long e, long long f);") == []
    assert problems_of("int host_select(int *a, int *b, int *c, int *d, int *e, int *f, int g, int h, int i);") == []
    # floats have registers of their own
    assert problems_of("int host_f(int a, int b, int c, int d, int e, int f, float x, float y, const char *z);") \
        and "argument 9" in problems_of("int host_f(int a, int b, int c, int d, int e, int f, float x, float y, "
                                        "const char *z);")[0]


def test_too_many_arguments():
    problems = problems_of("int host_many(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j);")
    assert len(problems) == 1 and "at most 9" in problems[0]
    floats = ", ".join(f"float f{index}" for index in range(9))
    problems = problems_of(f"void host_floats({floats});")
    assert len(problems) == 1 and "floating-point" in problems[0]


def test_bad_types():
    assert "variadic" in problems_of("int host_printf(const char *format, ...);")[0]
    assert "by value" in problems_of("int host_pose(struct pose pose);")[0]
    assert "differs" in problems_of("int host_size(size_t size);")[0]
    assert "returns a long" in problems_of("long host_long(void);")[0]
    assert "by value" in problems_of("struct pose host_get_pose(void);")[0]


def test_lists_and_declarations_in_step():
    prototypes = checker.parse_prototypes("int host_a(int a);\nint host_b(int b);\n", "guest.h")
    # listed but not declared, declared but not listed
    problems = checker.check({"imports.list": ["host_a", "host_c"]}, {"guest.h": prototypes}, [], [])
    assert any("host_c has no declaration" in problem for problem in problems)
    assert any("host_b is declared but in no import list" in problem for problem in problems)
    # twice in a list, and in two lists
    problems = checker.check({"one.list": ["host_a", "host_a"], "two.list": ["host_b", "host_a"]},
                             {"guest.h": prototypes}, [], [])
    assert any("listed twice" in problem for problem in problems)
    assert any("also in another list" in problem for problem in problems)


def test_declarations_agree():
    problems = problems_of("int host_a(unsigned int a, const char *b);",
                           other="int host_a(unsigned int a, unsigned int b);\n")
    assert len(problems) == 1 and "differ" in problems[0]
    assert problems_of("int host_a(unsigned int a, const char *b);",
                       other="int host_a(unsigned int size, const char *name);\n") == []


def test_posix():
    prototypes = checker.parse_prototypes("int posix_bad(int a, int b, int c, int d, int e, int f, void *g);",
                                          "posix.h")
    problems = checker.check({}, {}, [], prototypes)
    assert len(problems) == 1 and "hostposix_bad" in problems[0]


def test_command_line(tmp_path):
    stamp = tmp_path / "checked"
    assert checker.main(["--root", str(ROOT), "--stamp", str(stamp)]) == 0
    assert stamp.is_file()
