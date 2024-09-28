%define VGA_BACKGROUND 0x03030303

; Remember the colors
; RRRGGGBB

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
    mov eax, VGA_BACKGROUND
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