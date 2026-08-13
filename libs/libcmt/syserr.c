/*
SYSERR.C

symbols in this file:
	__sys_errlist (.data, 0xB0)
	__sys_nerr (.data, 0x04)
*/

/* ---------- public data */

char *_sys_errlist[] =
{
	"No error",
	"Operation not permitted",
	"No such file or directory",
	"No such process",
	"Interrupted function call",
	"Input/output error",
	"No such device or address",
	"Arg list too long",
	"Exec format error",
	"Bad file descriptor",
	"No child processes",
	"Resource temporarily unavailable",
	"Not enough space",
	"Permission denied",
	"Bad address",
	"Unknown error",
	"Resource device",
	"File exists",
	"Improper link",
	"No such device",
	"Not a directory",
	"Is a directory",
	"Invalid argument",
	"Too many open files in system",
	"Too many open files",
	"Inappropriate I/O control operation",
	"Unknown error",
	"File too large",
	"No space left on device",
	"Invalid seek",
	"Read-only file system",
	"Too many links",
	"Broken pipe",
	"Domain error",
	"Result too large",
	"Unknown error",
	"Resource deadlock avoided",
	"Unknown error",
	"Filename too long",
	"No locks available",
	"Function not implemented",
	"Directory not empty",
	"Illegal byte sequence",
	"Unknown error"
};

int _sys_nerr = sizeof(_sys_errlist) / sizeof(_sys_errlist[0]) - 1;
