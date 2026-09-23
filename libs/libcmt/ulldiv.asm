; Microsoft CRT unsigned 64-bit division helper.
; Four operand stack words are callee-cleaned; EDX:EAX returns the quotient.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __aulldiv
__aulldiv PROC
    push ebx
    push esi
    mov eax, DWORD PTR [esp + 18h]
    or eax, eax
    jne wide_divisor
    mov ecx, DWORD PTR [esp + 14h]
    mov eax, DWORD PTR [esp + 10h]
    xor edx, edx
    div ecx
    mov ebx, eax
    mov eax, DWORD PTR [esp + 0ch]
    div ecx
    mov edx, ebx
    jmp return_quotient
wide_divisor:
    mov ecx, eax
    mov ebx, DWORD PTR [esp + 14h]
    mov edx, DWORD PTR [esp + 10h]
    mov eax, DWORD PTR [esp + 0ch]
normalize_divisor:
    shr ecx, 1
    rcr ebx, 1
    shr edx, 1
    rcr eax, 1
    or ecx, ecx
    jne normalize_divisor
    div ebx
    mov esi, eax
    mul DWORD PTR [esp + 18h]
    mov ecx, eax
    mov eax, DWORD PTR [esp + 14h]
    mul esi
    add edx, ecx
    jb quotient_too_large
    cmp edx, DWORD PTR [esp + 10h]
    ja quotient_too_large
    jb quotient_fits
    cmp eax, DWORD PTR [esp + 0ch]
    jbe quotient_fits
quotient_too_large:
    dec esi
quotient_fits:
    xor edx, edx
    mov eax, esi
return_quotient:
    pop esi
    pop ebx
    ret 10h
__aulldiv ENDP

.text ENDS
END
