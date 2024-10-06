#include <DOS.H>
#include <STDIO.H>

/* CONSTANTS */
#define VIDEO 				0xA0000000
#define TEMP  				0xB0000000
#define SCREEN				0xFA00 / 0x02
#define NULL				0

#define FLAG_2D_GRID 		1<<0
#define FLAG_COLLIDE_BASE	1<<1
#define FLAG_COLLIDE_REACT	1<<2
#define FLAG_UI				1<<3

#define BLOCK_TYPE_NULL		0x00
#define BLOCK_TYPE_DIRT		0x01
#define BLOCK_TYPE_GRASS	0x02

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
#define MILLISECONDS_TO_TICKS(X) x / 55

/* TYPES */
typedef unsigned char 	uint_8;
typedef unsigned short 	uint_16;
typedef unsigned int   	uint_32;

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
void VGA_INIT();
void VGA_EXIT();
void VGA_SWAP();
void VGA_PLACE(uint_8, uint_16, uint_16);

/* DRAWING */
void DRAW_CUBE(uint_8, uint_8, uint_8, uint_16, uint_16);
void DRAW_OBJECT(OBJECT_PROPERTY);
void DRAW_GRID(char *);

/* LAYERS/OBJECTS */
void INIT_LAYERS();
void LAYER_UPDATE();
void CREATE_OBJECT(OBJECT_PROPERTY, uint_8, uint_16);
void SEND_OBJECT_UPDATE(uint_8, char, char);

/* INPUT */
void interrupt ISR_KEYBOARD(void);
void interrupt ISR_MOUSE(void);
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

/* ARRAYS */
LAYER_PROPERTIES LAYERS[4];

char TILE_WALL_ARRAY[640];
char TILE_BLOCK_ARRAY[640];
OBJECT_PROPERTY ENTITY_LIST[0x40];
OBJECT_PROPERTY UI_LIST[0x20];
OBJECT_UPDATE UPDATE_LIST[0x20];

KEYBIND KEYBINDS[0x10];
char CURRENT_KEYS[0x10];
uint_32 KEYBINDS_INDEX = 0;
uint_32 CURRENT_KEYS_INDEX = 0;

THREAD_PROPERTY THREAD_LIST[0x40];


void interrupt (*ISR_KEYBOARD_OLD)(void);
void interrupt (*ISR_MOUSE_OLD)(void);
void interrupt (*ISR_TIMER_OLD)(void);
char EXIT = 0;

/* ALL FUNCTIONS */
void Test_Left() {
	SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, -1, 0);
}

void Test_Right() {
	SEND_OBJECT_UPDATE(OBJECT_ID_PLAYER, 1, 0);
}

void Exit_Loop() {
	EXIT = 1;
}

int main() {
	OBJECT_PROPERTY Plr = {OBJECT_ID_PLAYER, OBJECT_FLAG_PRESENT, COLOR_PICKER(1, 1, 3), 20, 30, 40, 40};
	void (* THREAD)(void);
	union REGS inp, outp;
	int i = 0;

	inp.x.ax = 0x00;
	int86(0x33, &inp, &outp);

	ISR_KEYBOARD_OLD = getvect(0x09);
	ISR_MOUSE_OLD = getvect(0x74);
	ISR_TIMER_OLD = getvect(0x1C);
    setvect(0x09, ISR_KEYBOARD);
    setvect(0x74, ISR_MOUSE);
    setvect(0x1C, ISR_TIMER);

	REGISTER_KEY_INPUT('a', (void *)&Test_Left);
	REGISTER_KEY_INPUT('d', (void *)&Test_Right);
	REGISTER_KEY_INPUT(0x2B, (void *)&Exit_Loop);

	VGA_INIT();
	INIT_LAYERS();	

	/* Remove when world gen is automated (with seeds) */
	for (i = 0; i < 32; i++) {
		TILE_BLOCK_ARRAY[(32 * 10) + i] = BLOCK_TYPE_GRASS;
	}

	for (i = 0; i < 288; i++) {
		TILE_BLOCK_ARRAY[(32 * 11) + i] = BLOCK_TYPE_DIRT;
	}

	/* Load Objects */
	CREATE_OBJECT(Plr, 0x02, 0x40);

	ADD_THREAD((void *)&LAYER_UPDATE);
	ADD_THREAD((void *)&VGA_SWAP);
	ADD_THREAD((void *)&KEYBOARD_THREAD);

	while (!EXIT) {
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
	VGA_EXIT();
	return 0;
}

void VGA_INIT() {
	union REGS inp, outp;

	inp.h.ah = 0x00;
	inp.h.al = 0x13;
	int86(0x10, &inp, &outp);
}

void VGA_EXIT() {
	union REGS inp, outp;

	/* Find a way to load the previous screen mode without assuming 3 */
	inp.h.ah = 0x00;
	inp.h.al = 0x03;
	int86(0x10, &inp, &outp);

	/* Clear console */
}

void VGA_SWAP() {
	int far *Video_Buffer = (int far *)VIDEO;
	int far *Temp_Buffer = (int far *)TEMP;
	int i = 0;

	for (; i < SCREEN; i++)
		Video_Buffer[i] = Temp_Buffer[i];

	for (i = 0; i < SCREEN; i++)
		Temp_Buffer[i] = 0x03030303;
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
			default:
				DRAW_CUBE(0x25, 0xA, 0xA, x * 10, y * 10);
				break;
		}
	}
}

