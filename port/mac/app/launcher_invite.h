/*
LAUNCHER_INVITE.H

A halo:// invite link, written where the game reads one.

The link reaches the launcher as an Apple event (opence_launcher.c); what
to do with it afterwards is here, apart from Carbon, so that it can be
tested on any computer (test_launcher_invite.c).
*/

#ifndef OPENCE_LAUNCHER_INVITE_H
#define OPENCE_LAUNCHER_INVITE_H

/* Writes link as <data_root>/join_link.txt, which is where the game looks
for an invite (port/linux/src/p2p.c renames the file away and reads it,
once a second). Nonzero when it is written.

The file appears whole: it is written beside its name and renamed into
place, so a game reading at that moment sees either the previous link or
this one, never half of one. Anything that is not a halo:// link is
refused. */
int launcher_invite_write(const char *data_root, const char *link);

#endif
