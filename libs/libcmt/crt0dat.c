/*
CRT0DAT.C

symbols in this file:
	__lockexit (.text, 0x9)
	__unlockexit (.text, 0x9)
	__initterm (.text, 0x18)
	_doexit (.text, 0xaa)
	_exit (.text, 0x12)
	__exit (.text, 0x12)
	__cexit (.text, 0x11)
	__c_exit (.text, 0x10)
	__umaskval (.bss, 0x4)
	__exitflag (.bss, 0x1)
	__C_Termination_Done (.bss, 0x4)
	__C_Exit_Done (.bss, 0x4)
*/

/* ---------- constants */

#define _EXIT_LOCK 8

/* ---------- types */

typedef void (__cdecl *_PVFV)(
	void);

/* ---------- external data */

extern _PVFV *__onexitbegin;
extern _PVFV *__onexitend;

extern _PVFV __xp_a[];
extern _PVFV __xp_z[];
extern _PVFV __xt_a[];
extern _PVFV __xt_z[];

/* ---------- external code */

void __cdecl _lock(
	int lock_number);

void __cdecl _unlock(
	int lock_number);

__declspec(dllimport) void __stdcall KeBugCheck(
	unsigned long bug_check_code);

/* ---------- public data */

int _umaskval = 0;
char _exitflag = 0;
int _C_Termination_Done = 0;
int _C_Exit_Done = 0;

/* ---------- public code */

void __cdecl _lockexit(
	void)
{
	_lock(_EXIT_LOCK);
}

void __cdecl _unlockexit(
	void)
{
	_unlock(_EXIT_LOCK);
}

static void __cdecl _initterm(
	_PVFV *first,
	_PVFV *last)
{
	while (first < last)
	{
		if (*first)
		{
			(**first)();
		}

		first++;
	}
}

static void __cdecl doexit(
	int code,
	int quick,
	int retcaller)
{
	_lock(_EXIT_LOCK);
	_C_Termination_Done = 1;
	_exitflag = (char)retcaller;

	if (!quick)
	{
		if (__onexitbegin)
		{
			while (--__onexitend >= __onexitbegin)
			{
				if (*__onexitend)
				{
					(**__onexitend)();
				}
			}
		}

		_initterm(__xp_a, __xp_z);
	}

	_initterm(__xt_a, __xt_z);

	if (retcaller)
	{
		_unlock(_EXIT_LOCK);
		return;
	}

	KeBugCheck(0);
}

void __cdecl exit(
	int code)
{
	doexit(code, 0, 0);
}

void __cdecl _exit(
	int code)
{
	doexit(code, 1, 0);
}

void __cdecl _cexit(
	void)
{
	doexit(0, 0, 1);
}

void __cdecl _c_exit(
	void)
{
	doexit(0, 1, 1);
}
