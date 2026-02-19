
; Memory buffers
HBA            equ 0x80000000
CMD_LIST_ADDR  equ 0x01000000
FIS_ADDR       equ 0x01000400
CMD_TABLE_ADDR equ 0x01000800
IDENTIFY_BUF   equ 0x01001000

; -------------------------------
; External helpers assumed:
;   mmio_read32  eax, rbx
;   mmio_write32 rbx, eax
;   PRDWORD      (prints eax as hex)
;   ECHO         (prints al)
; -------------------------------

; -------------------------------
; configure_port
; rdi = AHCI ABAR base
; r8  = port number
; -------------------------------
PxCMD          equ 0x18
configure_port:
    ; r15 = port base = ABAR + 0x100 + n*0x80
    mov     r15, rdi
    mov     rax, r8
  
    mov rax, r8
    call PRBYTE
    dot
    mov rax, r15
    call PRDWORD
    dot
    mov rax, [edi+0x28]
    call PRDWORD
    dot
    call .port_stop
    call PRDWORD
    call .write_cmd_bases
    call .write_cmd_list

    call .write_cmd_table



    call .port_start
    call .issue_cmd


    call crlf

    mov rsi, r15
    add rsi, 0x10
    lodsd

    call PRDWORD
    call crlf


  
    
    
    ret
.port_stop:
    ; r15 = port base (ABAR + 0x100 + port*0x80)

    ; 1. Clear ST (bit 0)
    mov rbx, r15
    add rbx, PxCMD
    mmio_read32 eax, rbx
    and eax, ~1
    mmio_write32 rbx, eax

; 2. Wait until CR (bit 15) == 0
.wait_cr:
    mmio_read32 eax, rbx
    test eax, (1 << 15)
    jnz .wait_cr

    ; 3. Clear FRE (bit 4)
    mmio_read32 eax, rbx
    and eax, ~(1 << 4)
    mmio_write32 rbx, eax

; 4. Wait until FR (bit 14) == 0
.wait_fr:
    mmio_read32 eax, rbx
    test eax, (1 << 14)
    jnz .wait_fr
    ret
.write_cmd_bases:
    ; r15 = port base
    ; rax = physical address of 1 KB command list (aligned to 1024)
    mov eax, CMD_LIST_ADDR
    mov rbx, r15
    add rbx, 0x00        ; PxCLB
    mmio_write32 rbx, eax





    mov rbx, r15
    add rbx, 0x04        ; PxCLBU
    xor rax, rax
    mmio_write32 rbx, eax    ; upper 32 bits (usually 0)

    ; rax = physical address of 256‑byte FIS receive buffer (aligned to 256)
    mov eax, FIS_ADDR
    mov rbx, r15
    add rbx, 0x08        ; PxFB
    mmio_write32 rbx, eax

    xor rax, rax
    mov rbx, r15
    add rbx, 0x0C        ; PxFBU
    mmio_write32 rbx, eax

    ; zero 1 KB command list
    mov rdi, CMD_LIST_ADDR
    mov rcx, 1024/8
    xor rax, rax
    rep stosq

    ; zero 256‑byte FIS buffer
    mov rdi, FIS_ADDR
    mov rcx, 256/8
    xor rax, rax
    rep stosq
    ret
.write_cmd_list:
    ; ----------------------------------------------------
    ; CMD_LIST_ADDR  = 0x01000000
    ; CMD_TABLE_ADDR = 0x01000800
    ; ----------------------------------------------------

    ; 1) Zero the 32‑byte command list entry (slot 0)
    mov rdi, CMD_LIST_ADDR
    mov rcx, 32/8
    xor rax, rax
    rep stosq

    ; 2) DWORD 0: CFL = 5 (20‑byte CFIS), PRDTL = 1
    ; bits 0–4   = CFL
    ; bits 16–31 = PRDTL
    mov dword [CMD_LIST_ADDR + 0], 5 | (1 << 16)

    ; 3) DWORD 2–3: CTBA / CTBAU = pointer to command table
    mov dword [CMD_LIST_ADDR + 8],  CMD_TABLE_ADDR
    mov dword [CMD_LIST_ADDR + 12], 0
    ret
.write_cmd_table:
    ; ----------------------------------------------------
    ; CMD_TABLE_ADDR = 0x01000800
    ; IDENTIFY_BUF   = 0x01001000
    ; ----------------------------------------------------

    ; 1) Zero the command table (256 bytes is enough)
    mov rdi, CMD_TABLE_ADDR
    mov rcx, 256/8
    xor rax, rax
    rep stosq

    ; ----------------------------------------------------
    ; 2) CFIS (Command FIS) — 20 bytes
    ; ----------------------------------------------------

    mov byte [CMD_TABLE_ADDR + 0], 0x27     ; FIS type: Register – Host to Device
    mov byte [CMD_TABLE_ADDR + 1], 1<<7     ; C = 1 (command)
    mov byte [CMD_TABLE_ADDR + 2], 0xEC     ; IDENTIFY DEVICE command
    mov byte [CMD_TABLE_ADDR + 3], 0x40     ; Device = LBA mode

    ; LBA fields and count = 0 for IDENTIFY
    ; (they are already zero from the table clear)

    ; ----------------------------------------------------
    ; 3) PRDT entry (one entry)
    ; ----------------------------------------------------

    ; PRDT entry starts at offset 0x80
    mov dword [CMD_TABLE_ADDR + 0x80], IDENTIFY_BUF      ; DBA
    mov dword [CMD_TABLE_ADDR + 0x84], 0                 ; DBAU
    mov dword [CMD_TABLE_ADDR + 0x88], (512 - 1)         ; Byte count (BC = N-1)
    mov dword [CMD_TABLE_ADDR + 0x8C], 0                 ; IOC = 1 (interrupt on completion)
    ret

.port_start:
    ; r15 = port base

    ; PxCMD
    mov rbx, r15
    add rbx, 0x18

    ; FRE = 1
    mmio_read32 eax, rbx
    or eax, (1 << 4)
    mmio_write32 rbx, eax

    ; ST = 1
    mmio_read32 eax, rbx
    or eax, 1
    mmio_write32 rbx, eax

    ret

.issue_cmd:

    ; fill IDENTIFY buffer with 0xAA
    mov rdi, IDENTIFY_BUF
    mov rcx, 512
    mov al, 0xAA
    rep stosb
    ; PxCI = 1 (slot 0)
    mov rbx, r15
    add rbx, 0x38        ; PxCI
    mov eax, 1
    mmio_write32 rbx, eax
.wait_iss:
    mmio_read32 eax, rbx
    test eax, 1
    jnz .wait_iss
    
    
    call crlf
    dot
    dot
    dot

    mov rsi, FIS_ADDR+0x40
    lodsd
    call PRDWORD
    call crlf                    ;prints 504034h

    mov rsi, FIS_ADDR+0x20
    lodsq
    call PRQWORD
    dot
    dot
    dot
    dot                           ;prints 58405Fh
    
    mov rsi, IDENTIFY_BUF
    lodsq
    call PRQWORD                  ;prints first 8 bytes that look like 40, AA, AA, AA, AA, AA, AA, AA
                                  ;The AA come from block line 208 added for debug.
    
 
    
    
    
    ret
.error:
    mov al, '$'
    call ECHO
    jmp $
