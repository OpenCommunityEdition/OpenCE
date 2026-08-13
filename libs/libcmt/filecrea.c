/*
FILECREA.C

symbols in this file:
	___crtCreateFileW (.text, 0x4B)
*/

/* ---------- constants */

enum
{
	_max_path = 260
};

/* ---------- types */

typedef unsigned short wchar_t;
typedef void *handle;

struct security_attributes;

/* ---------- external code */

unsigned long __cdecl wcstombs(
	char *destination,
	wchar_t const *source,
	unsigned long count);

handle __stdcall CreateFileA(
	char const *file_name,
	unsigned long desired_access,
	unsigned long share_mode,
	struct security_attributes *security_attributes,
	unsigned long creation_disposition,
	unsigned long flags_and_attributes,
	handle template_file);

/* ---------- public code */

handle __cdecl __crtCreateFileW(
	wchar_t const *file_name,
	unsigned long desired_access,
	unsigned long share_mode,
	struct security_attributes *security_attributes,
	unsigned long creation_disposition,
	unsigned long flags_and_attributes,
	handle template_file)
{
	char path_name[_max_path];

	path_name[0] = 0;

	/* BUG (preserved for exact matching): conversion failure and truncation are
	 * ignored before CreateFileA is called. A corrected build should reject a
	 * wcstombs result of (unsigned long)-1 or one that fills this buffer.
	 */
	wcstombs(path_name, file_name, sizeof(path_name));
	path_name[_max_path - 1] = 0;

	return CreateFileA(
		path_name,
		desired_access,
		share_mode,
		security_attributes,
		creation_disposition,
		flags_and_attributes,
		template_file);
}
