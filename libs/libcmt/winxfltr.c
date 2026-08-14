/*
WINXFLTR.C

symbols in this file:
	_code_001d40c5 (.text, 0x2c; original name: xcptlookup)
	__XcptFilter (.text, 0x163)
	__XcptActTab (.data, 0x78)
	__First_FPE_Indx (.data, 0x4)
	__Num_FPE (.data, 0x4)
	__XcptActTabSize (.data, 0x4)
	__XcptActTabCount (.data, 0x4)
*/

#include <stddef.h>
#include <signal.h>
#include <float.h>
#include <xtl.h>

/* ---------- types */

typedef void (__cdecl *_PHNDLR)(
	int signal_number);

typedef void (__cdecl *_PHNDLR_FPE)(
	int signal_number,
	int floating_point_error);

struct _XCPT_ACTION
{
	unsigned long XcptNum;
	int SigNum;
	_PHNDLR XcptAction;
};

struct _tiddata
{
	unsigned long _tid;
	unsigned long _thandle;
	int _terrno;
	unsigned long _tdoserrno;
	unsigned int _fpds;
	unsigned long _holdrand;
	char *_token;
	wchar_t *_wtoken;
	unsigned char *_mtoken;
	char *_errmsg;
	wchar_t *_werrmsg;
	char *_namebuf0;
	wchar_t *_wnamebuf0;
	char *_namebuf1;
	wchar_t *_wnamebuf1;
	char *_asctimebuf;
	wchar_t *_wasctimebuf;
	void *_gmtimebuf;
	char *_cvtbuf;
	unsigned char _con_ch_buf[5];
	unsigned short _ch_buf_used;
	struct _XCPT_ACTION *_pxcptacttab;
	PEXCEPTION_POINTERS _tpxcptinfoptrs;
	int _tfpecode;
};

typedef struct _tiddata *_ptiddata;

typedef char _tiddata_xcpt_action_offset[
	offsetof(struct _tiddata, _pxcptacttab) == 0x54 ? 1 : -1];

/* ---------- constants */

#define SIG_DIE ((_PHNDLR)5)

/* ---------- external code */

_ptiddata __cdecl _getptd(
	void);

/* ---------- public data */

struct _XCPT_ACTION _XcptActTab[] =
{
	{ STATUS_ACCESS_VIOLATION, SIGSEGV, SIG_DFL },
	{ STATUS_ILLEGAL_INSTRUCTION, SIGILL, SIG_DFL },
	{ STATUS_PRIVILEGED_INSTRUCTION, SIGILL, SIG_DFL },
	{ STATUS_FLOAT_DENORMAL_OPERAND, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_DIVIDE_BY_ZERO, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_INEXACT_RESULT, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_INVALID_OPERATION, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_OVERFLOW, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_STACK_CHECK, SIGFPE, SIG_DFL },
	{ STATUS_FLOAT_UNDERFLOW, SIGFPE, SIG_DFL }
};

int _First_FPE_Indx = 3;
int _Num_FPE = 7;
int _XcptActTabSize = sizeof(_XcptActTab);
int _XcptActTabCount = sizeof(_XcptActTab) / sizeof(_XcptActTab[0]);

/* ---------- private code */

static struct _XCPT_ACTION *code_001d40c5(
	unsigned long exception_number,
	struct _XCPT_ACTION *exception_action_table)
{
	struct _XCPT_ACTION *exception_action;

	exception_action = exception_action_table;
	do
	{
		if (exception_action->XcptNum == exception_number)
			break;

		exception_action++;
	}
	while (exception_action < exception_action_table + _XcptActTabCount);

	if (exception_action < exception_action_table + _XcptActTabCount &&
		exception_action->XcptNum == exception_number)
	{
		return exception_action;
	}

	return NULL;
}

/* ---------- public code */

int __cdecl _XcptFilter(
	unsigned long exception_number,
	PEXCEPTION_POINTERS exception_pointers)
{
	_ptiddata thread_data;
	struct _XCPT_ACTION *exception_action;
	_PHNDLR exception_handler;
	PEXCEPTION_POINTERS previous_exception_pointers;
	int previous_floating_point_error;
	int exception_index;

	thread_data = _getptd();
	exception_action = code_001d40c5(
		exception_number,
		thread_data->_pxcptacttab);
	if (exception_action == NULL || exception_action->XcptAction == SIG_DFL)
		return UnhandledExceptionFilter(exception_pointers);

	exception_handler = exception_action->XcptAction;
	if (exception_handler == SIG_DIE)
	{
		exception_action->XcptAction = SIG_DFL;
		return EXCEPTION_EXECUTE_HANDLER;
	}

	if (exception_handler == SIG_IGN)
		return EXCEPTION_CONTINUE_EXECUTION;

	previous_exception_pointers = thread_data->_tpxcptinfoptrs;
	thread_data->_tpxcptinfoptrs = exception_pointers;
	if (exception_action->SigNum == SIGFPE)
	{
		for (exception_index = _First_FPE_Indx;
			exception_index < _First_FPE_Indx + _Num_FPE;
			exception_index++)
		{
			thread_data->_pxcptacttab[exception_index].XcptAction = SIG_DFL;
		}

		previous_floating_point_error = thread_data->_tfpecode;
		if (exception_action->XcptNum == STATUS_FLOAT_DIVIDE_BY_ZERO)
		{
			thread_data->_tfpecode = _FPE_ZERODIVIDE;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_INVALID_OPERATION)
		{
			thread_data->_tfpecode = _FPE_INVALID;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_OVERFLOW)
		{
			thread_data->_tfpecode = _FPE_OVERFLOW;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_UNDERFLOW)
		{
			thread_data->_tfpecode = _FPE_UNDERFLOW;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_DENORMAL_OPERAND)
		{
			thread_data->_tfpecode = _FPE_DENORMAL;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_INEXACT_RESULT)
		{
			thread_data->_tfpecode = _FPE_INEXACT;
		}
		else if (exception_action->XcptNum == STATUS_FLOAT_STACK_CHECK)
		{
			thread_data->_tfpecode = _FPE_STACKOVERFLOW;
		}

		((_PHNDLR_FPE)exception_handler)(
			SIGFPE,
			thread_data->_tfpecode);
		thread_data->_tfpecode = previous_floating_point_error;
	}
	else
	{
		exception_action->XcptAction = SIG_DFL;
		exception_handler(exception_action->SigNum);
	}

	thread_data->_tpxcptinfoptrs = previous_exception_pointers;
	return EXCEPTION_CONTINUE_EXECUTION;
}
