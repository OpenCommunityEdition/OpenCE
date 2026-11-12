/* Per-profile MCC completion history; never uses Xbox campaign indices. */
#ifndef MCC_PROGRESS_H
#define MCC_PROGRESS_H
unsigned mcc_progress_flags(long profile, char const *level);
unsigned mcc_progress_menu_flags(long profile, char const *level, unsigned long now);
void mcc_progress_complete(char const *level, short difficulty);
boolean mcc_progress_copy(char const *profile_file, char const *new_directory);
#endif
