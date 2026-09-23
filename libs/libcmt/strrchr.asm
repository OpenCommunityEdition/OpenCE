; Microsoft CRT reverse byte-string search.
; The forward and backward scans use x86 string instructions. The direction
; flag is restored before returning to the caller.

.386
.model flat
OPTION DOTNAME
.text SEGMENT ALIGN(16) PUBLIC 'CODE'

PUBLIC _strrchr
_strrchr PROC
    push ebp
    mov ebp, esp
    push edi
    mov edi, DWORD PTR [ebp + 8]
    xor eax, eax
    or ecx, -1
    repne scasb
    inc ecx
    neg ecx
    dec edi
    mov al, BYTE PTR [ebp + 0ch]
    std
    repne scasb
    inc edi
    cmp BYTE PTR [edi], al
    je found
    xor eax, eax
    jmp done
found:
    mov eax, edi
done:
    cld
    pop edi
    leave
    ret
_strrchr ENDP

.text ENDS
END
