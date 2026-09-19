#ifndef BINK_RADCB_H
#define BINK_RADCB_H

#include "radplatform.h"

/* Descriptive names recovered from the target's lifecycle and callback code.
 * Field names are reconstructed, not asserted to be original RAD spellings.
 */
enum radcb_handler_state
{
	RADCB_HANDLER_NEW= 0,
	RADCB_HANDLER_RUNNING= 1,
	RADCB_HANDLER_SUSPENDED= 2,
	RADCB_HANDLER_STOPPING= 3,
	RADCB_HANDLER_SUSPEND_REQUESTED= 4
};

enum
{
	RADCB_MAXIMUM_WAIT_CALLBACKS= 16,
	RADCB_THREAD_STACK_BYTES= 0x3000,
	RADCB_FREE_EMPTY_HANDLERS= 1
};

struct radcb_callback;
typedef unsigned long (__stdcall *radcb_callback_proc)(
	struct radcb_callback *callback,
	unsigned long iteration);

struct radcb_callback
{
	struct radcb_callback *next;
	HANDLE mutex;
	struct radcb_callback *priority_next;
	unsigned long priority;
	radcb_callback_proc get_priority;
	radcb_callback_proc dispatch;
};

struct radcb_handler
{
	enum radcb_handler_state state;
	unsigned long timeout;
	HANDLE stop_event;
	HANDLE thread;
	unsigned long thread_id;
	HANDLE mutex;
	struct radcb_callback *callbacks;
	unsigned long iteration;
	unsigned long registered_count;
};

unsigned long __stdcall RADCB_registered_count(
	struct radcb_handler *handler);
int __stdcall RADCB_try_to_suspend_handler(
	struct radcb_handler *handler);
void __stdcall RADCB_suspend_handler(
	struct radcb_handler *handler);
void __stdcall RADCB_resume_handler(
	struct radcb_handler *handler);
int __stdcall RADCB_try_to_suspend_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback);
void __stdcall RADCB_suspend_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback);
void __stdcall RADCB_resume_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback);
void __stdcall RADCB_idle_on_callbacks(
	void);

struct radcb_handler *__stdcall RADCB_allocate_handler(
	unsigned long frequency);
void __stdcall RADCB_register_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback,
	radcb_callback_proc get_priority,
	radcb_callback_proc dispatch);
void __stdcall RADCB_suspend_2_handlers(
	struct radcb_handler *first,
	struct radcb_handler *second);
void __stdcall RADCB_free_handler(
	struct radcb_handler *handler);
void __stdcall RADCB_suspend_2_callbacks(
	struct radcb_handler *first_handler,
	struct radcb_callback *first_callback,
	struct radcb_handler *second_handler,
	struct radcb_callback *second_callback);
void __stdcall RADCB_free_2_handlers(
	struct radcb_handler *first,
	struct radcb_handler *second);
int __stdcall RADCB_unregister_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback,
	unsigned long flags);
int __stdcall RADCB_unregister_2_callbacks(
	struct radcb_handler *first,
	struct radcb_callback *first_callback,
	struct radcb_handler *second,
	struct radcb_callback *second_callback,
	unsigned long flags);

#endif
