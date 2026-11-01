/* Admission for the experimental camera wire format, independent of MCC. */
#ifndef THIRD_PERSON_PROTOCOL_H
#define THIRD_PERSON_PROTOCOL_H

#define THIRD_PERSON_PROTOCOL_JOIN_KIND 101
#define THIRD_PERSON_PROTOCOL_JOIN_REVISION 1
#define THIRD_PERSON_PROTOCOL_JOIN_MAX 520

/* Buffers must hold JOIN_MAX bytes and be word-aligned, like engine messages. */
void const *third_person_protocol_join(void const *message, unsigned short bytes, void *buffer);
int third_person_protocol_unwrap(void const *message, unsigned short bytes,
    void *inner, unsigned short *inner_bytes);

#endif
