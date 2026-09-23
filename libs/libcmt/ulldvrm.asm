; Microsoft CRT combined unsigned 64-bit division/remainder helper.
; This is a compiler-internal register ABI, not an ordinary C-callable routine.
; EDX:EAX carries the quotient while EBX:ECX carries the remainder.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __aulldvrm
__aulldvrm PROC
    push esi
    mov eax, DWORD PTR [esp + 14h]
    or eax, eax
    jne wide_divisor
    mov ecx, DWORD PTR [esp + 10h]
    mov eax, DWORD PTR [esp + 0ch]
    xor edx, edx
    div ecx
    mov ebx, eax
    mov eax, DWORD PTR [esp + 8]
    div ecx
    mov esi, eax
    mov eax, ebx
    mul DWORD PTR [esp + 10h]
    mov ecx, eax
    mov eax, esi
    mul DWORD PTR [esp + 10h]
    add edx, ecx
    jmp calculate_remainder
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
    mov esi, eax
    mul DWORD PTR [esp + 14h]
    mov ecx, eax
    mov eax, DWORD PTR [esp + 10h]
    mul esi
    add edx, ecx
    jb quotient_too_large
    cmp edx, DWORD PTR [esp + 0ch]
    ja quotient_too_large
    jb quotient_fits
    cmp eax, DWORD PTR [esp + 8]
    jbe quotient_fits
quotient_too_large:
    dec esi
    sub eax, DWORD PTR [esp + 10h]
    sbb edx, DWORD PTR [esp + 14h]
quotient_fits:
    xor ebx, ebx
calculate_remainder:
    sub eax, DWORD PTR [esp + 8]
    sbb edx, DWORD PTR [esp + 0ch]
    neg edx
    neg eax
    sbb edx, 0
    mov ecx, edx
    mov edx, ebx
    mov ebx, ecx
    mov ecx, eax
    mov eax, esi
    pop esi
    ret 10h
__aulldvrm ENDP

.text ENDS
END
