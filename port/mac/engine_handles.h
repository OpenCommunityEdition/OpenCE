#ifndef HALO_MAC_ENGINE_HANDLES_H
#define HALO_MAC_ENGINE_HANDLES_H
/* Native ILP32 clean-XDK entry points; owned threads, unnamed events, files and process-local mutexes. */
void *CreateThread(void *attributes, unsigned long stack_size,
    unsigned long (*entry)(void *), void *argument, unsigned long flags,
    unsigned long *thread_id);
void *CreateMutexA(void *attributes, int initial_owner, const char *name);
int ReleaseMutex(void *handle);
void *CreateEventA(void *attributes, int manual_reset, int initial_state, const char *name);
int SetEvent(void *handle);
int ResetEvent(void *handle);
int CloseHandle(void *handle);
unsigned long ResumeThread(void *handle);
int GetExitCodeThread(void *handle, unsigned long *exit_code);
int SetThreadPriority(void *handle, int priority);
unsigned long WaitForSingleObject(void *handle, unsigned long milliseconds);
unsigned long GetLastError(void);
void SetLastError(unsigned long error);
/* Private file-service leases. Release exactly once per successful acquire.
   A destructor must not reenter handle services. */
void *mac_file_handle_create(void *payload, void (*destroy)(void *));
void *mac_file_handle_acquire(void *handle, void **payload);
void mac_file_handle_release(void *lease);
void *mac_find_handle_create(void *payload, void (*destroy)(void *));
void *mac_find_handle_acquire(void *handle, void **payload);
void mac_find_handle_release(void *lease);
int mac_find_handle_close(void *handle);
/* Distinct service namespaces prevent casting an unrelated opaque payload.
   Save/nickname enumeration closes with FindClose; signatures close only through
   the matching private service close. Leases use mac_file_handle_release. */
enum mac_service_handle_kind {
    MAC_HANDLE_SAVE_FIND=6, MAC_HANDLE_NICKNAME_FIND=7, MAC_HANDLE_SIGNATURE=8
};
void *mac_service_handle_create(int kind,void *payload,void (*destroy)(void *));
void *mac_service_handle_acquire(void *handle,int kind,void **payload);
int mac_service_handle_close(void *handle,int kind);
#endif
