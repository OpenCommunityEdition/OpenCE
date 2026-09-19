/* RAD callback control: reconstructed from January 2002 Xbox target code. */
#include "radcb.h"
#include "radmem.h"

/* Original target contributions are in the demand-loaded BINK section.
 * This specifies section ownership; it does not change optimization policy.
 */
#pragma code_seg("BINK")

/* Private names below are descriptive reconstructions. Target ownership is
 * recorded in evidence/radcb_private_owners.json, not hidden by aliases.
 */
static void radcb_wait_for_two_handles(
	HANDLE first,
	HANDLE second)
{
	HANDLE handles[2];
	handles[0]= first;
	handles[1]= second;
	switch (WaitForMultipleObjects(2, handles, FALSE, INFINITE))
	{
	case WAIT_OBJECT_0:
		WaitForSingleObject(second, INFINITE);
		break;
	case WAIT_OBJECT_0+1:
		WaitForSingleObject(first, INFINITE);
		break;
	}
	return;
}

static void radcb_insert_priority_callback(
	struct radcb_callback **head,
	struct radcb_callback *callback)
{
	if (!*head)
	{
		callback->priority_next= 0;
		*head= callback;
	}
	else if (callback->priority >= (*head)->priority)
	{
		callback->priority_next= *head;
		*head= callback;
	}
	else
	{
		struct radcb_callback *current= *head;
		while (current->priority_next)
		{
			if (callback->priority >= current->priority_next->priority)
			{
				callback->priority_next= current->priority_next;
				current->priority_next= callback;
				return;
			}
			current= current->priority_next;
		}
		callback->priority_next= 0;
		current->priority_next= callback;
	}
	return;
}

static void radcb_dispatch_ready_callbacks(
	struct radcb_handler *handler)
{
	HANDLE handles[RADCB_MAXIMUM_WAIT_CALLBACKS];
	struct radcb_callback *callbacks[RADCB_MAXIMUM_WAIT_CALLBACKS];
	handler->iteration++;
	for (;;)
	{
		struct radcb_callback *callback= handler->callbacks;
		struct radcb_callback *priority_head= 0;
		unsigned long count;
		unsigned long ready;
		while (callback)
		{
			if (handler->state != RADCB_HANDLER_RUNNING)
			{
				return;
			}
			callback->priority= callback->get_priority(callback, handler->iteration);
			if (callback->priority)
			{
				radcb_insert_priority_callback(&priority_head, callback);
			}
			callback= callback->next;
		}
		for (count= 0, callback= priority_head;
			callback && count < RADCB_MAXIMUM_WAIT_CALLBACKS;
			callback= callback->priority_next, ++count)
		{
			handles[count]= callback->mutex;
			callbacks[count]= callback;
		}
		if (!count)
		{
			return;
		}
		ready= WaitForMultipleObjects(count, handles, FALSE, 0);
		if (ready < count)
		{
			callbacks[ready]->dispatch(callbacks[ready], handler->iteration);
			ReleaseMutex(handles[ready]);
		}
	}
	return;
}

static DWORD WINAPI radcb_handler_thread(
	void *context)
{
	struct radcb_handler *handler= context;
	while (WaitForSingleObject(handler->stop_event, handler->timeout) != WAIT_OBJECT_0)
	{
		WaitForSingleObject(handler->mutex, INFINITE);
		switch (handler->state)
		{
		case RADCB_HANDLER_RUNNING:
			if (handler->callbacks)
			{
				radcb_dispatch_ready_callbacks(handler);
			}
			break;
		case RADCB_HANDLER_STOPPING:
			ReleaseMutex(handler->mutex);
			return 0;
		}
		ReleaseMutex(handler->mutex);
	}
	return 0;
}

static int radcb_unlink_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback)
{
	if (handler)
	{
		/* The original routine requires a nonempty registration list.
		 * Its target code dereferences the head without an empty-list check.
		 */
		struct radcb_callback *current= handler->callbacks;
		if (current == callback)
		{
			handler->callbacks= current->next;
			return 1;
		}
		while (current->next)
		{
			if (current->next == callback)
			{
				current->next= callback->next;
				return 1;
			}
			current= current->next;
		}
	}
	return 0;
}

