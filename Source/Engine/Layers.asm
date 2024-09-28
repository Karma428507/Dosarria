%define LAYER_FLAG_GRID 1<<0
%define LAYER_COLLISION_SEND 1<<1
%define LAYER_COLLISION_READ 1<<2
%define LAYER_CLICKABLE 1<<3

Init_Layers:
    ret

Thread_Visualize_Layers:
    ret

; Work later
; (Will ONLY refer to the player for now)
Single_Collision:
    ret

Layer_Idents:
    .Sky        db 0x00 ; Might not be needed if I can't draw clouds
    .Wall       db LAYER_FLAG_GRID
    .Blocks     db LAYER_FLAG_GRID | LAYER_COLLISION_SEND
    .Entities   db LAYER_COLLISION_READ
    .UI         db LAYER_CLICKABLE