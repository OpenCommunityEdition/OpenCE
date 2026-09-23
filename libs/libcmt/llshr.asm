; Microsoft CRT signed 64-bit arithmetic right shift helper.
;
; Input: EDX:EAX holds the signed value, CL holds the shift count.
; Output: EDX:EAX holds the shifted value. The ABI is register-based, so this
; vendor arithmetic helper is maintained as MASM, not a fake C signature.
; The segment's 16-byte alignment is MASM's authentic ordinary-code layout.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __allshr
__allshr PROC
    cmp cl, 40h
    jae shift_64
    cmp cl, 20h
    jae shift_32
    shrd eax, edx, cl
    sar edx, cl
    ret
shift_32:
    mov eax, edx
    sar edx, 1fh
    and cl, 1fh
    sar eax, cl
    ret
shift_64:
    sar edx, 1fh
    mov eax, edx
    ret
__allshr ENDP

.text ENDS
END
