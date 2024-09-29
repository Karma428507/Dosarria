; ah = Width
; al = Height
; bl = Color
; bh = Page (later)
; cx = X
; dx = Y
Add_Object:
    pusha
    mov [.Storage], bx
    ; Find the page
    xor bx, bx
    add bx, [.Offset]
    
    ; Load the data
    mov [Object_List + bx + OBJECT_INFO_STRUCT.W], ah
    mov [Object_List + bx + OBJECT_INFO_STRUCT.H], al
    mov [Object_List + bx + OBJECT_INFO_STRUCT.X], cx
    mov [Object_List + bx + OBJECT_INFO_STRUCT.Y], dx
    mov ax, [.Storage]
    mov [Object_List + bx + OBJECT_INFO_STRUCT.Color], al

    pusha
    mov ah, 0x0E
    mov al, [Object_List + bx + OBJECT_INFO_STRUCT.Color]
    add al, 0x30
    ;int 0x10
    popa

    add bx, 0x08
    mov [.Offset], bx
    popa
    ret

    .Storage: dw 0x0000
    .Offset: dw 0x0000

; ax = Index
; bh = Page
Draw_Object:
    pusha
    mov dx, 0x08
    mul dx

    ;xor ax, ax
    mov bx, ax

    ; ah = W
    ; al = H
    ; bl = Color
    ; cx = X
    ; dx = Y

    mov ah, [Object_List + bx + OBJECT_INFO_STRUCT.W]
    mov al, [Object_List + bx + OBJECT_INFO_STRUCT.H]
    mov cx, [Object_List + bx + OBJECT_INFO_STRUCT.X]
    mov dx, [Object_List + bx + OBJECT_INFO_STRUCT.Y]

    mov [.Excess], ax
    mov al, [Object_List + bx + OBJECT_INFO_STRUCT.Color]
    mov bx, ax
    mov ax, [.Excess]

    ; Draw cube
    call Draw_Cube

    popa
    ret

    .Excess: dw 0x0000

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

    mov bl, [.H]
    mov dx, [.Y]

    .Loop_Row:
        mov cx, [.X]

        push bx
        xor bx, bx
        mov bl, [.W]

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

    .X dw 0x0000
    .Y dw 0x0000
    .W db 0x00
    .H db 0x00

; Add layers later when allocations added
; Delete later when I'm done testing with this
Object_List:
    times 0x1400 * 5 db 0

OBJECT_INFO_STRUCT:
    .W          equ 0x00
    .H          equ 0x01
    .X          equ 0x02
    .Y          equ 0x04
    .Color      equ 0x06
    .Reserved   equ 0x07