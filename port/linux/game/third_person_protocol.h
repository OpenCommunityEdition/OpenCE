/* Admission for the experimental camera wire format, independent of MCC. */
#ifndef THIRD_PERSON_PROTOCOL_H
#define THIRD_PERSON_PROTOCOL_H

/* Separate agreement versions; never advertise these as upstream compatibility. */
#define THIRD_PERSON_PROTOCOL_VERSION 32716
#define THIRD_PERSON_PROTOCOL_MCC_VERSION 32717

struct network_game;
void third_person_session_host_begin(void);
void third_person_session_host_end(void);
void third_person_session_lock(void);
int third_person_session_locked(void);
int third_person_session_set(int enabled);
int third_person_session_active(void);
void third_person_session_client_begin(unsigned int version);
void third_person_session_client_clear(void);
unsigned int third_person_session_version(char const *map);
int third_person_session_compatible(char const *map, unsigned int version, unsigned int flags);

#define THIRD_PERSON_PROTOCOL_JOIN_KIND 102
#define THIRD_PERSON_PROTOCOL_JOIN_REVISION 1
#define THIRD_PERSON_PROTOCOL_JOIN_MAX 520

/* Buffers must hold JOIN_MAX bytes and be word-aligned, like engine messages. */
void const *third_person_protocol_join(void const *message, unsigned short bytes, void *buffer);
int third_person_protocol_unwrap(void const *message, unsigned short bytes,
    void *inner, unsigned short *inner_bytes);

#endif
