org 0x7E00
bits 16

entry:
	mov [BOOT_DRIVE_NUM], al     ;save the boot drive number
	jmp BOOTL
; =====================================================================
;                 REAL‑MODE PHYSICAL MEMORY MAP (0x0000–0xFFFF)
;                 Valid for all IBM‑compatible PCs at boot
; =====================================================================
; Range (hex)        Size        Description
; ---------------------------------------------------------------------

; 0000 – 03FF      1 KB          Interrupt Vector Table (IVT)
;                                256 × 4‑byte far pointers.
;                                BIOS and DOS rely on this. Do not touch.

; 0400 – 04FF      256 B         BIOS Data Area (BDA)
;                                Keyboard flags, COM/LPT ports, timer tick
;                                counter at 046Ch, memory size at 0413h.

; 0500 – 06FF      512 B         Reserved / BIOS workspace
;                                EBDA pointer lives in BDA, but EBDA itself
;                                is far above this region (usually 9FC00h).

; 0700 – 07BF      192 B         BIOS scratch area
;                                Temporary workspace for INT calls.
;                                Safe to read; BIOS may clobber during calls.

; 07C0 – 7BFF      ~7 KB         Free area
;                                This os will it for constant data:
;                                MEM_MAP GDT IDT etc.

; 7C00 – 7DFF      512 B         Boot sector load address
;                                BIOS loads LBA 0 (or floppy sector 1) here.
;                                Execution begins at 0000:7C00.

; 7E00 – FFFF      ¬7 KB         Free RAM
;                                Common location for stage‑2 loader.
;                                Safe unless you overwrite your own stack.

; ---------------------------------------------------------------------






ADDRESSES:
BOOT_DRIVE_NUM		equ     	0x07C0		;BYTE

E820_ENTRIES_CNT	equ			0x07D2		;WORD
TOTAL_RAM			equ			0x07C8		;QUAD
  
MEMORY_MAP_START    equ    		0x0800
MEMORY_MAP_END		equ			0x0DFF

VALUES:
MAX_MEMORY_ENTRIES	equ			75
                                

%include "src/16bit/terminal.inc"

; ============================================================
; GetMemoryMap
; Collects the full E820 memory map into E820Map
; Outputs:
;   E820Entries = number of entries
;   TotalRAM    = sum of all usable RAM (type 1)
; ============================================================
GetMemoryMap:
    xor ebx, ebx            ; continuation value = 0
    mov di, MEMORY_MAP_START         ; ES:DI = buffer
    

.next_entry:
    mov eax, 0E820h         ; E820h function
    mov edx, 534D4150h      ; "SMAP"
    mov ecx, 20             ; size of buffer
    int 15h
    jc .done                ; carry = error or end

    cmp eax, 534D4150h
    jne .done               ; signature mismatch

    ; store entry
    add di, 20
    inc word [E820_ENTRIES_CNT]

    ; check entry count
    mov ax, [E820_ENTRIES_CNT]
    cmp ax, MAX_MEMORY_ENTRIES
    jae .pc_change

    


    mov eax, [di-20+8]      ; length low
    mov edx, [di-20+12]     ; length high
    add [TOTAL_RAM], eax
    adc [TOTAL_RAM+4], edx

.skip:
    test ebx, ebx           ; EBX = 0 means last entry
    jnz .next_entry

.done:
    ret



; ============================================================
; Error: too many memory map entries for this OS
; ============================================================
.pc_change:
    mov si, msg_pc_change
    call print
	jmp BOOTL.end


msg_pc_change: db "THIS PC HAS TOO FRAGMENTED MEMORY,  SYSTEM HALTED!" , 0  
;-----------------------------------------------
; E820 Memory Map Entry (20 bytes total)
; ------------------------------------------------------------
; Offset | Size | Description
; -------+------+----------------------------------------------
;   0    |  QWORD  | Base Address (start of region)
;   8    |  QWORD  | Length in bytes (size of region)
;  16    |  DWORD  | Type:
;                 |   1 = Usable RAM
;                 |   2 = Reserved
;                 |   3 = ACPI Reclaimable
;                 |   4 = ACPI NVS
;                 |   5 = Bad Memory
print_mem_region:
	
	pusha
	mov bx, 20
	mul bx
	mov di, ax
	lea si, [MEMORY_MAP_START + di]
	mov bp, si

	call p_qd_MSB 

	
	call space
	mov al, ':'
	call echo
	call space


	call p_qd_MSB

	call space

	call p_dd_MSB
	
	call crlf	

	popa
	ret

ListMem:
	mov ax, [E820_ENTRIES_CNT]
	call p_word
	call space 
	call space

	mov si, TOTAL_RAM
	call p_qd_MSB
	call crlf

	xor ax, ax
	mov cx, [E820_ENTRIES_CNT]
.lp:	
	call print_mem_region
	inc ax
	loop .lp
	ret


	%include "src/16bit/entering_pm.inc"
	


BOOTL:
	call GetMemoryMap
	call ListMem

	call GoProtected

.end:		
	hlt
	jmp .end



	

	

	


