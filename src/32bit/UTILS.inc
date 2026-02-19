x: db 0
y: db 0


print_32:
    lodsb
    cmp al, 0
    je .done
    call echo
    jmp print_32
.done:
    ret

PRBYTE_32:
    push eax
    call prhex
	pop eax
	shl al, 4
prhex:
	and al, 0xF0
	shr al, 4
	cmp al, 9
	ja .l
	add al, '0'
	call echo
	ret

.l:	
	sub al, 10
	add al, 'A'
	call echo
    ret



echo:
    push eax
    push ebx
    push ecx
    push edx
    ; ----------------------------------------------------
    ; Compute offset = (y * 80 + x) * 2
    ; ----------------------------------------------------
    push eax
    movzx  edx, byte [y]      ; EDX = y
    mov     eax, edx          ; EAX = y
    mov     ecx, 80
    mul     ecx               ; EAX = y * 80

    movzx  edx, byte [x]      ; EDX = x
    add     eax, edx          ; EAX = y*80 + x

    shl     eax, 1            ; multiply by 2 (each cell = 2 bytes)
    add     eax, 0xB8000      ; VGA base
    mov     edx, eax
    pop     eax
    mov     byte [edx], al    ; write character

    pop edx
    pop ecx
    pop ebx
    pop eax


    ; ----------------------------------------------------
    ; Increment cursor (x, y)
    ; ----------------------------------------------------

    inc byte [x]              ; x++

    cmp byte [x], 80          ; reached end of row?
    jb  .done                 ; if x < 80, finished

    ; x overflow → next line
    mov byte [x], 0           ; x = 0
    inc byte [y]              ; y++

    cmp byte [y], 25          ; reached bottom?
    jb  .done                 ; if y < 25, finished

    ; y overflow → wrap to top
    mov byte [y], 0

.done:
    ret