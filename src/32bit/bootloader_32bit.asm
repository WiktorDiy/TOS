org 0x9000
[bits 32]
SCREEN  equ 0xB8000

ADDRESSES:
BOOT_DRIVE_NUM		equ     	0x07C0		;BYTE

E820_ENTRIES_CNT	equ			0x07D2		;WORD
TOTAL_RAM			equ			0x07C8		;QUAD

PAGE_TABLES_LONG    equ         0x07D0      ;QUAD
STACK_TOP           equ         0x07D8      ;QUAD
  
MEMORY_MAP_START    equ    		0x0800
MEMORY_MAP_END		equ			0x0DFF

    
PROTECTED_MODE_ENTRY:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax              ; "reset" reverything for the 32 bit PM   
    
    mov eax, 0x55AA55AA
    mov [0x01001000], eax
    mov eax, 0x12345678
    mov [0x01001004], eax

    mov ecx, 0x7D0
    mov edi, SCREEN
    inc edi
.wgitify:
    mov al, 0x0F
    stosb 
    inc edi
    loop .wgitify


    call checkCPUID
    
    add al, '0'
    mov [SCREEN + 2], al

    mov al, 0x0F
    mov [SCREEN + 3], al

    call check_long_mode

    add al, '0'
    mov [SCREEN + 4], al

    mov al, 0x0F
    mov [SCREEN + 5], al

    mov byte [x], 0
    mov byte [y], 1
    mov eax, 0x4000
    mov ebx, 0x100000

    call FIND_USABLE_REGION

    mov [PAGE_TABLES_LONG], eax
    mov [PAGE_TABLES_LONG + 4], edx

    std
    mov esi, PAGE_TABLES_LONG + 7   ; start at last byte
    mov ecx, 8
.pq:
    lodsb
    call PRBYTE_32
    loop .pq
    cld 

    mov byte [x], 0
    mov byte [y], 2

    mov eax, 0x10000
    mov ebx, [PAGE_TABLES_LONG]
    add ebx, 0x4000
    
    call FIND_USABLE_REGION

    add eax, 0x10000
    mov dword [STACK_TOP], eax


    std
    mov esi, STACK_TOP + 7   ; start at last byte
    mov ecx, 8
.pq2:
    lodsb
    call PRBYTE_32
    loop .pq2
    cld 

    

 


; Assume PAGE_TABLES_LONG holds physical base of 16 KB region
    call clear_block
    mov eax, 0x80000001
    xor ecx, ecx
    cpuid
    bt edx, 26           ; test bit 26 (1GiB pages)
    jnc NO_1_GB


 ; ---------------------------------------------------------
; Build minimal 2 MB identity map:
;   PML4[0] → PDPT
;   PDPT[0] → PD
;   PD[0]   → 2 MB page at physical 0x00000000
; ---------------------------------------------------------

    ; EBX = PDPT physical address
    call get_pdpt
    mov ebx, eax

    ; EAX = PML4 physical address
    call get_pml4

    ; PML4[0] = PDPT | Present | Write
    or  ebx, 0x03
    mov dword [eax], ebx
    mov dword [eax+4], 0
    ;PML4 mapped PDPT

    call get_pdpt
    mov dword [eax], 0x00000083
    add eax, 8
    mov dword [eax], 0x40000083
    add eax, 8

    mov ebx, eax    
    ; ECX = PD physical address
    call get_pd
    ;
    ; PDPT[2] = PD | Present | Write
    or  eax, 0x03
    mov dword [ebx], eax

    
    call get_pd
    mov ebx, eax
    call get_pt0

    or eax, 0x03
    mov dword [ebx], eax
   


    
    call get_pml4
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5        ; CR4.PAE = 1
    mov cr4, eax

    mov ecx, 0xC0000080   ; IA32_EFER
    rdmsr
    or eax, 1 << 8        ; LME = Long Mode Enable
    wrmsr

    mov eax, cr0
    or eax, 1 << 31       ; PG = 1
    mov cr0, eax

    lgdt [gdt64_desc]

    jmp 0x08:long_mode_entry

.end:
    hlt
    jmp .end


gdt64:
    dq 0x0000000000000000        ; null
    dq 0x00AF9A000000FFFF        ; 0x08: 64-bit code
    dq 0x00AF92000000FFFF        ; 0x10: 64-bit data

gdt64_desc:
    dw gdt64_desc - gdt64 - 1
    dd gdt64                     ; base (32-bit, identity-mapped)

ngb: db "1 GB PAGES NOT SUPPORTED!!!!!"


NO_1_GB:
    mov si, ngb
    call print_32
    jmp $
%include "src/32bit/CPUID.inc"
%include "src/32bit/CPULONG.inc"
%include "src/32bit/PAGES_TABLES_SPACE.inc"
%include "src/32bit/UTILS.inc"

long_mode_entry:
    [bits 64]
    mov ax, 0x10        ; 64-bit data selector
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, [STACK_TOP]

    call MAIN_ASM

    jmp $
    ; set up a 64-bit stack if you haven’t yet
    ; mov rsp, SOME_STACK_TOP

    ; now you’re fully in long mode
    ; call your 64-bit kernel main here
%include "src/64bit/bootloader_64bit.asm"