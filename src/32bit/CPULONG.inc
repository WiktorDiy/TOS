; ---------------------------------------------------------
; check_long_mode
; Returns:
;   eax = 1  → long mode supported
;   eax = 0  → NOT supported
; ---------------------------------------------------------

CPUID_EXTENSIONS      equ 0x80000000
CPUID_EXT_FEATURES    equ 0x80000001
CPUID_LM_BIT          equ (1 << 29)

check_long_mode:
    ; Step 1: check if extended CPUID functions exist
    mov eax, CPUID_EXTENSIONS
    cpuid
    cmp eax, CPUID_EXT_FEATURES
    jb .no_lm              ; CPU cannot report LM → no LM

    ; Step 2: read extended feature flags
    mov eax, CPUID_EXT_FEATURES
    cpuid
    test edx, CPUID_LM_BIT
    jz .no_lm              ; LM bit not set → no LM

    mov eax, 1
    ret

.no_lm:
    xor eax, eax
    ret
