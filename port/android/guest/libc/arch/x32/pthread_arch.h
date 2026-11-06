/* The guest has no thread pointer register of its own (%fs belongs to the
host's C library), so the runtime hands out each thread's struct pthread
(guest/runtime/guest_thread.c). */
uintptr_t __guest_get_tp(void);

static inline uintptr_t __get_tp()
{
	return __guest_get_tp();
}

#define MC_PC gregs[REG_RIP]