void INIT_LAYERS() {
	LAYERS[0].ADDRESS = (uint_16)TILE_WALL_ARRAY;
	LAYERS[0].SIZE = sizeof(TILE_WALL_ARRAY);
	LAYERS[0].FLAGS = FLAG_2D_GRID | FLAG_COLLIDE_BASE;

	LAYERS[1].ADDRESS = (uint_16)TILE_BLOCK_ARRAY;
	LAYERS[1].SIZE = sizeof(TILE_BLOCK_ARRAY);
	LAYERS[1].FLAGS = FLAG_2D_GRID | FLAG_COLLIDE_BASE;

	LAYERS[2].ADDRESS = (uint_16)ENTITY_LIST;
	LAYERS[2].SIZE = sizeof(ENTITY_LIST);
	LAYERS[2].FLAGS = FLAG_COLLIDE_REACT;

	LAYERS[3].ADDRESS = (uint_16)UI_LIST;
	LAYERS[3].SIZE = sizeof(UI_LIST);
	LAYERS[3].FLAGS = FLAG_UI;
}

void LAYER_UPDATE() {
	OBJECT_PROPERTY *OBJECT;
	int i = 0, j = 0, k = 0;

	for (; i < 4; i++) {
		if (LAYERS[i].FLAGS & FLAG_2D_GRID) {
			DRAW_GRID((char *)LAYERS[i].ADDRESS);
		} else {
			OBJECT = (OBJECT_PROPERTY *)LAYERS[i].ADDRESS;

			for (j = 0; j < 0x20; j++) {
				if (OBJECT[j].FLAGS & OBJECT_FLAG_PRESENT)
					if (!OBJECT[j].ID)
						continue;
					
					for (k = 0; k < 0x20; k++) {
						if (OBJECT[j].ID == UPDATE_LIST[k].ID) {
							UPDATE_LIST[k].ID = 0;
							OBJECT[j].X += UPDATE_LIST[k].X_OFFSET;
							OBJECT[j].W += UPDATE_LIST[k].X_OFFSET;
							OBJECT[j].Y += UPDATE_LIST[k].Y_OFFSET;
							OBJECT[j].H += UPDATE_LIST[k].Y_OFFSET;
						}
					}

					DRAW_OBJECT(OBJECT[j]);
			}
		}
	}
}

void LAYER_CHECK_COLLISION() {

}

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

	inp.x.ax = 0x03;
	int86(0x33, &inp, &outp);
	outp(0x20, 0x20);

	if (outp.x.bx & 1) {
		/*LMB*/
	}

	if (outp.x.bx & 2) {
		/*RMB*/
	}

	if (outp.x.cx > 320) {
		inp.x.ax = 4;
		inp.x.cx = 319;
		inp.x.dx = outp.x.dx;
		int86(0x33, &inp, &outp);
	}

	ISR_MOUSE_OLD();
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



