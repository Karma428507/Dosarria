VGA_Init:
    ; Allocate memory WIP
    mov ah, 0x48
    mov bx, 0x1000
    int 0x21

    ; 320*200 screen with 256 colors
    mov ah, 0x00
    mov al, 0x13
    int 0x10
    ret

VGA_Sync:

; ax = x
; bx = y
; cx = w
; dx = h
Draw_Cube:
    mov [.X], ax
    mov [.Y], bx
    mov [.W], cx
    mov [.H], dx

    mov ah, 0x0C
    mov al, 0x20

    mov bx, [.W]
    mov dx, [.Y]

    .Loop_Row:
        mov cx, [.X]

        push bx
        mov bx, [.H]

        .Loop_Col:
            push bx
            xor bx, bx
            int 0x10
            pop bx
            
            test bx, bx
            jz .Col_End

            dec bx
            inc cx
            jmp .Loop_Col
        
        .Col_End:
            pop bx

            test bx, bx
            jz .Row_End

            dec bx
            inc dx
            jmp .Loop_Row

    .Row_End:
        ret

    .X dw 0x00
    .Y dw 0x00
    .W dw 0x00
    .H dw 0x00