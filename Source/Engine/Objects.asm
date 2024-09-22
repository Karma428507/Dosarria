Add_Object:
    ret

; Add layers later when allocations added
Object_List:
    times 0x1000 db 0

; 16 bytes
OBJECT_INFO:
    .FLAGS      equ 0x00
    .BMP_SEG    equ 0x02
    .BMP_OFF    equ 0x04
    .ENV_POS    equ 0x06
    .WIDTH      equ 0x0A
    .HIGHT      equ 0x0B
    .RESERVED   equ 0x0C