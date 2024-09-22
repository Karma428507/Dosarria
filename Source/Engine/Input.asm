; Loads the action of a key to a function
; al = key
; bx = address
Register_Input_Action:
    pusha
    pusha
    
    ; Check if it hadn't reached the limit
    mov ax, [Input_Vector_Table_Index]
    cmp ax, 0x20
    jge .End

    ; Multiply to get the offset
    mov dx, 0x03
    mul dx
    mov [.IVT_Offset], ax

    ; Increase the index
    mov ax, [Input_Vector_Table_Index]
    inc ax
    mov [Input_Vector_Table_Index], ax
    popa

    ; Get the address
    mov si, Input_Vector_Table
    add si, [.IVT_Offset]
    mov [si], al
    inc si
    mov [si], bx
    popa
    ret

    .End:
        popa
        popa
        ret

    .IVT_Offset dw 0x0000

Thread_Action_Input:
    call Reload_Input_Buffer

    mov bx, 0x08
    mov ah, 0x0E

    .Find_Keypresses:
        dec bx
        call .Print_Letter

        test bx, bx
        jnz .Find_Keypresses

    .End:
        ret

    .Print_Letter:
        mov al, [Input_Buffer + bx]
        test al, al
        jz .End

        int 0x10
        mov al, ' '
        int 0x10
        ret

Reload_Input_Buffer:
    ; Get new keyboard actions
    mov dx, 0x64
    in al, dx

    and al, 1
    jz .End

    mov dx, 0x60
    in al, dx

    mov ah, al
    and ah, 0x80
    jnz .Remove

    call Convert_Scancode_To_ASCII
    mov bx, 0x08

    .Loop_Check:
        dec bx

        mov ah, [Input_Buffer + bx]
        cmp al, ah
        je .End

        test cl, cl
        jnz .Loop_Check

    mov bx, 0x08

    .Loop_Fill:
        dec bx

        mov ah, [Input_Buffer + bx]
        jz .End

        test cl, cl
        jnz .Loop_Check
        jmp .End

    .Add_Value:
        mov [Input_Buffer + bx], al
        jmp .End

    .Remove:
        xor al, 0x80
        call Convert_Scancode_To_ASCII
        mov bx, 0x08

        .Loop_Remove:
            dec bx

            mov ah, [Input_Buffer + bx]
            cmp al, ah
            je .Remove_Element

            test cl, cl
            jnz .Loop_Remove
            jmp .End

        .Remove_Element:
            mov byte [Input_Buffer + bx], 0x00

    .End:
        ret

Convert_Scancode_To_ASCII:
    xor bx, bx
    mov bl, al
    mov al, [Scancode + bx]
    ret

; Not to be confused with 'Interrupt Vector Table'
; Handles 32 keys
Input_Vector_Table:
    times 0x03 * 0x20 db 0x00

Input_Vector_Table_Index: dw 0x0000

Input_Buffer:
    dq 0x0000000000000000

Scancode:
    db 0, 0x2B, "1234567890-=", 0, 0
    db "qwertyuiop[]", 0xA, 0
    db "asdfghjkl;'`", 0
    db "\zxcvbnm,./", 0
    db '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0
    db 0, 0, 0, 0, 0, 0, 0, 0, 0