Thread_Run:
    mov bx, [Thread_Table_Index]
    shl bx, 1<<0

    .Loop:
        sub bx, 0x02
        mov ax, [Thread_Table + bx]

        pusha
        call ax
        popa

        test bx, bx
        jnz .Loop

    ret

Thread_Add:
    pusha
    pusha
    
    ; Check if it hadn't reached the limit
    mov ax, [Thread_Table_Index]
    cmp ax, 0x40
    jge .End

    ; Multiply to get the offset
    mov dx, 0x02
    mul dx
    mov [.Thread_Offset], ax

    ; Increase the index
    mov ax, [Thread_Table_Index]
    inc ax
    mov [Thread_Table_Index], ax
    popa

    ; Get the address
    mov si, Thread_Table
    add si, [.Thread_Offset]
    mov [si], ax
    popa
    ret

    .End:
        popa
        popa
        ret

    .Thread_Offset dw 0x0000

Thread_Remove:
    ret

Thread_Table:
    times 0x80 db 0x00

Thread_Table_Index: dw 0x0000