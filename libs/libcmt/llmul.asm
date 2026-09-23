; Microsoft CRT 64-bit multiplication helper.
;
; The operands occupy four stack words. The compiler expects EDX:EAX as the
; low 64 bits of the product and a 16-byte callee stack cleanup.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __allmul
__allmul PROC
    mov eax, DWORD PTR [esp + 8]
    mov ecx, DWORD PTR [esp + 10h]
    or ecx, eax
    mov ecx, DWORD PTR [esp + 0ch]
    jne multiply_wide
    mov eax, DWORD PTR [esp + 4]
    mul ecx
    ret 10h
multiply_wide:
    push ebx
    mul ecx
    mov ebx, eax
    mov eax, DWORD PTR [esp + 8]
    mul DWORD PTR [esp + 14h]
    add ebx, eax
    mov eax, DWORD PTR [esp + 8]
    mul ecx
    add edx, ebx
    pop ebx
    ret 10h
__allmul ENDP

.text ENDS
END
