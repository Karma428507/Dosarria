#include <DOS.H>

/* CONSTANTS */
#define VIDEO 				0xA0000000
#define TEMP  				0xB0000000
#define TEXT  				0xB8000000
#define SCREEN				0xFA00 / 0x02
#define NULL				0

#define FLAG_2D_GRID 		1<<0
#define FLAG_COLLIDE_BASE	1<<1
#define FLAG_COLLIDE_REACT	1<<2
#define FLAG_UI				1<<3

#define BLOCK_TYPE_NULL		0x00
#define BLOCK_TYPE_DIRT		0x01
#define BLOCK_TYPE_GRASS	0x02
#define BLOCK_TYPE_PINK		0x03
#define BLOCK_TYPE_BLACK	0x04
#define BLOCK_TYPE_ORANGE	0x05

#define OBJECT_ID_NULL		0x00
#define OBJECT_ID_PLAYER	0x01

#define OBJECT_FLAG_PRESENT	1<<0

#define THREAD_FLAG_ENABLED	1<<0
#define THREAD_FLAG_TICK	1<<1
#define THREAD_FLAG_LIMIT	1<<2
#define THREAD_FLAG_ZERO	1<<3
#define THREAD_FLAG_TIMED	1<<4
#define THREAD_FLAG_WHILE	1<<5
#define THREAD_FLAG_AFTER	1<<6
#define THREAD_FLAG_DESTROY	1<<7

#define COLOR_PICKER(R, G, B) ((R & 7) << 5) | ((G & 7) << 2) | (B & 3)

/* TYPES */
typedef unsigned char 	uint_8;
typedef unsigned short 	uint_16;
typedef unsigned int   	uint_32;

/* ENUMS */
enum DIRECTION {
	DIRECTION_TOP,
	DIRECTION_BOTTOM,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
};

/* STRUCTURES */
typedef struct {
	uint_16 ADDRESS;
	uint_16 SIZE;
	uint_8 FLAGS;
} LAYER_PROPERTIES;

typedef struct {
	uint_8 ID;
	uint_8 FLAGS;
	uint_8 COLOR;
	uint_8 W;
	uint_8 H;
	uint_16 X;
	uint_16 Y;
} OBJECT_PROPERTY;

typedef struct {
	uint_8 ID;
	char X_OFFSET;
	char Y_OFFSET;
} OBJECT_UPDATE;

typedef struct {
	char KEY;
	void *FUNCTION;
} KEYBIND;

typedef struct {
	uint_8 FLAGS;
	int TIMER;
	void *MAIN_FUNC;
	void *SUB_FUNC;
} THREAD_PROPERTY;

/* VGA */
void VGA_MODE(uint_8);
void VGA_SWAP();
void VGA_LOAD_TERMINAL();
void VGA_PLACE_CHARACTER(uint_8);
void VGA_PRINT(const char *);
void VGA_PLACE(uint_8, uint_16, uint_16);

/* DRAWING */
void DRAW_CUBE(uint_8, uint_8, uint_8, uint_16, uint_16);
void DRAW_OBJECT(OBJECT_PROPERTY);
void DRAW_GRID(char *);

/* LAYERS/OBJECTS */
void INIT_LAYERS();
void LAYER_THREAD();
char LAYER_CHECK_COLLISION(uint_8, enum DIRECTION);
void CREATE_OBJECT(OBJECT_PROPERTY, uint_8, uint_16);
void SEND_OBJECT_UPDATE(uint_8, char, char);

/* INPUT */
void interrupt ISR_KEYBOARD(void);
void interrupt ISR_MOUSE(void);
uint_8 GET_CHARACTER();
void INIT_MOUSE();
void REGISTER_KEY_INPUT(uint_8, void *);
void KEYBOARD_THREAD();

/* SOUND (if there's time) */

/* THREADS/TIMER */
void interrupt ISR_TIMER(void);
void ADD_THREAD_ENTRY(THREAD_PROPERTY ENTRY);
void REMOVE_THREAD_ENTRY(int INDEX);
void ADD_THREAD(void *);
void ADD_LIMITED_THREAD(int, void *);
void ADD_TIMER_WHILE_THREAD(int, void *);
void ADD_TIMER_POST_THREAD(int, void *);
void ADD_TIMER_THREAD(int, void *, void *);
void RUN_NEXT_TICK(void *);

/* FILE MANAGEMENT */

