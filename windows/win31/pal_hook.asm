; Win16 INT 2F observer. Original Palettium code. Passive build performs
; no C/Windows/DOS/video calls in the handler. Experimental native-repaint
; build calls a guarded C bridge for 4002h only; original frame is chained.
; Vector management occurs at ordinary Enable/Disable.
ifdef PAL_NATIVE_REPAINT
.386
else
.286
endif
_DATA segment word public use16 'DATA'
old_off dw 0
old_seg dw 0
installed dw 0
seen_bg dw 0
seen_fg dw 0
seen_save dw 0
seen_restore dw 0
last_focus dw 0             ; 0 unknown, 1 background, 2 foreground
seen_query dw 0
_DATA ends
DGROUP group _DATA

PALHOOK_CODE segment word public use16 'CODE'
assume cs:PALHOOK_CODE, ds:DGROUP
public _pal_hook_install, _pal_hook_remove, _pal_hook_read
public PALHOOKSTATUS
ifdef PAL_NATIVE_REPAINT
extrn _pal_native_repaint_event:far
endif

; Incoming stack is IP,CS,FLAGS. Reserve two words WITHOUT altering flags.
; Place prior vector there, restore original live flags/registers, RETF
; consumes only those two words. The prior handler sees the original
; interrupt frame and decides carry/return values for EVERY function.
hook_entry proc far
    push ax
    push ax
    pushf
    push bp
    mov bp,sp
    push ax
    push ds
    mov ax,DGROUP
    mov ds,ax
    mov ax,[bp-2]           ; interrupted AX (SS, not DS)
    cmp ax,4001h
    jne check_fg
    inc seen_bg
    mov last_focus,1
    jmp short chain_old
check_fg:
    cmp ax,4002h
    jne check_save
    inc seen_fg
    mov last_focus,2
ifdef PAL_NATIVE_REPAINT
    ; Driver DS is established above. Preserve 386 register halves too,
    ; since the special USER procedure may reenter Win16 GDI.
    pushfd
    pushad
    push ds
    push es
    push fs
    push gs
    cld
    call _pal_native_repaint_event
    pop gs
    pop fs
    pop es
    pop ds
    popad
    popfd
endif
    jmp short chain_old
check_save:
    cmp ax,4005h
    jne check_restore
    inc seen_save
    jmp short chain_old
check_restore:
    cmp ax,4006h
    jne check_query
    inc seen_restore
    jmp short chain_old
check_query:
    cmp ax,1684h
    jne chain_old
    inc seen_query
chain_old:
    mov ax,old_off
    mov [bp+4],ax
    mov ax,old_seg
    mov [bp+6],ax
    pop ds
    pop ax
    pop bp
    popf
    retf
hook_entry endp

; Get protected-mode DOS multiplex vector, as the Win3.x DDK specifies.
; Preserve DS/BP across DOS; ES:BX is the documented result.
get_vector proc near
    push ds
    push bp
    mov ax,352fh
    int 21h
    pop bp
    pop ds
    ret
get_vector endp

; Set DS:DX, preserving the caller's DS and BP.
set_vector proc near
    push ds
    push bp
    mov ax,252fh
    int 21h
    pop bp
    pop ds
    ret
set_vector endp

; Return AX=1 if verified top, 0 if no installation, 2 if uncertain
; chain ownership. Never overwrite an intervening handler on removal.
_pal_hook_install proc far
    push bp
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    mov ax,DGROUP
    mov ds,ax
    cmp installed,0
    jne install_uncertain
    call get_vector
    mov ax,es
    or ax,ax
    jz install_none
    mov dx,cs
    cmp ax,dx
    jne save_previous
    cmp bx,offset hook_entry
    je install_uncertain     ; cannot chain to ourselves
save_previous:
    mov old_off,bx
    mov old_seg,ax
    mov seen_bg,0
    mov seen_fg,0
    mov seen_save,0
    mov seen_restore,0
    mov seen_query,0
    mov last_focus,0
    mov installed,1          ; state valid before vector becomes visible
    push ds
    push cs
    pop ds
    mov dx,offset hook_entry
    call set_vector
    pop ds
    call get_vector
    mov ax,es
    mov dx,cs
    cmp ax,dx
    jne install_check_old
    cmp bx,offset hook_entry
    je install_ok
install_check_old:
    cmp ax,old_seg
    jne install_uncertain
    cmp bx,old_off
    jne install_uncertain
    mov installed,0
install_none:
    xor ax,ax
    jmp short install_done
install_ok:
    mov ax,1
    jmp short install_done
install_uncertain:
    mov ax,2
install_done:
    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop bp
    retf
_pal_hook_install endp

_pal_hook_remove proc far
    push bp
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    mov ax,DGROUP
    mov ds,ax
    cmp installed,0
    je remove_ok
    call get_vector
    mov ax,es
    mov dx,cs
    cmp ax,dx
    jne remove_conflict
    cmp bx,offset hook_entry
    jne remove_conflict
    mov dx,old_off
    mov ax,old_seg
    push ds
    mov ds,ax
    call set_vector
    pop ds
    call get_vector
    mov ax,es
    cmp ax,old_seg
    jne remove_conflict
    cmp bx,old_off
    jne remove_conflict
    mov installed,0
remove_ok:
    mov ax,1
    jmp short remove_done
remove_conflict:
    mov ax,2
remove_done:
    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop bp
    retf
_pal_hook_remove endp

; Read one 16-bit counter or last notification. Individual reads are
; atomic on x86; a series is NOT a simultaneous snapshot. Counters wrap.
_pal_hook_read proc far
    push bp
    mov bp,sp
    push ds
    push bx
    mov ax,DGROUP
    mov ds,ax
    mov bx,[bp+6]           ; first cdecl argument after far return address
    xor ax,ax
    cmp bx,5
    ja read_done
    shl bx,1
    mov ax,seen_bg[bx]
read_done:
    pop bx
    pop ds
    pop bp
    retf
_pal_hook_read endp
; Diagnostic-only named Win16 FAR PASCAL export. Load our own DS even
; when invoked by an unrelated task. One WORD argument; callee pops it.
; FFFFh requests the v0080 signature (only while installed). No I/O.
PALHOOKSTATUS proc far
    push bp
    mov bp,sp
    push ds
    push bx
    mov ax,DGROUP
    mov ds,ax
    mov ax,0ffffh
    cmp installed,1
    jne status_done
    mov bx,[bp+6]
    cmp bx,0ffffh
    jne status_index
    mov ax,805ah
    jmp short status_done
status_index:
    cmp bx,5
    ja status_done
    shl bx,1
    mov ax,seen_bg[bx]
status_done:
    pop bx
    pop ds
    pop bp
    retf 2
PALHOOKSTATUS endp
PALHOOK_CODE ends
end