static void radcb_request_stop(
	struct radcb_handler *handler)
{
	enum radcb_handler_state previous_state= handler->state;
	SetEvent(handler->stop_event);
	handler->state= RADCB_HANDLER_STOPPING;
	switch (previous_state)
	{
	case RADCB_HANDLER_NEW:
		ResumeThread(handler->thread);
		break;
	case RADCB_HANDLER_SUSPENDED:
		ReleaseMutex(handler->mutex);
		break;
	}
	return;
}

static void radcb_destroy_handler_resources(
	struct radcb_handler *handler)
{
	CloseHandle(handler->mutex);
	CloseHandle(handler->thread);
	CloseHandle(handler->stop_event);
	radfree(handler);
	return;
}


unsigned long __stdcall RADCB_registered_count(
	struct radcb_handler *handler)
{
	return handler->registered_count;
}

int __stdcall RADCB_try_to_suspend_handler(
	struct radcb_handler *handler)
{
	if (WaitForSingleObject(handler->mutex, 0) == WAIT_OBJECT_0)
	{
		handler->state= RADCB_HANDLER_SUSPENDED;
		return 1;
	}
	return 0;
}

void __stdcall RADCB_suspend_handler(
	struct radcb_handler *handler)
{
	handler->state= RADCB_HANDLER_SUSPEND_REQUESTED;
	WaitForSingleObject(handler->mutex, INFINITE);
	handler->state= RADCB_HANDLER_SUSPENDED;
	return;
}

void __stdcall RADCB_resume_handler(
	struct radcb_handler *handler)
{
	if (handler->state == RADCB_HANDLER_NEW)
	{
		handler->state= RADCB_HANDLER_RUNNING;
		ResumeThread(handler->thread);
	}
	else
	{
		handler->state= RADCB_HANDLER_RUNNING;
		ReleaseMutex(handler->mutex);
	}
	return;
}

int __stdcall RADCB_try_to_suspend_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback)
{
	if (callback && callback->mutex)
	{
		return WaitForSingleObject(callback->mutex, 0) == WAIT_OBJECT_0;
	}
	return 1;
}

void __stdcall RADCB_suspend_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback)
{
	if (callback && callback->mutex)
	{
		WaitForSingleObject(callback->mutex, INFINITE);
	}
	return;
}

void __stdcall RADCB_resume_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback)
{
	if (callback && callback->mutex)
	{
		ReleaseMutex(callback->mutex);
	}
	return;
}

void __stdcall RADCB_idle_on_callbacks(
	void)
{
	Sleep(0);
	return;
}

struct radcb_handler *__stdcall RADCB_allocate_handler(
	unsigned long frequency)
{
	struct radcb_handler *handler= radmalloc(sizeof(*handler));
	if (handler)
	{
		/* The original API requires nonzero frequency. */
		handler->state= RADCB_HANDLER_NEW;
		handler->callbacks= 0;
		handler->iteration= 1;
		handler->registered_count= 0;
		handler->timeout= 1000/frequency;
		handler->thread= CreateThread(0, RADCB_THREAD_STACK_BYTES,
			radcb_handler_thread, handler, CREATE_SUSPENDED, &handler->thread_id);
		if (!handler->thread)
		{
			radfree(handler);
			return 0;
		}
		handler->mutex= CreateMutex(0, FALSE, 0);
		handler->stop_event= CreateEvent(0, FALSE, FALSE, 0);
		return handler;
	}
	return 0;
}

void __stdcall RADCB_register_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback,
	radcb_callback_proc get_priority,
	radcb_callback_proc dispatch)
{
	callback->get_priority= get_priority;
	callback->dispatch= dispatch;
	callback->mutex= CreateMutex(0, FALSE, 0);
	callback->next= handler->callbacks;
	handler->callbacks= callback;
	handler->registered_count++;
	return;
}

void __stdcall RADCB_suspend_2_handlers(
	struct radcb_handler *first,
	struct radcb_handler *second)
{
	if (first && second)
	{
		first->state= RADCB_HANDLER_SUSPEND_REQUESTED;
		second->state= RADCB_HANDLER_SUSPEND_REQUESTED;
		radcb_wait_for_two_handles(first->mutex, second->mutex);
		first->state= RADCB_HANDLER_SUSPENDED;
		second->state= RADCB_HANDLER_SUSPENDED;
	}
	else if (first)
	{
		RADCB_suspend_handler(first);
	}
	else if (second)
	{
		RADCB_suspend_handler(second);
	}
	return;
}

