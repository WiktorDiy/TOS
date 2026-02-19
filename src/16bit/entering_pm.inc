KBD_CTRL_DATA                   equ     0x60
KBD_CTRL_CMD                    equ     0x64
KBD_CTRL_DISABLE_KBD            equ     0xAD
KBD_CTRL_ENABLE_KBD             equ     0xAE
KBD_CTRL_READ_CTRL_OUT_P        equ     0xD0
KBD_CTRL_WRITE_CTRL_OUT_P       equ     0xD1


GoProtected:
    cli


    call EN_A20_gate
    call LOAD_GDT


    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:0x9000      ; far jump: CS = 0x08, EIP = 0x00009000


    ret


EN_A20_gate:
    call .A20_WAIT_IN
    mov al, KBD_CTRL_DISABLE_KBD
    out KBD_CTRL_CMD, al

    call .A20_WAIT_IN
    mov al, KBD_CTRL_READ_CTRL_OUT_P    ; 0xD0
    out KBD_CTRL_CMD, al

    call .A20_WAIT_OUT                  ; wait for data
    in al, KBD_CTRL_DATA
    push ax                             ; ax is enough



    call .A20_WAIT_IN                   ; wait for input buffer empty
    mov al, KBD_CTRL_WRITE_CTRL_OUT_P   ; 0xD1
    out KBD_CTRL_CMD, al

    call .A20_WAIT_IN
    pop ax
    or al, 2
    out KBD_CTRL_DATA, al

    call .A20_WAIT_IN
    mov al, KBD_CTRL_ENABLE_KBD
    out KBD_CTRL_CMD, al

    ret

.A20_WAIT_IN:
    in al, KBD_CTRL_CMD
    test al, 2
    jnz .A20_WAIT_IN
    ret

.A20_WAIT_OUT:
    in al, KBD_CTRL_CMD 
    test al, 1
    jz .A20_WAIT_OUT
    ret


LOAD_GDT:
    lgdt [GDTab_disc]
    ret



GDTab:
    dq 0


    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
 GDTab_disc:
    dw GDTab_disc - GDTab - 1
    dd GDTab   