/* WORLD GEN */
void WORLD_GENERATE_SIMPLE();

/* ARRAYS */
LAYER_PROPERTIES LAYERS[2];

char TILE_BLOCK_ARRAY[640];
OBJECT_PROPERTY ENTITY_LIST[0x40];
OBJECT_UPDATE UPDATE_LIST[0x20];

KEYBIND KEYBINDS[0x10];
char CURRENT_KEYS[0x10];
uint_32 KEYBINDS_INDEX = 0;
uint_32 CURRENT_KEYS_INDEX = 0;
THREAD_PROPERTY THREAD_LIST[0x40];

/* MISC */
void interrupt (*ISR_KEYBOARD_OLD)(void);
void interrupt (*ISR_MOUSE_OLD)(void);
void interrupt (*ISR_TIMER_OLD)(void);

uint_8 EXIT = 0, JUMPING = 0, PLAYER_COLOR = COLOR_PICKER(7, 0, 0);
int COLOR_PICKED = BLOCK_TYPE_GRASS, VGA_OFFSET = 0;
int X_MOUSE = 0, Y_MOUSE = 0;

/* ALL FUNCTIONS */
void Block_Change_1() {
	COLOR_PICKED = BLOCK_TYPE_GRASS;
}

void Block_Change_2() {
	COLOR_PICKED = BLOCK_TYPE_DIRT;
}

void Block_Change_3() {
	COLOR_PICKED = BLOCK_TYPE_PINK;
}

void Block_Change_4() {
	COLOR_PICKED = BLOCK_TYPE_BLACK;
}

void Block_Change_5() {
	COLOR_PICKED = BLOCK_TYPE_ORANGE;
}

void Player_Left() {
	if (!LAYER_CHECK_COLLISION(OBJECT_ID_PLAYER, DIRECTION_LEFT))
		SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, -1, 0);
}

void Player_Right() {
	if (!LAYER_CHECK_COLLISION(OBJECT_ID_PLAYER, DIRECTION_RIGHT))
		SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, 1, 0);
}

void _Sub_Jump_While() {
	if (!LAYER_CHECK_COLLISION(OBJECT_ID_PLAYER, DIRECTION_TOP))
		SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, 0, -1);
}

void _Sub_Jump_Post() {
	JUMPING &= 2;
}

void Player_Jump() {
	if (JUMPING & 2)
		return;

	JUMPING = 3;
	ADD_TIMER_THREAD(825, (void *)&_Sub_Jump_While, (void *)&_Sub_Jump_Post);
}

void Player_Fall() {
	if (JUMPING & 1)
		return;

	if (!LAYER_CHECK_COLLISION(OBJECT_ID_PLAYER, DIRECTION_BOTTOM))
		SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, 0, 1);
	else
		JUMPING = 0;
}

void Exit_Loop() {
	EXIT = 1;
}

