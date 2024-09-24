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
        mov al, [.Y]
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

; ax = ID
; cx = X
; dx = Y
Buffer_Display_Image:
    xor bx, bx

    .Find_ID:
        mov cx, [Image_Buffer_ID_Table + bx + 2]
        cmp ax, cx
        je .Get_Block

        add bx, 0x04
        cmp bx, 0x400
        jl .Find_ID
        ret

    .Get_Block:
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
        mov bx, ax
        xor cx, cx
        add bx, cx

    .Display_Loop:
        inc cx

        pusha
        push ds
        mov ax, [Memory_Map.Image_Buffer]
        mov ds, ax
        mov al, [bx]
        pop ds
        ;mov cx, bx
        xor dx, dx
        ;mov cx, 10
        ;mov dx, 10
        call VGA_Place_Pixel
        popa

        cmp cx, 0x100
        jl .Display_Loop
        ret

    .X_Current dw 0x0000
    .Y_Current dw 0x0000

Buffer_Debug:
    times 0x100 db 0x00

Image_Buffer_ID_Table:
    times 0x400 db 0x00

Image_Buffer_Block_Table:
    times 0x100 db 0x00