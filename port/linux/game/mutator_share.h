/* The host's mutators' files sent to its clients (mutator_share.c). */
#ifndef MUTATOR_SHARE_H
#define MUTATOR_SHARE_H

/* whether this client is still finding out or fetching the host's mutators' files:
its game does not load until it is not */
boolean mutator_share_busy(void);
/* whether a client of this host is fetching them */
boolean mutator_share_machine_fetching(long machine_index);
/* whether this client's players wait for that */
boolean mutator_share_holds_players(void);
/* a block of the file being fetched (map_share.c's connection) */
void mutator_share_block(long mutator, long file, unsigned long offset, byte const *bytes, long size);
/* whether a player is being asked about a host's script: the controller's and keys'
presses are the question's, not the menus' or the game's */
boolean mutator_share_prompt_open(void);
/* the question's lines */
void mutator_share_prompt_text(char *question, char *answers, unsigned long size);
void mutator_share_update(void);
boolean mutator_share_handles_message(word const *message, word size);
void mutator_share_handle_message(long machine_index, word const *message, word size);

#endif
