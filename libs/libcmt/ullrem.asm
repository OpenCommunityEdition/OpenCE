; Microsoft CRT unsigned 64-bit remainder helper.
; Four operand stack words are callee-cleaned; EDX:EAX returns the remainder.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __aullrem
__aullrem PROC
    push ebx
    mov eax, DWORD PTR [esp + 14h]
    or eax, eax
    jne wide_divisor
    mov ecx, DWORD PTR [esp + 10h]
    mov eax, DWORD PTR [esp + 0ch]
    xor edx, edx
    div ecx
    mov eax, DWORD PTR [esp + 8]
    div ecx
    mov eax, edx
    xor edx, edx
    jmp return_remainder
wide_divisor:
    mov ecx, eax
    mov ebx, DWORD PTR [esp + 10h]
    mov edx, DWORD PTR [esp + 0ch]
    mov eax, DWORD PTR [esp + 8]
normalize_divisor:
    shr ecx, 1
    rcr ebx, 1
    shr edx, 1
    rcr eax, 1
    or ecx, ecx
    jne normalize_divisor
    div ebx
    mov ecx, eax
    mul DWORD PTR [esp + 14h]
    xchg ecx, eax
    mul DWORD PTR [esp + 10h]
    add edx, ecx
    jb quotient_too_large
    cmp edx, DWORD PTR [esp + 0ch]
    ja quotient_too_large
    jb quotient_fits
    cmp eax, DWORD PTR [esp + 8]
    jbe quotient_fits
quotient_too_large:
    sub eax, DWORD PTR [esp + 10h]
    sbb edx, DWORD PTR [esp + 14h]
quotient_fits:
    sub eax, DWORD PTR [esp + 8]
    sbb edx, DWORD PTR [esp + 0ch]
    neg edx
    neg eax
    sbb edx, 0
return_remainder:
    pop ebx
    ret 10h
__aullrem ENDP

.text ENDS
END
