/*
STREAM.C

symbols in this file:
	__getstream (.text, 0x122)
	_rdata_002b5e00 (.rdata, 0x0c -- SEH scope table)

Reconstruction of the VC7 CRT _getstream().  Built with /O1 /Gy (the CRT's
favour-size settings); /O2 inlines the SEH frame instead of emitting the shared
__SEH_prolog/__SEH_epilog helpers the target uses.

NonMatching: .text is 290/290 bytes with 288/290 raw bytes identical, and is
BYTE-IDENTICAL once relocation fields are masked; .rdata (the 12-byte scope
table) is byte-identical under the same masking.  The single residual is a
relocation *representation* difference that disappears at link time:

	target  .text+0x109: e8 19 01 00 00  + REL32 vs __getstream (addend 0x119)
	ours    .text+0x109: e8 0b 00 00 00  (direct displacement, no relocation)

Both resolve to the same destination (__getstream+0x119 == +0x0b relative).  The
same pattern appears in the scope table: January references the funclet as
__getstream+0x116, whereas our CL emits local label symbols ($L3640 @0x116,
$L3642 @0x119, $T3647 for the scope table) and references them with addend 0.
No source shape controls this -- it is a compiler symbol-emission choice, inert
against 16 flag variants.

PARKED, NOT CREDITED.  The relocation COUNT differs (21 vs 22), i.e. the target
carries a relocation entry at .text+0x10a that this object does not have at all.
That is structurally unlike the semantic-ledger precedents, which are all
same-count cases where only the symbol a relocation NAMES differs; the project's
own tools/audit_semantic_matches.py rejects a count mismatch outright
(relocation_shape_matches).  This object stays NonMatching.
Full analysis: docs/object_matching_logs/libcmt_stream_obj.md
*/

#ifndef _X86_
#define _X86_
#endif
#include <stdarg.h>
#include <excpt.h>
#include <windef.h>
#include <winbase.h>

typedef struct _iobuf {
	char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
} FILE;

typedef struct {
	FILE f;
	CRITICAL_SECTION lock;
} _FILEX;

#define _IOREAD  0x0001
#define _IOWRT   0x0002
#define _IORW    0x0080
#define inuse(s) ((s)->_flag & (_IOREAD | _IOWRT | _IORW))

#define _IOB_SCAN_LOCK  1
#define _STREAM_LOCKS   16
#define _IOB_ENTRIES    20
#define _CRT_SPINCOUNT  4000

extern int _nstream;
extern FILE **__piob;

void __cdecl _lock(int);
void __cdecl _unlock(int);
void __cdecl _lock_file2(int, void *);
void __cdecl _unlock_file2(int, void *);
int __cdecl _mtinitlocknum(int);
int __cdecl __crtInitCritSecAndSpinCount(CRITICAL_SECTION *, unsigned long);
void * __cdecl malloc(unsigned int);
void __cdecl free(void *);

FILE * __cdecl _getstream(void)
{
	FILE *retval = NULL;
	int i;

	_lock(_IOB_SCAN_LOCK);

	__try {

	for (i = 0; i < _nstream; i++) {

		if (__piob[i] != NULL) {

			if (inuse((FILE *)__piob[i]) == 0) {

				if ((i > 2) && (i < _IOB_ENTRIES))
					if (!_mtinitlocknum(i + _STREAM_LOCKS))
						break;
				_lock_file2(i, __piob[i]);
				if (inuse((FILE *)__piob[i]) != 0) {
					_unlock_file2(i, __piob[i]);
				}
				else {
					retval = (FILE *)__piob[i];
					break;
				}
			}
		}
		else {
			if ((__piob[i] = (FILE *)malloc(sizeof(_FILEX))) != NULL) {
				if (!__crtInitCritSecAndSpinCount(&(((_FILEX *)__piob[i])->lock), _CRT_SPINCOUNT)) {
					free(__piob[i]);
					__piob[i] = NULL;
				}
				else {
					EnterCriticalSection(&(((_FILEX *)__piob[i])->lock));
					retval = (FILE *)__piob[i];
				}
			}
			break;
		}
	}

	if (retval != NULL) {
		retval->_flag = retval->_cnt = 0;
		retval->_tmpfname = retval->_ptr = retval->_base = NULL;
		retval->_file = -1;
	}

	}
	__finally {
		_unlock(_IOB_SCAN_LOCK);
	}

	return retval;
}
