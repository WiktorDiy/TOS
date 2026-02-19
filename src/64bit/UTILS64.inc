[BITS 64]
%macro rts 0
    ret
%endmacro

%macro pha 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
%endmacro

%macro pla 0
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro andi 2
    push rcx
    mov  rcx, %2
    and  %1, rcx
    pop  rcx
%endmacro

%macro movi 2
    push rcx
    mov rcx, %2
    mov %1, [rcx]
    pop rcx
%endmacro


%macro mmio_read32 2
    push rcx
    mov  rcx, %2
    mov  %1, dword [rcx]
    pop  rcx
%endmacro

%macro mmio_write32 2
    push rdx
    mov     rdx, %1        ; address
    mov     eax, %2        ; value
    mov     dword [rdx], eax
    pop rdx
%endmacro

%macro dot 0
    push rax
    mov al, '.'
    call ECHO
    pop rax
%endmacro

X:      db 0
Y:      db 0
SCREEN  equ 0xB8000
PRINT:
    pha
.lp:
    lodsb
    cmp al, 0
    je .done
    call ECHO
    jmp .lp
    pla
.done:
    rts

clear:
    mov rdi, SCREEN
    mov rcx, (80*25)
    mov ax, (0x0F20)

    rep stosw

    ret

scroll:
    mov rdi, SCREEN + (80*25*2)
    mov ecx, 80
    mov ax, (0x0F20)

    rep stosw

    mov rdi, SCREEN
    mov rsi, SCREEN + (2 * 80)
    mov rcx, (80*25)

.lp2:
    lodsw
    stosw
    loop .lp2
    ret

;al - x, ah - y
set_scr:
    mov byte [rel X], al
    mov byte [rel Y], ah
    ret

; AL = character to print
crlf:
    pha

    mov byte [rel X], 0
    mov al, byte [rel Y]
    inc al
    cmp al, 25
    jb .done
    call scroll
    dec al
.done:
    mov byte [rel Y], al

    pla
    ret

PRDWORD_BIN:
    push rax
    shr eax, 24
    call PRBYTE_BIN
    mov al, '.'
    call ECHO
    pop rax

    push rax
    shr eax, 16
    call PRBYTE_BIN
    mov al, '.'
    call ECHO
    pop rax
    push rax
    shr eax, 8
    call PRBYTE_BIN
    mov al, '.'
    call ECHO
    pop rax
    jmp PRBYTE_BIN
PRWORD_BIN:
    push rax
    shr ax, 8
    call PRBYTE_BIN
    mov al, '.'
    call ECHO
    pop rax

PRBYTE_BIN:
    push rcx
    mov rcx, 8
.lp:
    rcl al, 1
    jc .p1

    ; print '0'
    push rax
    mov al, '0'
    call ECHO
    pop rax
    jmp .next

.p1:
    ; print '1'
    push rax
    mov al, '1'
    call ECHO
    pop rax

.next:
    loop .lp
    pop rcx
    ret


PRQWORD:
    push rax
    shr rax, 32
    call PRDWORD
    pop rax
PRDWORD:
    push rax
    shr rax, 16
    call PRWORD
    pop rax
PRWORD:
    push rax
    shr rax, 8
    call PRBYTE
    pop rax

PRBYTE:
    push rax
    call .prhex
    pop rax
    shl al, 4
.prhex:
	and al, 0xF0
	shr al, 4
	cmp al, 9
	ja .l
	add al, '0'
	call ECHO
	ret

.l:	
	sub al, 10
	add al, 'A'
	call ECHO
    ret

ECHO:
    push rax
    push rbx
    push rcx
    push rdx
    ; ----------------------------------------------------
    ; Compute offset = (y * 80 + x) * 2
    ; ----------------------------------------------------
    push rax
    movzx  edx, byte [rel Y]      ; EDX = y
    mov     eax, edx          ; EAX = y
    mov     ecx, 80
    mul     ecx               ; EAX = y * 80

    movzx  edx, byte [rel X]      ; EDX = x
    add     eax, edx          ; EAX = y*80 + x

    shl     eax, 1            ; multiply by 2 (each cell = 2 bytes)
    add     eax, 0xB8000      ; VGA base
    mov     edx, eax
    pop     rax
    mov     byte [edx], al    ; write character

    pop rdx
    pop rcx
    pop rbx
    pop rax


    ; ----------------------------------------------------
    ; Increment cursor (x, y)
    ; ----------------------------------------------------

    inc byte [rel X]              ; x++

    cmp byte [rel X], 80          ; reached end of row?
    jb  .done                 ; if x < 80, finished

    ; x overflow → next line
    mov byte [rel X], 0           ; x = 0
    inc byte [rel Y]              ; y++

    cmp byte [rel Y], 25          ; reached bottom?
    jb  .done                 ; if y < 25, finished

    ; y overflow → wrap to top
    dec byte [rel Y]
    call scroll

.done:
    ret


get_pt0_64:
    mov eax, [PAGE_TABLES_LONG]
    add eax, 0x3000
    adc edx, 0
    ret

