; Microsoft CRT signed 64-bit division helper.
;
; The dividend and divisor occupy four stack words. The quotient is returned
; in EDX:EAX, with 16-byte callee stack cleanup. The routine normalizes both
; signs, divides using unsigned arithmetic, then restores the quotient sign.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC __alldiv
__alldiv PROC
    push edi
    push esi
    push ebx
    xor edi, edi
    mov eax, DWORD PTR [esp + 14h]
    or eax, eax
    jge dividend_nonnegative
    inc edi
    mov edx, DWORD PTR [esp + 10h]
    neg eax
    neg edx
    sbb eax, 0
    mov DWORD PTR [esp + 14h], eax
    mov DWORD PTR [esp + 10h], edx
dividend_nonnegative:
    mov eax, DWORD PTR [esp + 1ch]
    or eax, eax
    jge divisor_nonnegative
    inc edi
    mov edx, DWORD PTR [esp + 18h]
    neg eax
    neg edx
    sbb eax, 0
    mov DWORD PTR [esp + 1ch], eax
    mov DWORD PTR [esp + 18h], edx
divisor_nonnegative:
    or eax, eax
    jne wide_divisor
    mov ecx, DWORD PTR [esp + 18h]
    mov eax, DWORD PTR [esp + 14h]
    xor edx, edx
    div ecx
    mov ebx, eax
    mov eax, DWORD PTR [esp + 10h]
    div ecx
    mov edx, ebx
    jmp apply_sign
wide_divisor:
    mov ebx, eax
    mov ecx, DWORD PTR [esp + 18h]
    mov edx, DWORD PTR [esp + 14h]
    mov eax, DWORD PTR [esp + 10h]
normalize_divisor:
    shr ebx, 1
    rcr ecx, 1
    shr edx, 1
    rcr eax, 1
    or ebx, ebx
    jne normalize_divisor
    div ecx
    mov esi, eax
    mul DWORD PTR [esp + 1ch]
    mov ecx, eax
    mov eax, DWORD PTR [esp + 18h]
    mul esi
    add edx, ecx
    jb quotient_too_large
    cmp edx, DWORD PTR [esp + 14h]
    ja quotient_too_large
    jb quotient_fits
    cmp eax, DWORD PTR [esp + 10h]
    jbe quotient_fits
quotient_too_large:
    dec esi
quotient_fits:
    xor edx, edx
    mov eax, esi
apply_sign:
    dec edi
    jne return_quotient
    neg edx
    neg eax
    sbb edx, 0
return_quotient:
    pop ebx
    pop esi
    pop edi
    ret 10h
__alldiv ENDP

.text ENDS
END
