[GLOBAL enable_paging]
section .text
enable_paging:
    mov eax, page_directory
    mov cr3, eax
    
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax