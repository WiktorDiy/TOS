reset:
	jmp 0x7c00
echo:
	jmp 0x7c04
print:
	JMP 0x7c08
crlf:
	push ax
	mov al, 10
	call echo 
	mov al, 13
	call echo
	pop ax
	ret
space:
	push ax
	mov al, ' '
	call echo
	pop ax
	ret

cin:
	mov ah, 0
	int 16h
	ret
;---------------------------------------------------------
; print_ax_hex
; Prints AX as 4 hex digits using BIOS teletype
; Destroys: AX, BX, CX, DX
;---------------------------------------------------------

	
p_word:
	
	push ax
	mov al, ah
	call p_byte
	pop ax

p_byte:
	push ax
	call prhex
	pop ax
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




.fail:
    mov al, '#'
    call echo
    ret


p_ab:
	push ax

	cmp al, ' '
	jb .pb
	cmp al, '~'
	ja .pb

	pop ax 
	jmp echo
	
	
.pb:
	call p_byte
	pop ax
	ret	
		


p_qd_MSB:
	lea si, [si+7]
	std
	mov cx, 8
._loop:
	lodsb
	call p_byte
	loop ._loop
	cld
	lea si, [si+9]
	ret

p_dd_MSB:
	lea si, [si+3]
	std
	mov cx, 4
._loop:
	lodsb
	call p_byte
	loop ._loop
	cld
	lea si, [si+5]
	ret

; -------------