int main() {
	OBJECT_PROPERTY Plr = {OBJECT_ID_PLAYER, OBJECT_FLAG_PRESENT, 0, 20, 30, 40, 40};
	/*uint_8 Color_Player = 0;*/
	void (* THREAD)(void);
	int i = 0;

	VGA_LOAD_TERMINAL();
	VGA_PRINT("==========\n$");
	VGA_PRINT("|DOSARRIA|\n$");
	VGA_PRINT("==========\n\n$");

	VGA_PRINT("FINDING MOUSE DRIVER...\n$");
	INIT_MOUSE();
	VGA_PRINT("MOUSE INITALIZED\n\n$");

	/*VGA_PRINT("ENTER PLAYER COLOR (R, G, B)$");
	PLAYER_COLOR:
		VGA_PRINT("\n> $");
		Color_Player = GET_CHARACTER();
		VGA_PLACE_CHARACTER(Color_Player);
		switch(Color_Player) {
			case 'R':
				Plr.COLOR = COLOR_PICKER(7, 0, 0);
				break;
			case 'g':
				Plr.COLOR = COLOR_PICKER(0, 7, 0);
				break;
			case 'b':
				Plr.COLOR = COLOR_PICKER(0, 0, 3);
				break;
			default:
				goto PLAYER_COLOR;
		}*/

	Plr.COLOR = COLOR_PICKER(7, 0, 0);

	VGA_PRINT("\n\nLOADING INTERRUPTS...\n$");
	ISR_KEYBOARD_OLD = getvect(0x09);
	ISR_MOUSE_OLD = getvect(0x74);
	ISR_TIMER_OLD = getvect(0x1C);
	setvect(0x09, ISR_KEYBOARD);
	setvect(0x74, ISR_MOUSE);
	setvect(0x1C, ISR_TIMER);

	REGISTER_KEY_INPUT('1', (void *)&Block_Change_1);
	REGISTER_KEY_INPUT('2', (void *)&Block_Change_2);
	REGISTER_KEY_INPUT('3', (void *)&Block_Change_3);
	REGISTER_KEY_INPUT('4', (void *)&Block_Change_4);
	REGISTER_KEY_INPUT('5', (void *)&Block_Change_5);
	REGISTER_KEY_INPUT('a', (void *)&Player_Left);
	REGISTER_KEY_INPUT('d', (void *)&Player_Right);
	REGISTER_KEY_INPUT(' ', (void *)&Player_Jump);
	REGISTER_KEY_INPUT(0x2B, (void *)&Exit_Loop);

	VGA_PRINT("\n\nLOADING GRAPHICS...\n$");
	VGA_MODE(0x13);
	INIT_LAYERS();
	WORLD_GENERATE_SIMPLE();

	/* Load Objects */
	CREATE_OBJECT(Plr, 0x01, 0x40);

	ADD_THREAD((void *)&LAYER_THREAD);
	ADD_THREAD((void *)&VGA_SWAP);
	ADD_THREAD((void *)&KEYBOARD_THREAD);
	ADD_THREAD((void *)&Player_Fall);

	while (!EXIT) {
		LAYER_THREAD();
		VGA_SWAP();
		KEYBOARD_THREAD();
		for (i = 0; i < 0x40; i++) {
			if (!(THREAD_LIST[i].FLAGS & THREAD_FLAG_ENABLED) || THREAD_LIST[i].FLAGS & THREAD_FLAG_TIMED)
				continue;

			THREAD = (void (*)(void))THREAD_LIST[i].MAIN_FUNC;
			THREAD();

			if (THREAD_LIST[i].FLAGS & THREAD_FLAG_LIMIT)
				if (--THREAD_LIST[i].TIMER <= 0)
					REMOVE_THREAD_ENTRY(i);
		}
	}

	setvect(0x09, ISR_KEYBOARD_OLD);
    setvect(0x74, ISR_MOUSE_OLD);
	setvect(0x1C, ISR_TIMER_OLD);
	VGA_MODE(0x03);
	return 0;
}

void LEAVE_FAIL() {
	short far *Text_Buffer = (short far *)TEXT;
	union REGS inp, outp;
	int i = 0;

	VGA_MODE(0x03);

	for (; i < 2000; i++) {
		Text_Buffer[i] = 0x0700;
	}

	inp.h.ah = 0x4C;
	inp.h.al = 0x01;
	int86(0x21, &inp, &outp);
}

void VGA_MODE(uint_8 MODE) {
	union REGS inp, outp;

	inp.h.ah = 0x00;
	inp.h.al = MODE;
	int86(0x10, &inp, &outp);
}

void VGA_SWAP() {
	int far *Video_Buffer = (int far *)VIDEO;
	int far *Temp_Buffer = (int far *)TEMP;
	int i = 0;

	for (; i < SCREEN; i++) {
		Video_Buffer[i] = Temp_Buffer[i];
		Temp_Buffer[i] = 0x03030303;
	}
}

void VGA_LOAD_TERMINAL() {
	short far *Text_Buffer = (short far *)TEXT;
	int i = 0;

	VGA_MODE(0x03);

	for (; i < 2000; i++) {
		Text_Buffer[i] = 0x0200;
	}
}

void VGA_PLACE_CHARACTER(uint_8 CHARACTER) {
	short far *Text_Buffer = (short far *)TEXT;
	Text_Buffer += VGA_OFFSET;
	VGA_OFFSET++;

	*Text_Buffer |= CHARACTER;
}

void VGA_PRINT(const char *MESSAGE) {
	int i = 0;

	for (; MESSAGE[i] != '$'; i++) {
		if (MESSAGE[i] == '\n') {
			VGA_OFFSET = ((VGA_OFFSET / 80) + 1) * 80; 
			continue;
		}

		VGA_PLACE_CHARACTER(MESSAGE[i]);
	}
}

