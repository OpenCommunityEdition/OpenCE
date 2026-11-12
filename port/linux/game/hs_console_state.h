/* Console text and syntax must outlive the saved threads that use them. */
#ifndef HS_CONSOLE_STATE_H
#define HS_CONSOLE_STATE_H

struct hs_syntax_node;

void hs_console_state_initialize(void);
void hs_console_state_reset(void);
char *hs_console_source_acquire(long size);
void hs_console_source_finish(char *source, long expression);
boolean hs_console_state_capture(void);
boolean hs_console_state_validate(void const *image, void const *base, unsigned long bytes);
void hs_console_state_restore(void);
boolean hs_console_syntax_node_valid(struct hs_syntax_node const *node);

#endif
