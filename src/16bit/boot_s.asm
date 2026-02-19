org 0x7C00
bits 16

; 0x7C00
jmp short START          
align 4

; 0x7C04
jmp short echo           
align 4            

; 0x7C08
jmp short print          
align 4          

echo:
    mov ah, 0x0E
    mov bh, 0
    int 10h
    ret

print:
    lodsb
    and al, al
    jz .done
    call echo
    jmp print
.done:
    ret

START:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000			;set up segment 0x07c0
	cld
	
    mov [disk], dl          ; save boot drive
	

	

    mov si, msg
    call print
	
load_stage2:

    mov si, DAP
    mov dl, [disk]
    mov ah, 0x42
    int 0x13
    jc disk_error

    mov si, msg2
    call print

	xor dx, dx
	mov dl, [disk]

load_pm:
    mov si, DAP2
    mov dl, [disk]
    mov ah, 0x42
    int 0x13
    jc disk_error

    jmp 0x7E00


   

   
disk_error:
    mov al, ah
    add al, 32
    call echo
    
h:
    hlt
    jmp h

; --------- data ---------

DAP:
    db 0x10        ; size of packet (16 bytes)
    db 0x00           ; reserved
    dw 3             ; number of sectors to read
    dw 0x7e00      ; destination offset
    dw 0x0000     ; destination segment
    dq 0x01   ; starting LBA

DAP2:
    db 0x10        ; size of packet (16 bytes)
    db 0x00           ; reserved
    dw 20             ; number of sectors to read
    dw 0x9000      ; destination offset
    dw 0x0000     ; destination segment
    dq 0x04   ; starting LBA


disk:   db 0
msg:    db "loading additional sectors (LBA)...", 10, 13, 0
msg2:   db "loading done, jumping to stage 2", 10, 13, 0
msgerr: db "disk read error!", 0


times 446-($-$$) db 0
times 64 db 0
dw 0xAA55
