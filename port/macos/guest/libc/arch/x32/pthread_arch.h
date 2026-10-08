/* The guest has no thread pointer register of its own (macOS gives user
code no way to set the %fs base, and %gs holds the host's own thread data),
so the runtime hands out each thread's struct pthread
(port/android/guest/runtime/guest_thread.c), as on Android. */
uintptr_t __guest_get_tp(void);

static inline uintptr_t __get_tp()
{
	return __guest_get_tp();
}

#define MC_PC gregs[REG_RIP]
