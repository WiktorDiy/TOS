; ---------------------------------------------------------
; FIND_USABLE_REGION
; IN:
;   EAX = required length (bytes)
;   EBX = minimum acceptable base address
;
; OUT:
;   EAX = base_low
;   EDX = base_high
;   If not found: EAX = EDX = 0
; ---------------------------------------------------------

FIND_USABLE_REGION:

    push esi
    push ecx
    push edi
    push ebp

    mov esi, MEMORY_MAP_START
    mov cx, [E820_ENTRIES_CNT]

    mov edi, eax        ; required length
    mov ebp, ebx        ; minimum base

    ; enforce skip of BIOS area (<1MB)
    cmp ebp, 0x00100000
    jae .scan
    mov ebp, 0x00100000

.scan:
.next_entry:
    cmp cx, 0
    je .not_found

    ; type
    mov eax, [esi + 16]
    cmp eax, 1
    jne .skip

    ; length
    mov eax, [esi + 8]      ; len_low
    mov edx, [esi + 12]     ; len_high

    ; length >= required?
    cmp edx, 0
    jne .check_base_len

    cmp eax, edi
    jb .skip

.check_base_len:
    ; base
    mov eax, [esi]          ; base_low
    mov edx, [esi + 4]      ; base_high

    ; compute region_end = base + length
    mov ebx, [esi + 8]      ; len_low
    mov ecx, [esi + 12]     ; len_high

    add ebx, eax            ; end_low = base_low + len_low
    adc ecx, edx            ; end_high = base_high + len_high

    ; if region_end <= min_base → skip
    cmp ecx, 0
    jne .check_end_high

    cmp ebx, ebp
    jbe .skip
    jmp .check_overlap

.check_end_high:
    ; if end_high < min_base_high (0) → skip
    cmp ecx, 0
    jb .skip

.check_overlap:
    ; Now check if region_start >= min_base
    cmp edx, 0
    jne .use_region_start

    cmp eax, ebp
    jae .use_region_start

    ; region_start < min_base < region_end → return min_base
    mov eax, ebp
    xor edx, edx
    jmp .found

.use_region_start:
    ; return region_start
    ; EAX:EDX already contain base
    jmp .found

.found:
    pop ebp
    pop edi
    pop ecx
    pop esi
    ret

.skip:
    add esi, 20
    dec cx
    jmp .next_entry

.not_found:
    xor eax, eax
    xor edx, edx
    pop ebp
    pop edi
    pop ecx
    pop esi
    ret

clear_block:
    push edi
    push ecx

    mov edi, [PAGE_TABLES_LONG]
    mov ecx, 16 * 1024      ; 16 KB
    xor eax, eax
    rep stosb               ; write 1 byte per iteration

    pop ecx
    pop edi
    ret

get_pml4:
    mov eax, [PAGE_TABLES_LONG]
    mov edx, [PAGE_TABLES_LONG + 4]
    ret

get_pdpt:
    mov eax, [PAGE_TABLES_LONG]
    add eax, 0x1000
    adc edx, 0          ; handle carry
    ret

get_pd:
    mov eax, [PAGE_TABLES_LONG]
    add eax, 0x2000
    adc edx, 0
    ret

get_pt0:
    mov eax, [PAGE_TABLES_LONG]
    add eax, 0x3000
    adc edx, 0
    ret