void VGA_PLACE(uint_8 COLOR, uint_16 X, uint_16 Y) {
	uint_8 far *Temp_Buffer = (uint_8 far *)TEMP;
	Temp_Buffer[(Y * 320) + X] = COLOR;
}

void DRAW_CUBE(uint_8 COLOR, uint_8 W, uint_8 H, uint_16 X, uint_16 Y) {
	int i = 0, j;

	for (; i < W; i++) {
		for (j = 0; j < H; j++) {
			VGA_PLACE(COLOR, X + i, Y + j);
		}
	}
}

void DRAW_OBJECT(OBJECT_PROPERTY OBJECT) {
	DRAW_CUBE(OBJECT.COLOR, OBJECT.W, OBJECT.H, OBJECT.X, OBJECT.Y);
}

void DRAW_GRID(char *GRID) {
	int x = 0, y = 0, i = 0;

	for (; i < 640; i++) {
		if (GRID[i] == 0)
			continue;

		x = i % 32;
		y = i / 32;

		switch(GRID[i]) {
			case BLOCK_TYPE_DIRT:
				DRAW_CUBE(0x12, 0xA, 0xA, x * 10, y * 10);
				break;
			case BLOCK_TYPE_GRASS:
				DRAW_CUBE(0x90, 0xA, 0xA, x * 10, y * 10);
				break;
			case BLOCK_TYPE_PINK:
				DRAW_CUBE(0x81, 0xA, 0xA, x * 10, y * 10);
				break;
			case BLOCK_TYPE_BLACK:
				DRAW_CUBE(0x00, 0xA, 0xA, x * 10, y * 10);
				break;
			case BLOCK_TYPE_ORANGE:
				DRAW_CUBE(66, 0xA, 0xA, x * 10, y * 10);
				break;
			default:
				DRAW_CUBE(0x25, 0xA, 0xA, x * 10, y * 10);
				break;
		}
	}
}

void INIT_LAYERS() {
	LAYERS[0].ADDRESS = (uint_16)TILE_BLOCK_ARRAY;
	LAYERS[0].SIZE = sizeof(TILE_BLOCK_ARRAY);
	LAYERS[0].FLAGS = FLAG_2D_GRID | FLAG_COLLIDE_BASE;

	LAYERS[1].ADDRESS = (uint_16)ENTITY_LIST;
	LAYERS[1].SIZE = sizeof(ENTITY_LIST);
	LAYERS[1].FLAGS = FLAG_COLLIDE_REACT;
}

void LAYER_THREAD() {
	uint_8 far *Temp_Buffer = (uint_8 far *)TEMP;
	OBJECT_PROPERTY *OBJECT;
	int i = 0, j = 0, k = 0;

	for (; i < 2; i++) {
		if (LAYERS[i].FLAGS & FLAG_2D_GRID) {
			DRAW_GRID((char *)LAYERS[i].ADDRESS);
		} else {
			OBJECT = (OBJECT_PROPERTY *)LAYERS[i].ADDRESS;

			for (j = 0; j < 0x40; j++) {
				if (!(OBJECT[j].FLAGS & OBJECT_FLAG_PRESENT) || OBJECT[j].ID == 0)
					continue;
					
				for (k = 0; k < 0x20; k++) {
					if (OBJECT[j].ID == UPDATE_LIST[k].ID) {
						UPDATE_LIST[k].ID = 0;
						OBJECT[j].X += UPDATE_LIST[k].X_OFFSET;
						/*OBJECT[j].W += UPDATE_LIST[k].X_OFFSET;*/
						OBJECT[j].Y += UPDATE_LIST[k].Y_OFFSET;
						/*OBJECT[j].H += UPDATE_LIST[k].Y_OFFSET;*/
					}
				}

				DRAW_OBJECT(OBJECT[j]);
			}
		}
	}

	for (i = 0; i < 10; i++)
		for (j = 0; j < 10 - i; j++)
			Temp_Buffer[((Y_MOUSE + j) * 320) + (X_MOUSE + i)] = 0x7F;
}

