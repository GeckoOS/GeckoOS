global switch_pd
switch_pd: ; [esp] = ret, [esp+4] = pd_addr
  mov eax, [esp+4]
  mov cr3, eax

  mov eax, cr0
  or eax, 0x80000001
  mov cr0, eax

  ret

global flush_pg
flush_pg:
  mov eax, cr3
  mov cr3, eax
  ret