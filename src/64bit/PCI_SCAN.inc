; =====================================================================
;  PCI CLASS CODES (0x00 – 0x07)
;  These identify the *type* of PCI device.
;  Read from PCI config space at offset 0x0B.
; =====================================================================
;
;  Class  Description
;  -----  --------------------------------------------------------------
;  0x00   Unclassified / Legacy device
;         - Subclass 0x00: Non-VGA unclassified
;         - Subclass 0x01: VGA-compatible unclassified
;
;  0x01   Mass Storage Controller
;         - 0x00: SCSI
;         - 0x01: IDE
;         - 0x02: Floppy
;         - 0x03: IPI
;         - 0x04: RAID
;         - 0x05: ATA (obsolete)
;         - 0x06: SATA
;         - 0x07: SAS
;         - 0x08: NVM Express (NVMe)
;
;  0x02   Network Controller
;         - 0x00: Ethernet
;         - 0x80: Other network
;
;  0x03   Display Controller
;         - 0x00: VGA
;         - 0x01: XGA
;         - 0x02: 3D controller
;
;  0x04   Multimedia Controller
;         - 0x00: Video device
;         - 0x01: Audio device
;         - 0x02: Computer telephony
;
;  0x05   Memory Controller
;         - 0x00: RAM controller
;         - 0x01: Flash controller
;
;  0x06   Bridge Device
;         - 0x00: Host bridge
;         - 0x01: ISA bridge
;         - 0x04: PCI-to-PCI bridge
;         - 0x09: PCI-to-PCI bridge (subtractive decode)
;
;  0x07   Simple Communication Controller
;         - 0x00: Serial controller
;         - 0x01: Parallel controller
;         - 0x80: Other communication device
;
; =====================================================================

; =====================================================================
;  pci_read32
;  ----------------
;  Read a 32‑bit value from PCI configuration space using the
;  standard I/O‑port mechanism (0xCF8 / 0xCFC).
;
;  INPUT:
;       EAX = Bus number      (0–255)
;       EBX = Device number   (0–31)
;       ECX = Function number (0–7)
;       EDX = Register offset (must be 4‑byte aligned: 0x00, 0x04, 0x08...)
;
;  OUTPUT:
;       EAX = 32‑bit value read from PCI config space
;
;  NOTES:
;       - This uses the "Configuration Address" mechanism.
;       - The address format is:
;             31      = Enable bit (must be 1)
;             30–24   = Reserved
;             23–16   = Bus number
;             15–11   = Device number
;             10–8    = Function number
;             7–2     = Register offset (aligned)
;             1–0     = Must be 0
;
;       - Writes to 0xCF8 select the config register.
;       - Reads from 0xCFC return the 32‑bit value.
;
; =====================================================================

pci_read32:
    ; -------------------------------------------------------------
    ; Build CONFIG_ADDRESS in EDI
    ; -------------------------------------------------------------
    mov  edi, eax            ; EDI = bus
    shl  edi, 16             ; move bus into bits 23–16

    mov  esi, ebx            ; ESI = device
    shl  esi, 11             ; move device into bits 15–11
    or   edi, esi

    mov  esi, ecx            ; ESI = function
    shl  esi, 8              ; move function into bits 10–8
    or   edi, esi

    or   edi, edx            ; add register offset (bits 7–2)
    or   edi, 0x80000000     ; set enable bit (bit 31)

    ; -------------------------------------------------------------
    ; Write CONFIG_ADDRESS to port 0xCF8
    ; -------------------------------------------------------------
    mov  dx, 0xCF8
    mov  eax, edi
    out  dx, eax

    ; -------------------------------------------------------------
    ; Read CONFIG_DATA from port 0xCFC
    ; -------------------------------------------------------------
    mov  dx, 0xCFC
    in   eax, dx
    ret

; =====================================================================
;  pci_find_ahci
;  ----------------
;  Scans all PCI devices and returns the bus/device of the first
;  AHCI SATA controller.
;
;  AHCI signature:
;       Class    = 0x01 (Mass Storage)
;       Subclass = 0x06 (SATA)
;       ProgIF   = 0x01 (AHCI 1.0)
;
;  OUTPUT:
;       AX = (bus << 8) | device
;       AX = FFFFh if not found
; =====================================================================

pci_find_ahci:
    xor r8, r8
.bus_loop:
   xor r9, r9

.device_loop:
    mov rax, r8         ; bus
    mov rbx, r9         ; device
    xor rcx, rcx         ; function = 0
    mov rdx, 0x08
    call pci_read32
    and eax, 0xFFFFFF00
    cmp eax, 0x01060100
    je .done

    inc r9
    cmp r9, 32
    jb .device_loop
    inc r8
    cmp r8, 256
    jb .bus_loop

    mov r8b, 0xFF
    mov r9b, 0xFF


.done:
    mov rax, r8
    mov rbx, r9
    ret