char LAYER_CHECK_COLLISION(uint_8 ID, enum DIRECTION HIT_LINE) {
	uint_16 A = 0, B = 0, X = 0, Y = 0;
	int i = 0;
	
	for (; i < 0x40; i++) {
		if (!(ENTITY_LIST[i].FLAGS & OBJECT_FLAG_PRESENT) || ENTITY_LIST[i].ID != ID)
			continue;

		switch (HIT_LINE) {
			case DIRECTION_TOP:
				if (ENTITY_LIST[i].Y == 0)
					return 1;

				A = ENTITY_LIST[i].X / 10;
				B = (ENTITY_LIST[i].X + ENTITY_LIST[i].W - 1) / 10;
				Y = (ENTITY_LIST[i].Y - 1) / 10;

				if (TILE_BLOCK_ARRAY[(Y * 32) + A] != 0 || TILE_BLOCK_ARRAY[(Y * 32) + B] != 0)
					return 1;

				if (TILE_BLOCK_ARRAY[(Y * 32) + A + 1] != 0)
					return 1;

				break;
			case DIRECTION_BOTTOM:
				if (ENTITY_LIST[i].Y + ENTITY_LIST[i].H == 200)
					return 1;

				A = ENTITY_LIST[i].X / 10;
				B = (ENTITY_LIST[i].X + ENTITY_LIST[i].W - 1) / 10;
				Y = (ENTITY_LIST[i].Y + ENTITY_LIST[i].H) / 10;

				if (TILE_BLOCK_ARRAY[(Y * 32) + A] != 0 || TILE_BLOCK_ARRAY[(Y * 32) + B] != 0)
					return 1;

				if (TILE_BLOCK_ARRAY[(Y * 32) + A + 1] != 0)
					return 1;

				break;
			case DIRECTION_LEFT:
				if (ENTITY_LIST[i].X == 0)
					return 1;

				A = ENTITY_LIST[i].Y / 10;
				B = (ENTITY_LIST[i].Y + ENTITY_LIST[i].H - 1) / 10;
				X = (ENTITY_LIST[i].X - 1) / 10;

				if (TILE_BLOCK_ARRAY[(A * 32) + X] != 0 || TILE_BLOCK_ARRAY[(B * 32) + X] != 0)
					return 1;

				if (TILE_BLOCK_ARRAY[((A + 1) * 32) + X] != 0)
					return 1;

				break;
			case DIRECTION_RIGHT:
				if (ENTITY_LIST[i].X + ENTITY_LIST[i].W == 320)
					return 1;

				A = ENTITY_LIST[i].Y / 10;
				B = (ENTITY_LIST[i].Y + ENTITY_LIST[i].H - 1) / 10;
				X = (ENTITY_LIST[i].X + ENTITY_LIST[i].W) / 10;

				if (TILE_BLOCK_ARRAY[(A * 32) + X] != 0 || TILE_BLOCK_ARRAY[(B * 32) + X] != 0)
					return 1;
				
				if (TILE_BLOCK_ARRAY[((A + 1) * 32) + X] != 0)
					return 1;

				break;
		}
	}

	return 0;
}

/* Work on after these tasks: advanced allocation->file management->bmp handling->fonts->text elements */
void LAYER_CHECK_CLICK() {
	
}

void CREATE_OBJECT(OBJECT_PROPERTY OBJECT, uint_8 LAYER, uint_16 LENGTH) {
	OBJECT_PROPERTY *LIST = (OBJECT_PROPERTY *)LAYERS[LAYER].ADDRESS;
	int i = 0;

	if (LAYERS[LAYER].FLAGS & FLAG_2D_GRID)
		return;

	for (; i < LENGTH; i++) {
		if (LIST[i].FLAGS & OBJECT_FLAG_PRESENT)
			continue;

		LIST[i] = OBJECT;
		break;
	}
}

void SEND_OBJECT_UPDATE(uint_8 ID, char X_OFFSET, char Y_OFFSET) {
	int i = 0;

	for (; i < 0x20; i++) {
		if (UPDATE_LIST[i].ID)
			continue;

		UPDATE_LIST[i].ID = ID;
		UPDATE_LIST[i].X_OFFSET = X_OFFSET;
		UPDATE_LIST[i].Y_OFFSET = Y_OFFSET;
		break;
	}
}

void REGISTER_KEY_INPUT(uint_8 KEY, void *FUNCTION) {
	KEYBIND EVENT;
	EVENT.KEY = KEY;
	EVENT.FUNCTION = FUNCTION;
	
	if (KEYBINDS_INDEX >= 0x20)
		return;

	KEYBINDS[KEYBINDS_INDEX++] = EVENT;
}

