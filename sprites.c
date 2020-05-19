
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>

#include "sprites_idx4.h"

///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_IDX1_W 640
#define SCREEN_IDX1_H 480
#define SCREEN_IDX4_W 320
#define SCREEN_IDX4_H 240
#define SCREEN_RGB333_W 160
#define SCREEN_RGB333_H 120

#define SCREEN_IDX4_W8 (SCREEN_IDX4_W/8)

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
#define unpack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x800000))
#define pack_idx4_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xa00000))
#define unpack_rgb333_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0xc00000))
#define joypad_p32 ((volatile uint32_t*)LPRS2_JOYPAD_BASE)

typedef struct {
	unsigned a      : 1;
	unsigned b      : 1;
	unsigned z      : 1;
	unsigned start  : 1;
	unsigned up     : 1;
	unsigned down   : 1;
	unsigned left   : 1;
	unsigned right  : 1;
} bf_joypad;
#define joypad (*((volatile bf_joypad*)LPRS2_JOYPAD_BASE))

typedef struct {
	uint32_t m[SCREEN_IDX1_H][SCREEN_IDX1_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))



///////////////////////////////////////////////////////////////////////////////
// Game config.

#define STEP 1

#define PACMAN_ANIM_DELAY 3

///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef enum {
	PACMAN_IDLE,
	PACMAN_OPENING_MOUTH,
	PACMAN_WITH_OPEN_MOUTH,
	PACMAN_CLOSING_MOUTH,
	PACMAN_WITH_CLOSED_MOUTH
} pacman_anim_steps_t;

typedef struct {
	pacman_anim_steps_t step;
	uint8_t delay_cnt;
} pacman_anim_t;

typedef struct {
	point_t pos;
	pacman_anim_t anim;
} pacman_t;

typedef struct {
	pacman_t pacman;
} game_state_t;

static inline uint32_t shift_div_with_round_down(uint32_t num, uint32_t shift){
	uint32_t d = num >> shift;
	return d;
}

static inline uint32_t shift_div_with_round_up(uint32_t num, uint32_t shift){
	uint32_t d = num >> shift;
	uint32_t mask = (1<<shift)-1;
	if((num & mask) != 0){
		d++;
	}
	return d;
}

static void draw_sprite(
	uint32_t* p,
	uint16_t src_w,
	uint16_t src_h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	uint16_t dst_x8 = shift_div_with_round_down(dst_x, 3);
	uint16_t src_w8 = shift_div_with_round_up(src_w, 3);
	
	for(uint16_t y = 0; y < src_h; y++){
		for(uint16_t x8 = 0; x8 < src_w8; x8++){
			uint32_t i = p[y*src_w8 + x8];
			pack_idx4_p32[(dst_y+y)*SCREEN_IDX4_W8 + (dst_x8+x8)] = i;
		}
	}
	
}

///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 2; // IDX4 mode.
	gpu_p32[1] = 1; // Packed mode.
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.
	
	
	// Copy palette.
	for(uint8_t i = 0; i < 16; i++){
		palette_p32[i] = palette[i];
	}
	
	
	
	
	// Game state.
	game_state_t gs;
	
	
	while(1){
		
		
		/////////////////////////////////////
		// Poll controls.
		
		
		/////////////////////////////////////
		// Gameplay.
		
		
		
		
		
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		
		// Black background.
		for(uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++){
			for(uint16_t c8 = 0; c8 < SCREEN_IDX4_W8; c8++){
				pack_idx4_p32[r1*SCREEN_IDX4_W8 + c8] = 0x00000000;
			}
		}


		draw_sprite(green_0__p, 32, 64, 16, 16);
		
		
		break;
	}
	while(1){}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
