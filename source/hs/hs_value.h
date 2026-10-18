/* port: scripting stores narrow values in the low bits of a 32-bit slot.
 * Keep typed aliases at those bits on either byte order. This is the native
 * runtime layout; loading little-endian map bytes still requires conversion. */
#ifndef __HS_VALUE_H
#define __HS_VALUE_H

#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define HS_SLOT_FIELD(type, name, padding) unsigned char hs_pad_##name[padding]; type name
#else
#define HS_SLOT_FIELD(type, name, padding) type name; unsigned char hs_pad_##name[padding]
#endif
#define HS_SHORT_SLOT(name) HS_SLOT_FIELD(short, name, 2)
#define HS_WORD_SLOT(name) HS_SLOT_FIELD(unsigned short, name, 2)
#define HS_BOOLEAN_SLOT(name) HS_SLOT_FIELD(unsigned char, name, 3)
#define HS_SHORT_ALIAS(name) struct { HS_SHORT_SLOT(name); }
#define HS_WORD_ALIAS(name) struct { HS_WORD_SLOT(name); }
#define HS_BOOLEAN_ALIAS(name) struct { HS_BOOLEAN_SLOT(name); }

/* Retain unused high bits, just as a narrow store did on little-endian hosts. */
static __inline__ void hs_slot_store_short(long *slot, short value)
{
    *slot = (long)(((unsigned long)*slot & 0xffff0000UL) | (unsigned short)value);
}
static __inline__ void hs_slot_store_boolean(long *slot, unsigned char value)
{
    *slot = (long)(((unsigned long)*slot & 0xffffff00UL) | value);
}
static __inline__ int hs_slot_equal(long left, long right, short size)
{
    unsigned long mask;
    mask = size == 0 ? 0 : size == 1 ? 0xffUL : size == 2 ? 0xffffUL : 0xffffffffUL;
    return (((unsigned long)left ^ (unsigned long)right) & mask) == 0;
}
#endif