void interrupt ISR_KEYBOARD(void) {
	uint_32 i = 0;
	uint_8 index;
	
	uint_8 ASCII[] = {
		0, 0x2B, '1', '2', '3', '4', '5', '6', '7', '8',
		'9', '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r', 't',
		'y', 'u', 'i', 'o', 'p', '[', ']', 0xA, 0,
		'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
		'\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
		'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	index = inp(0x60);

	outp(0x20, 0x20);

	if (index & 0x80) {
		for (; i < 0x10; i++) {
			if (CURRENT_KEYS[i] == ASCII[index & 0x7F])
				CURRENT_KEYS[i] = 0;
		}
	} else {
		for (; i < 0x10; i++)
			if (CURRENT_KEYS[i] == ASCII[index])
				return;

		for (i = 0; i < 0x10; i++) {
			if (CURRENT_KEYS[i] != 0)
				continue;

			CURRENT_KEYS[i] = ASCII[index];
			break;
		}
	}
}

void interrupt ISR_MOUSE(void) {
	union REGS inp, outp;
	int i = 0, X, Y;

	inp.x.ax = 0x03;
	int86(0x33, &inp, &outp);
	outp(0x20, 0x20);

	if (outp.x.bx & 1) {
		/*LMB*/

		X = X_MOUSE / 10;
		Y = Y_MOUSE / 10;

		/* Fix later but this is good for now */
		for (; i < 0x40; i++) {
			if (!(ENTITY_LIST[i].FLAGS & OBJECT_FLAG_PRESENT))
				continue;

			if (ENTITY_LIST[i].X <= X_MOUSE && X_MOUSE <= ENTITY_LIST[i].X + ENTITY_LIST[i].W &&
				ENTITY_LIST[i].Y <= Y_MOUSE && Y_MOUSE <= ENTITY_LIST[i].Y + ENTITY_LIST[i].H)
				goto Failed;
		}

		TILE_BLOCK_ARRAY[(Y * 32) + X] = COLOR_PICKED;
	}

	Failed:

	if (outp.x.bx & 2) {
		/*RMB*/
		TILE_BLOCK_ARRAY[((Y_MOUSE / 10) * 32) + (X_MOUSE / 10)] = BLOCK_TYPE_NULL;
	}

	if (outp.x.cx > 320) {
		inp.x.ax = 4;
		inp.x.cx = 319;
		inp.x.dx = outp.x.dx;
		int86(0x33, &inp, &outp);
	}

	X_MOUSE = outp.x.cx;
	Y_MOUSE = outp.x.dx;

	ISR_MOUSE_OLD();
}

uint_8 GET_CHARACTER() {
	union REGS inp, outp;

	inp.x.ax = 0x00;
	int86(0x16, &inp, &outp);
	return inp.h.ah;
}

void INIT_MOUSE() {
	union REGS inp, outp;

	inp.x.ax = 0x00;
	int86(0x33, &inp, &outp);

	if (outp.x.ax != 0xFFFF) {
		VGA_PRINT("ERROR, MOUSE DRIVER NOT FOUND\nPRESS ANY KEY TO EXIT\n$");
		GET_CHARACTER();
		LEAVE_FAIL();
	}
}

void KEYBOARD_THREAD() {
	void (* EVENT)(void);
	int i = 0, j;

	for (; i < KEYBINDS_INDEX; i++) {
		for (j = 0; j < 0x10; j++) {
			if (KEYBINDS[i].KEY == CURRENT_KEYS[j]) {
				EVENT = (void (*)(void))KEYBINDS[i].FUNCTION;
				RUN_NEXT_TICK((void *)EVENT);
			}
		}
	}
}

void interrupt ISR_TIMER(void) {
	void (* EVENT)(void);
	int i = 0;
	
	ISR_TIMER_OLD();
	outp(0x20, 0x20);

	for (; i < 0x40; i++) {
		if (!(THREAD_LIST[i].FLAGS & THREAD_FLAG_TIMED))
			continue;

		if (THREAD_LIST[i].FLAGS & THREAD_FLAG_TICK) {
			EVENT = (void (*)(void))THREAD_LIST[i].MAIN_FUNC;
			EVENT();
			REMOVE_THREAD_ENTRY(i);
			continue;
		}

		if (THREAD_LIST[i].FLAGS & THREAD_FLAG_WHILE && THREAD_LIST[i].FLAGS & THREAD_FLAG_AFTER) {
			EVENT = (void (*)(void))THREAD_LIST[i].MAIN_FUNC;
			EVENT();
			THREAD_LIST[i].TIMER -= 55;

			if (THREAD_LIST[i].TIMER <= 0) {
				EVENT = (void (*)(void))THREAD_LIST[i].SUB_FUNC;
				EVENT();
				REMOVE_THREAD_ENTRY(i);
			}
		} else if (THREAD_LIST[i].FLAGS & THREAD_FLAG_WHILE) {
			EVENT = (void (*)(void))THREAD_LIST[i].MAIN_FUNC;
			EVENT();
			THREAD_LIST[i].TIMER -= 55;

			if (THREAD_LIST[i].TIMER <= 0) {
				REMOVE_THREAD_ENTRY(i);
			}
		} else if (THREAD_LIST[i].FLAGS & THREAD_FLAG_AFTER) {
			THREAD_LIST[i].TIMER -= 55;

			if (THREAD_LIST[i].TIMER <= 0) {
				EVENT = (void (*)(void))THREAD_LIST[i].MAIN_FUNC;
				EVENT();
				REMOVE_THREAD_ENTRY(i);
			}
		}
	}
}

void ADD_THREAD_ENTRY(THREAD_PROPERTY ENTRY) {
	int i = 0;
	
	for (; i < 0x40; i++) {
		if (THREAD_LIST[i].FLAGS & THREAD_FLAG_ENABLED) {
			if (THREAD_LIST[i].MAIN_FUNC == ENTRY.MAIN_FUNC)
				break;

			continue;
		}

		THREAD_LIST[i] = ENTRY;
		THREAD_LIST[i].FLAGS |= THREAD_FLAG_ENABLED;
		break;
	}
}

void REMOVE_THREAD_ENTRY(int INDEX) {
	THREAD_LIST[INDEX].FLAGS = 0;
}

void ADD_THREAD(void *FUNCTION) {
	THREAD_PROPERTY Thread;

	Thread.MAIN_FUNC = FUNCTION;
	ADD_THREAD_ENTRY(Thread);
}

void ADD_LIMITED_THREAD(int ROUNDS, void *FUNCTION) {
	THREAD_PROPERTY Thread;

	Thread.FLAGS = THREAD_FLAG_LIMIT;
	Thread.TIMER = ROUNDS;
	Thread.MAIN_FUNC = FUNCTION;
	ADD_THREAD_ENTRY(Thread);
}

void ADD_TIMER_WHILE_THREAD(int TIME, void *FUNCTION) {
	THREAD_PROPERTY Thread;

	Thread.FLAGS = THREAD_FLAG_TIMED | THREAD_FLAG_WHILE;
	Thread.TIMER = TIME;
	Thread.MAIN_FUNC = FUNCTION;
	ADD_THREAD_ENTRY(Thread);
}

void ADD_TIMER_POST_THREAD(int TIME, void *FUNCTION) {
	THREAD_PROPERTY Thread;

	Thread.FLAGS = THREAD_FLAG_TIMED | THREAD_FLAG_AFTER;
	Thread.TIMER = TIME;
	Thread.MAIN_FUNC = FUNCTION;
	ADD_THREAD_ENTRY(Thread);
}

void ADD_TIMER_THREAD(int TIME, void *FUNCTION_MAIN, void *FUNCTION_POST) {
	THREAD_PROPERTY Thread;

	Thread.FLAGS = THREAD_FLAG_TIMED | THREAD_FLAG_WHILE | THREAD_FLAG_AFTER;
	Thread.TIMER = TIME;
	Thread.MAIN_FUNC = FUNCTION_MAIN;
	Thread.SUB_FUNC = FUNCTION_POST;
	ADD_THREAD_ENTRY(Thread);
}

void RUN_NEXT_TICK(void *FUNCTION) {
	THREAD_PROPERTY Thread;

	Thread.FLAGS = THREAD_FLAG_TIMED | THREAD_FLAG_TICK;
	Thread.MAIN_FUNC = FUNCTION;
	ADD_THREAD_ENTRY(Thread);
}

void WORLD_GENERATE_SIMPLE() {
	int i = 0;

	/* Remove when world gen is automated (with seeds) */
	for (i = 0; i < 32; i++) {
		TILE_BLOCK_ARRAY[(32 * 10) + i] = BLOCK_TYPE_GRASS;
	}

	for (i = 0; i < 288; i++) {
		TILE_BLOCK_ARRAY[(32 * 11) + i] = BLOCK_TYPE_DIRT;
	}
}