void __stdcall RADCB_free_handler(
	struct radcb_handler *handler)
{
	radcb_request_stop(handler);
	WaitForSingleObject(handler->thread, INFINITE);
	radcb_destroy_handler_resources(handler);
	return;
}

void __stdcall RADCB_suspend_2_callbacks(
	struct radcb_handler *first_handler,
	struct radcb_callback *first_callback,
	struct radcb_handler *second_handler,
	struct radcb_callback *second_callback)
{
	radcb_wait_for_two_handles(first_callback->mutex, second_callback->mutex);
	return;
}

void __stdcall RADCB_free_2_handlers(
	struct radcb_handler *first,
	struct radcb_handler *second)
{
	if (!first)
	{
		if (second) RADCB_free_handler(second);
		return;
	}
	if (!second)
	{
		RADCB_free_handler(first);
		return;
	}
	radcb_request_stop(first);
	radcb_request_stop(second);
	radcb_wait_for_two_handles(first->thread, second->thread);
	radcb_destroy_handler_resources(first);
	radcb_destroy_handler_resources(second);
	return;
}

static int radcb_finish_unregistration(
	struct radcb_handler *handler,
	struct radcb_callback *callback,
	unsigned long flags)
{
	int freed;
	handler->registered_count--;
	if ((flags & RADCB_FREE_EMPTY_HANDLERS) && !handler->callbacks)
	{
		RADCB_free_handler(handler);
		freed= 1;
	}
	else
	{
		RADCB_suspend_handler(handler);
		RADCB_resume_handler(handler);
		freed= 0;
	}
	CloseHandle(callback->mutex);
	callback->mutex= 0;
	return freed;
}

int __stdcall RADCB_unregister_callback(
	struct radcb_handler *handler,
	struct radcb_callback *callback,
	unsigned long flags)
{
	if (radcb_unlink_callback(handler, callback))
	{
		return radcb_finish_unregistration(handler, callback, flags);
	}
	return 0;
}

int __stdcall RADCB_unregister_2_callbacks(
	struct radcb_handler *first,
	struct radcb_callback *first_callback,
	struct radcb_handler *second,
	struct radcb_callback *second_callback,
	unsigned long flags)
{
	int removed= 0;
	int freed;
	if (radcb_unlink_callback(first, first_callback)) removed= 1;
	if (radcb_unlink_callback(second, second_callback)) removed |= 2;
	switch (removed)
	{
	case 1:
		return radcb_finish_unregistration(first, first_callback, flags);
	case 2:
		return radcb_finish_unregistration(second, second_callback, flags)<<1;
	case 3:
		first->registered_count--;
		second->registered_count--;
		if (flags & RADCB_FREE_EMPTY_HANDLERS)
		{
			if (!first->callbacks)
			{
				if (!second->callbacks)
				{
					RADCB_free_2_handlers(first, second);
					freed= 3;
				}
				else
				{
					radcb_request_stop(first);
					second->state= RADCB_HANDLER_SUSPEND_REQUESTED;
					radcb_wait_for_two_handles(first->thread, second->mutex);
					radcb_destroy_handler_resources(first);
					second->state= RADCB_HANDLER_SUSPENDED;
					RADCB_resume_handler(second);
					freed= 1;
				}
			}
			else if (!second->callbacks)
			{
				radcb_request_stop(second);
				first->state= RADCB_HANDLER_SUSPEND_REQUESTED;
				radcb_wait_for_two_handles(second->thread, first->mutex);
				radcb_destroy_handler_resources(second);
				first->state= RADCB_HANDLER_SUSPENDED;
				RADCB_resume_handler(first);
				freed= 2;
			}
			else
			{
				goto resume_handlers;
			}
			goto close_callback_mutexes;
		}
resume_handlers:
		RADCB_suspend_2_handlers(first, second);
		RADCB_resume_handler(first);
		RADCB_resume_handler(second);
		freed= 0;
close_callback_mutexes:
		CloseHandle(first_callback->mutex);
		CloseHandle(second_callback->mutex);
		first_callback->mutex= 0;
		second_callback->mutex= 0;
		return freed;
	}
	return 0;
}
