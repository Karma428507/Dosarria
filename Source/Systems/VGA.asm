VGA_Init:
    ; Allocate memory WIP

    ; 320*200 screen with 256 colors
    mov ah, 0x00
    mov al, 0x13
    int 0x10
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
    ; Clear second buffer
    mov ax, 0xB000
    mov es, ax

    xor di, di
    xor eax, eax
    mov ecx, (320*200)/4
    rep stosd

    pop ds
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

; ax = x
; bx = y
; cx = w
; dx = h
Draw_Cube:
    mov [.X], ax
    mov [.Y], bx
    mov [.W], cx
    mov [.H], dx

    mov al, 0x20

    mov bx, [.W]
    mov dx, [.Y]

    .Loop_Row:
        mov cx, [.X]

        push bx
        mov bx, [.H]

        .Loop_Col:
            call VGA_Place_Pixel
            
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