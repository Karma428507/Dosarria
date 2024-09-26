; ah = Width
; al = Height
; bl = Color
; cx = X
; dx = Y
Add_Object:
    ret

; Add layers later when allocations added
Object_List:
    times 0x1000 db 0

; 16 bytes
OBJECT_INFO:
    .W db 0x00
    .H db 0x00
    .X dw 0x0000
    .Y dw 0x0000
    .Color db 0x00
    .Reserved db 0x00