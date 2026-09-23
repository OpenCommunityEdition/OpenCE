; Microsoft CRT bounded byte-string comparison.
; The result is -1, 0, or 1 using unsigned-byte ordering.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC _strncmp
_strncmp PROC
    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov ecx, DWORD PTR [ebp + 10h]
    jecxz equal
    mov ebx, ecx
    mov edi, DWORD PTR [ebp + 8]
    mov esi, edi
    xor eax, eax
    repne scasb
    neg ecx
    add ecx, ebx
    mov edi, esi
    mov esi, DWORD PTR [ebp + 0ch]
    repe cmpsb
    mov al, BYTE PTR [esi - 1]
    xor ecx, ecx
    cmp al, BYTE PTR [edi - 1]
    ja greater
    je equal
    dec ecx
    dec ecx
greater:
    not ecx
equal:
    mov eax, ecx
    pop ebx
    pop esi
    pop edi
    leave
    ret
_strncmp ENDP

.text ENDS
END
