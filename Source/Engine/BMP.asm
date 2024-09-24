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

DEBUG: db "ASSETS\TEST.BMP", 0x00