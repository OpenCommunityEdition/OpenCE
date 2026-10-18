#ifndef MAC_ENGINE_SHELL_H
#define MAC_ENGINE_SHELL_H

/* Host-owned input to the original shell APIs. The host passes its canonical
   command-line string after process startup and may attach an event pump. */
typedef void (*mac_engine_shell_idle_proc)(void *context);

int mac_engine_shell_set_command_line(const char *command_line);
void mac_engine_shell_set_idle_proc(mac_engine_shell_idle_proc proc, void *context);

#endif
