; Microsoft CRT unsigned 64-bit right-shift helper.
;
; Input: EDX:EAX holds the unsigned value, CL holds the shift count.
; Output: EDX:EAX holds the shifted value. This is the compiler's register ABI,
; not a callable C function signature.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __aullshr
__aullshr PROC
    cmp cl, 40h
    jae shift_64
    cmp cl, 20h
    jae shift_32
    shrd eax, edx, cl
    shr edx, cl
    ret
shift_32:
    mov eax, edx
    xor edx, edx
    and cl, 1fh
    shr eax, cl
    ret
shift_64:
    xor eax, eax
    xor edx, edx
    ret
__aullshr ENDP

.text ENDS
END
