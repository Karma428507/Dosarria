VGA_Init:
    ; 320*200 screen with 256 colors
    mov ah, 0x00
    mov al, 0x13
    int 0x10
    
    ; Clear 0xB000 (the drawing buffer)
    call VGA_Clear_Buffer
    ret

VGA_Transfer:
    push es
    push ds

    ; Copy main buffer
    mov ax, 0xB000
    mov ds, ax
    mov ax, 0xA000
    mov es, ax

    xor si, si
    xor di, di
    mov ecx, 320*200
    rep movsb
    pop ds
    pop es

    ; Clear second buffer
    call VGA_Clear_Buffer
    ret

VGA_Clear_Buffer:
    push es
    mov ax, 0xB000
    mov es, ax

    xor di, di
    xor eax, eax
    mov ecx, (320*200)/4
    rep stosd

    pop es
    ret

VGA_Sync:

; cx = x
; dx = y
; al = color
VGA_Place_Pixel:
    pusha
    push ax
    mov si, cx

    mov ax, 320
    mul dx
    add si, ax

    push ds
    mov ax, 0xB000
    mov ds, ax

    pop ax
    mov [si], al
    pop ds
    popa
    ret

; ah = W
; al = H
; bl = Color
; cx = X
; dx = Y
Draw_Cube:
    mov [.W], ah
    mov [.H], al
    mov [.X], cx
    mov [.Y], dx

    mov al, bl
    xor bx, bx

    mov bl, [.W]
    mov dx, [.Y]

    .Loop_Row:
        mov cx, [.X]

        push bx
        mov bl, [.H]

        .Loop_Col:
            call VGA_Place_Pixel
            
            test bl, bl
            jz .Col_End

            dec bl
            inc cx
            jmp .Loop_Col
        
        .Col_End:
            pop bx

            test bl, bl
            jz .Row_End

            dec bl
            inc dx
            jmp .Loop_Row

    .Row_End:
        ret

    .X dw 0x00
    .Y dw 0x00
    .W db 0x00
    .H db 0x00