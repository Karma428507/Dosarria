; ax = ID
; bl = Color
Buffer_Draw_Cube:
    ; Create the cube
    push ax
    mov al, bl
    mov bx, 0x100

    .Loop:
        dec bx
        mov [Buffer_Debug + bx], al
        test bx, bx
        jz .Loop

    pop ax

    ; Turn the cube into an image
    mov cx, 0x10
    mov dx, 0x10
    mov si, Buffer_Debug
    call Buffer_Add_Image
    ret

; ds:si = Buffer
; ax = ID
; cx = Width
; dx = Height
Buffer_Add_Image:
    ; Save buffer
    pusha
    mov ax, ds
    mov bx, [Memory_Map.Main_Program]
    mov ds, bx
    mov [.Buffer_Segment], ax
    mov [.Buffer_Offset], si
    popa

    ; Save variables
    mov [.ID], ax
    mov [.X], cx
    mov [.Y], dx

    ; Save the amount of blocks needed
    ; ((X * Y) / 0x100) + (((X * Y) % 0x100) == 0 ? 0 : 1)
    mov ax, cx
    mul dx
    xor dx, dx
    mov cx, 0x100
    div cx

    ; Increase by one if there's a remainder
    test dx, dx
    jz .Set_Size
    inc ax  
    .Set_Size:
        mov [.Size], ax

    ; Load current data onto the table
    xor bx, bx

    .Table_Element_Find:
        mov eax, [Image_Buffer_ID_Table + bx]
        jz .Table_Add_Element
        
        add bx, 4
        cmp bx, 0x400
        jl .Table_Element_Find

        mov ah, 0x0E
        mov al, 'e'
        int 0x10
        ret

    .Table_Add_Element:
        mov ax, [.ID]
        shl eax, 16

        mov ah, [.X]
        dec ah
        mov al, [.Y]
        dec al
        mov [Image_Buffer_ID_Table + bx], eax

    ; Copy the image data to the BMP segment
    mov ax, bx
    shr ax, 0x02
    xor bx, bx
    mov dx, [.Size]

    pusha
    mov ah, 0x0E
    mov al, dl
    add al, 0x30
    int 0x10
    popa
    ;ret

    .Table_Define_Chunks:
        inc bx
        mov ah, [Image_Buffer_Block_Table + bx]
        test ah, ah
        jnz .Continue

        mov [Image_Buffer_Block_Table + bx], al

        ; Copy the data
        pusha
        mov ax, bx
        mov dx, 0x100
        mul dx
        mov di, ax
        mov si, [.Buffer_Offset]

        push es
        push ds

        mov ax, [Memory_Map.Image_Buffer]
        mov es, ax
        mov ax, [.Buffer_Segment]
        mov ds, ax
        mov cx, 256
        rep stosd

        pop ds
        pop es
        popa

        dec dx
        test dx, dx
        jz .End

        .Continue:
            cmp bx, 256
            jnz .Table_Define_Chunks

            ; Error
            ret

    .End:
        ret

    .Buffer_Segment dw 0x0000
    .Buffer_Offset dw 0x0000
    .Size dw 0x0000
    .ID dw 0x0000
    .X dw 0x0000
    .Y dw 0x0000

; This... I hate
; I would put the AM speech to convey my hate but it would be too much effort for something I hate this much
; Why won't you work function
; ax = ID
; cx = X
; dx = Y
Buffer_Display_Image:
    ; This is very important
    ; IDK how but it is
    ;pusha
    ;mov al, 0x50
    ;xor cx, cx
    ;xor dx, dx
    ;call VGA_Place_Pixel
    ;popa

    xor bx, bx
    ;mov [.X_Current], cx
    ;mov [.Y_Current], dx

    .Find_ID:
        mov cx, [Image_Buffer_ID_Table + bx + 2]
        cmp ax, cx
        je .Get_Block

        add bx, 0x04
        cmp bx, 0x400
        jl .Find_ID
        
        mov al, 0x10
        xor cx, cx
        inc cx
        xor dx, dx
        call VGA_Place_Pixel
        ret

    .Get_Block:
        xor cx, cx
        xor dx, dx
        mov cl, [Image_Buffer_ID_Table + bx + 1]
        mov dl, [Image_Buffer_ID_Table + bx]
        inc cx
        inc dx
        mov [.Width], cx
        mov [.Height], dx

        ;pusha
        ;mov al, 0x30
        ;xor cx, cx
        ;inc cx
        ;xor dx, dx
        ;inc dx
        ;call VGA_Place_Pixel
        ;popa

        mov ax, bx
        shl ax, 0x02
        xor bx, bx

    .Find_Block:
        mov cl, [Image_Buffer_Block_Table + bx]
        test al, cl
        je .Display

        inc bx
        cmp bx, 0x100
        jl .Find_Block
        ret

    .Display:
        mov ax, bx
        mov dx, 0x100
        mul dx
        mov si, ax
        xor cx, cx
        add si, cx

        call Restore_Segments
        mov word [.X_Current], 20

    .Display_Loop:
        inc bx
        inc cx

        pusha
        push es
        mov ax, [Memory_Map.Image_Buffer]
        mov es, ax
        mov al, [si]
        pop es
        ;mov cx, bx
        ;xor dx, dx
        add cx, [.X_Current]
        ;mov dx, [.Y_Current]
        ;add cx, [.Width];10
        mov dx, [.Y_Current];10
        call VGA_Place_Pixel
        popa

        cmp cx, 0x100
        jl .Display_Loop
        ret

    .X_Current dd 0x0000
    .Y_Current dd 0x0000
    .Height dd 0x0000
    .Width dd 0x0000

Buffer_Debug:
    times 0x100 db 0x00

Image_Buffer_ID_Table:
    times 0x400 db 0x00

Image_Buffer_Block_Table:
    times 0x100 db 0x00