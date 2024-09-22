; ax = File name
; bx = Size
Load_File:
    clc
    pusha
    mov dx, ax
    mov cx, bx

    ; Open the file
    mov ah, 0x3D
    mov al, 0x00
    int 0x21
    jc .Fail
    mov bx, ax

    push ds
    ; Set Buffer Address
    mov ax, [Memory_Map.File_Storage]
    mov ds, ax
    xor dx, dx
    
    ; Setup initial data
    mov ah, 0x3F
    mov al, 0x00
    int 0x21
    pop ds

    ; Close the file
    mov ah, 0x3E
    mov al, 0x00
    int 0x10
    popa
    ret

    .Fail:
        popa
        ret