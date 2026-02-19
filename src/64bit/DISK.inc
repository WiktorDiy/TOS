bus: db 0
dev: db 0
fnc: db 0

HBA equ 0x80000000

AHCI_DISK_INIT:
    call pci_find_ahci
    push rax
    call PRBYTE
    mov al, '.'
    call ECHO
    mov rax, rbx
    call PRBYTE
    pop rax

    mov byte [rel bus], al
    mov byte [rel dev], bl
    mov byte [rel fnc], 0
    ;mov eax, bus
    ;mov ebx, device
    xor ecx, ecx        ; function 0
    mov edx, 0x24       ; BAR5 offset
    call pci_read32
    
    call crlf
    push rax
    call PRDWORD
    pop rax
    mov cl, 5
    call decode_bar
    
    jnc .err
    
    mov rbx, rax
    andi rbx, 0xFFFFFFF0
    
    ; --------------------------------------------------------------------------
; Page Table Entry Flags: 0x1B (0001_1011b)
;
; Bit 0  (1)  PRESENT          – Page is mapped
; Bit 1  (1)  RW               – Read/Write enabled
; Bit 2  (0)  US               – Supervisor only (kernel)
; Bit 3  (1)  PWT              – Write‑Through caching
; Bit 4  (1)  PCD              – Cache Disable (required for MMIO)
; Bit 5  (0)  ACCESSED         – CPU sets this automatically
; Bit 6  (0)  DIRTY            – CPU sets for writable pages
; Bit 7  (0)  PS               – 0 = 4 KB page
; Bit 12 (0)  PAT              – Default memory type (UC)
;
; Recommended for MMIO regions (e.g., AHCI BAR5)
; --------------------------------------------------------------------------    
    
    or rbx, 0x1B
    call get_pt0_64
    mov [rax], rbx

    call configure_ahci_mmio



    
    ret
.err:
    call clear
    mov rsi, .msg
    call PRINT
    jmp $
.msg: db "BAR5 ENTRY NOT VALID!!!!!", 0
configure_ahci_mmio:
    call crlf

    mmio_read32 eax, HBA
    push rax
    call PRDWORD
    pop rax

    mmio_read32 ebx, HBA + 0x04    ; GHC
    or  ebx, 1 << 31               ; AE = 1
    mmio_write32 HBA + 0x04, ebx

    mmio_read32 edx, HBA + 0x0C    ; PI

    call crlf
    xor ecx, ecx                   ; port index = 0

.next_port:
    push rax
    mov eax, 1
    shl eax, cl        ; or shl eax, esi (but CL is required for variable shifts)
    test edx, eax
    pop rax
    jz   .skip_port            ; not implemented
    
    mov r8, rcx
    call detect_port


.skip_port:

    inc  cl
    cmp  cl, 32
    jb   .next_port



    ret

detect_port:
    pha
    and ecx, 0x000000FF
    ; cl = port index
    xor rax, rax
    mov eax, ecx
    shl eax, 7                     ; * 0x80
    add eax, HBA + 0x100
    xor rdi, rdi
    mov edi, eax                   ; EDI = Px base

    ; -------------------------
    ; 1. Check PxSIG (SATA signature)
    ; -------------------------
    lea rbx, [edi + 0x24]
    mmio_read32 eax, rbx    ; PxSIG
    cmp eax, 0x00000101            ; SATA signature
    jne .no_device

    ; -------------------------
    ; 2. Check PxSSTS (SStatus)
    ; DET = bits 0–3
    ; SPD = bits 4–7
    ; IPM = bits 8–11
    ; -------------------------
    lea rbx, [edi + 0x28]
    mmio_read32 ebx, rbx    ; PxSSTS

    ; DET (device detection)
    mov edx, ebx
    and edx, 0x0F                  ; DET = bits 0–3
    cmp edx, 3                     ; 3 = device present & PHY up
    jne .no_device

    ; IPM (interface power management)
    mov edx, ebx
    shr edx, 8
    and edx, 0x0F                  ; IPM = bits 8–11
    cmp edx, 1                     ; 1 = active
    jne .no_device

    ; -------------------------
    ; If we reach here → SATA device present
    ; -------------------------
    call configure_port

.no_device:
    pla
    ret


%include "src/64bit/port_config.inc"


; ---------------------------------------------------------
; decode_bar:
;   Input:
;       EAX = raw BAR value
;       CL  = BAR index (0..5)
;
;   Behavior:
;       - If bit0 = 1 → I/O BAR
;       - If BAR index is odd → always 32-bit MMIO
;       - Else decode bits1-2 for 32/64-bit MMIO
; ---------------------------------------------------------

decode_bar:
    
    ; -----------------------------------------
    ; Test bit 0: I/O BAR or MMIO BAR?
    ; -----------------------------------------
    test eax, 1
    jnz bar_is_io              ; bit0 = 1 → I/O BAR
      

    ; -----------------------------------------
    ; If BAR index is odd → cannot be 64-bit
    ; BAR1, BAR3, BAR5 are ALWAYS 32-bit MMIO
    ; -----------------------------------------
    test cl, 1                 ; check lowest bit of BAR index
    jnz bar_is_mmio_32         ; odd BAR → forced 32-bit MMIO
    mov r8b, cl
   

    ; -----------------------------------------
    ; MMIO BAR (bit0 = 0)
    ; Now test bits 1–2 for MMIO type
    ; 00 = 32-bit
    ; 10 = 64-bit
    ; -----------------------------------------

    mov ecx, eax
    and ecx, 0x6               ; isolate bits 1 and 2

    cmp ecx, 0x0
    je bar_is_mmio_32          ; 00 → 32-bit MMIO

    cmp ecx, 0x4
    mov cl, r8b
    je bar_is_mmio_64          ; 10 → 64-bit MMIO


    ; -----------------------------------------
    ; Test bit 3: Prefetchable?
    ; -----------------------------------------
    bt eax, 3
    jc bar_is_mmio_prefetchable

    jmp bar_unknown


    bar_is_io:
    call clear
    mov rsi, .ms
    call PRINT
.hang:
    jmp .hang
    ret
.ms: db "BAR IS IO, CURRENTLY UNSUPPORTED!!!", 0


bar_is_mmio_32:
    ; Mask off flags
    
    andi rax, 0x00000000FFFFFFFF
    
    stc                     ; signal success
    ret


bar_is_mmio_64:
    ; BAR(n+1) must be read separately by caller
    ; This handler only indicates that BAR is 64-bit
    
    andi rax, 0x00000000FFFFFFFF
    

    push rax
    add ecx, 1
    shl ecx, 2
    add ecx, 0x10
    mov edx, ecx

    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx

    mov al, byte [rel bus]
    mov bl, byte [rel dev]
    mov cl, byte [rel fnc]
    call pci_read32
    shl rax, 32

    andi rax, 0xFFFFFFFF00000000
    

    mov rbx, rax



    pop rax
    or rax, rbx
    stc
    ret


bar_is_mmio_prefetchable:
    ret


bar_unknown:
    ret