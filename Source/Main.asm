[bits 16]
[org 0x100]

jmp Start

%include "Source/Engine/BMP.asm"
%include "Source/Engine/Image_Storage.asm"
%include "Source/Engine/Input.asm"
%include "Source/Engine/Layers.asm"
%include "Source/Engine/Objects.asm"

%include "Source/Systems/Files.asm"
%include "Source/Systems/Memory.asm"
%include "Source/Systems/Sound.asm"
%include "Source/Systems/Threading.asm"
%include "Source/Systems/Timer.asm"
%include "Source/Systems/VGA.asm"

Start:
    ; Setup screen
    call VGA_Init

    ;call Init_Memory

    mov ah, 0x0A
    mov al, 0x0A
    mov bl, 0x09
    mov bh, 0x00
    mov cx, 0x20
    mov dx, 0x20
    call Add_Object

    mov ax, 0x00
    call Draw_Object
    call VGA_Transfer
    jmp $

    .Draw:
        mov ax, 0x00
        call Draw_Object
        call VGA_Transfer
        jmp .Draw

    ; Setup basic threads
    mov ax, Thread_Action_Input
    ;call Thread_Add

    ;jmp $
    ret

    .Loop:
        call Thread_Run
        jmp .Loop

    jmp Exit

    .Main_Loop:
        mov ah, 0x00
        int 0x16

        cmp al, 0x1B
        je .Escape

        cmp al, 'w'
        je .Up
        cmp al, 's'
        je .Down
        cmp al, 'a'
        je .Left
        cmp al, 'd'
        je .Right
        
        jmp .Main_Loop

    .Up:
        mov ax, [.Coord_Y]
        dec ax
        mov [.Coord_Y], ax
        jmp .Refresh

    .Down:
        mov ax, [.Coord_Y]
        inc ax
        mov [.Coord_Y], ax
        jmp .Refresh
    
    .Left:
        mov ax, [.Coord_X]
        dec ax
        mov [.Coord_X], ax
        jmp .Refresh

    .Right:
        mov ax, [.Coord_X]
        inc ax
        mov [.Coord_X], ax
        jmp .Refresh

    .Refresh:
        mov ax, [.Coord_X]
        mov bx, [.Coord_Y]
        mov cx, 20
        mov dx, 20
        call Draw_Cube
        call VGA_Transfer
        jmp .Main_Loop

    .Escape:
        mov ah, 0x00
        mov al, 0x03
        int 0x10
        ret

    .Coord_X dw 0x0A
    .Coord_Y dw 0x0A
    
Exit:
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    ret