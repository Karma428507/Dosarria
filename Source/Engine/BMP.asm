; ax = file name
Load_BMP:
    mov [.File_Address], ax
    mov bx, 0x0E
    call Load_File
    jc .Fail_Sign

    ; Get the signature
    push ds
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    xor bx, bx
    mov ax, [bx]
    pop ds

    ; Check signature
    mov bx, [.BMP_Sign]
    cmp ax, bx
    jne .Fail_Sign

    ; Get file size
    push ds
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    mov bx, 0x02
    mov bx, [bx]
    pop ds

    ; Get the full file
    mov ax, [.File_Address]
    call Load_File
    jc .Fail_Sign

    ; Load offset
    push ds
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    mov bx, 0x0A
    mov eax, [bx]
    mov [.Image_Offset], eax
    pop ds

    ; Get image size
    push ds
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    mov bx, 0x22
    mov ax, [bx]
    pop ds

    ; Ensures it uses enough allocated blocks
    test ax, ax
    jz .Fail_Corrupted

    xor dx, dx
    mov cx, 0x100
    div cx

    test dx, dx
    jz .Set_Size
    inc ax

    .Set_Size:
        mov [.Size], ax

    ; Get width and height
    push ds
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    mov bx, 0x16
    mov cx, [bx]
    mov dx, [bx + 4]
    pop ds

    ; Check if sizes are valid
    test cx, cx
    jz .Fail_Corrupted
    test dx, dx
    jz .Fail_Corrupted

    ; Save sizes
    dec cx
    dec dx
    mov [.Size_X], cl
    mov [.Size_Y], dl

    ; Load current data onto the table
    mov bx, 255 * 4

    .Table_Element_Find:
        mov eax, [BMP_ID_Table + bx]
        jz .Table_Add_Element
        
        sub bx, 4
        jnz .Table_Element_Find

        ; Error
        ret

    .Table_Add_Element:
        mov ax, [.File_Address]
        shl eax, 16

        mov ah, [.Size_X]
        mov al, [.Size_Y]
        mov [BMP_ID_Table + bx], eax
        
    ; Copy the image data to the BMP segment
    mov ax, bx
    shl ax, 0x02
    mov bx, 256
    mov dl, [.Size]

    .Table_Define_Chunks:
        dec bx
        mov ah, [BMP_Allocation_Table + bx]
        test ah, ah
        jnz .Continue

        mov [BMP_Allocation_Table + bx], al

        ; Copy the data
        pusha
        mov ax, bx
        mov dx, 0x100
        mul dx
        mov si, ax
        mov di, [.Image_Offset]

        mov ax, [Memory_Map.File_Storage]
        mov es, ax
        mov ax, [Memory_Map.BMP_Storage]
        mov ds, ax

        mov cx, 256
        rep stosd
        popa

        dec dl
        test dl, dl
        jz .Copy

        .Continue:
            jnz .Table_Define_Chunks

            ; Error
            ret

    .Copy:
        

    mov ah, 0x0E
    mov al, 'p'
    int 0x10
    ret

    .Fail_Sign:
        mov ah, 0x0E
        mov al, 'e'
        int 0x10
        ret

    .Fail_Corrupted:
        ret

    .BMP_Sign db "BM"

    .Image_Offset dd 0x00000000
    .File_Address dw 0x0000
    .Size_X db 0x00
    .Size_Y db 0x00
    .Size db 0x00

Display_Graphics:
    mov ax, [.File_Address]
    mov bx, 255 * 4

    .Find_ID:
        mov cx, [BMP_ID_Table + bx]
        cmp ax, cx
        je .Get_Block

        sub bx, 0x04
        jnz .Find_ID
        ret

    .Get_Block:
        mov ax, bx
        shl ax, 0x02
        mov bx, 0x100

    .Find_Block:
        dec bx

        mov cl, [BMP_Allocation_Table + bx]
        test al, cl
        je .Display

        test bx, bx
        jnz .Find_Block
        ret

    .Display:
        mov ax, bx
        mov dx, 0x100
        mul dx
        mov bx, ax
        mov cx, 0x100
        add bx, cx

    .Display_Loop:
        dec cx

        pusha
        push ds
        mov ax, [Memory_Map.BMP_Storage]
        mov ds, ax
        mov al, 4;[bx]
        pop ds
        mov cx, 0;bx
        xor dx, dx
        call VGA_Place_Pixel
        popa

        test cx, cx
        jne .Display_Loop

    ret

    .File_Address dw 0x0000

; 2 bytes for filepath, 1 for width, 1 for height
BMP_ID_Table:
    times 0x400 db 0x00

; Each byte will refer to an index in the ID table
BMP_Allocation_Table:
    times 0x100 db 0x00

DEBUG: db "ASSETS\TEST.BMP", 0x00