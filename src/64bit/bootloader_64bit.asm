
[BITS 64]
MAIN_ASM:
    call clear
    mov al, '#'
    call ECHO
    mov al, '#'
    call ECHO
    mov al, '#'
    call ECHO
    mov al, '#'
    call ECHO
    mov al, '#'
    call ECHO
    mov al, '#'
    call ECHO
    call crlf

    call AHCI_DISK_INIT



    call crlf
    mov al, '@'
    call ECHO

    ; AX = (bus << 8) | device
    ; DL = function

    ret
%include "src/64bit/UTILS64.inc"
%include "src/64bit/PCI_SCAN.inc"
%include "src/64bit/DISK.inc"