Init_Memory:
    ; Get lower memory
    clc
    int 0x12
    jc .Int_x12_Fail

    ; Convert kilobytes into segments
    xor dx, dx
    mov cx, 0x40
    div cx
    mov cx, 0x0A

    ; Get Current Memory
    mov bx, ds
    shr bx, 0x0C
    inc bx

    ; See how many segments can be allocated (must be at least 3)
    sub ax, bx
    cmp ax, 0x02
    jl .Low_Memory_Error

    ; Set the map
    mov ax, ds
    mov [Memory_Map.Main_Program], ax
    add ax, 0x1000
    mov [Memory_Map.Alloc_Memory], ax
    add ax, 0x1000
    mov [Memory_Map.BMP_Storage], ax
    ret

    .Int_x12_Fail:
        mov ah, 0x09
        mov dx, .Int_x12_Fail_Msg
        int 0x21

        mov ah, 0x4C
        int 0x21

    .Low_Memory_Error:
        mov ah, 0x09
        mov dx, .Low_Memory_Error_Msg
        int 0x21

        mov ah, 0x4C
        int 0x21

    .Int_x12_Fail_Msg db "Error, failed to use int 12, exiting...$"
    .Low_Memory_Error_Msg db "Error, not enough memory to allocate, exiting...$"

Memory_Map:
    .Main_Program   dw 0x0000 ; For the current program
    .Alloc_Memory   dw 0x0000 ; For save file, chunks, layers and ect
    .BMP_Storage    db 0x0000 ; For current BMPs