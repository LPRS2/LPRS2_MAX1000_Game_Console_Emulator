
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>

#include "sprites_rgb333.h"

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

void draw_sprite_from_atlas(
	uint16_t src_x,
	uint16_t src_y,
	uint16_t w,
	uint16_t h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	for(uint16_t y = 0; y < h; y++){
		for(uint16_t x = 0; x < w; x++){
			uint16_t p = Pacman_Sprite_Map__p[
				(src_y+y)*Pacman_Sprite_Map__w + (src_x+x)
			];
			unpack_rgb333_p32[(dst_y+y)*SCREEN_RGB333_W + (dst_x+x)] = p;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 3; // RGB333 mode.
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.


	// Game state.
	game_state_t gs;
	gs.pacman.pos.x = 32;
	gs.pacman.pos.y = 32;
	gs.pacman.anim.step = PACMAN_IDLE;
	gs.pacman.anim.delay_cnt = 0;
	
	
	while(1){
		
		
		/////////////////////////////////////
		// Poll controls.
		
		int mov_x = 0;
		int mov_y = 0;
		if(joypad.right){
			mov_x = +1;
		}
		
		/////////////////////////////////////
		// Gameplay.
		
		
		gs.pacman.pos.x += mov_x*STEP;
		gs.pacman.pos.y += mov_y*STEP;
		
		
		switch(gs.pacman.anim.step){
		case PACMAN_IDLE:
			if(mov_x != 0 || mov_y != 0){
				gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
				gs.pacman.anim.step = PACMAN_WITH_OPEN_MOUTH;
			}
			break;
		case PACMAN_OPENING_MOUTH:
			if(gs.pacman.anim.delay_cnt != 0){
					gs.pacman.anim.delay_cnt--;
			}else{
				gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
				gs.pacman.anim.step = PACMAN_WITH_OPEN_MOUTH;
			}
			break;
		case PACMAN_WITH_OPEN_MOUTH:
			if(gs.pacman.anim.delay_cnt != 0){
					gs.pacman.anim.delay_cnt--;
			}else{
				if(mov_x != 0 || mov_y != 0){
					gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
					gs.pacman.anim.step = PACMAN_CLOSING_MOUTH;
				}else{
					gs.pacman.anim.step = PACMAN_IDLE;
				}
			}
			break;
		case PACMAN_CLOSING_MOUTH:
			if(gs.pacman.anim.delay_cnt != 0){
					gs.pacman.anim.delay_cnt--;
			}else{
				gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
				gs.pacman.anim.step = PACMAN_WITH_CLOSED_MOUTH;
			}
			break;
		case PACMAN_WITH_CLOSED_MOUTH:
			if(gs.pacman.anim.delay_cnt != 0){
					gs.pacman.anim.delay_cnt--;
			}else{
				if(mov_x != 0 || mov_y != 0){
					gs.pacman.anim.delay_cnt = PACMAN_ANIM_DELAY;
					gs.pacman.anim.step = PACMAN_OPENING_MOUTH;
				}else{
					gs.pacman.anim.step = PACMAN_IDLE;
				}
			}
			break;
		}
		
		
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		
		
		// Black background.
		for(uint16_t r = 0; r < SCREEN_RGB333_H; r++){
			for(uint16_t c = 0; c < SCREEN_RGB333_W; c++){
				unpack_rgb333_p32[r*SCREEN_RGB333_W + c] = 0000;
			}
		}
		
		
		
		// Draw pacman.
		switch(gs.pacman.anim.step){
		case PACMAN_IDLE:
		case PACMAN_OPENING_MOUTH:
		case PACMAN_CLOSING_MOUTH:
			// Half open mouth.
			draw_sprite_from_atlas(16, 0, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y);
			break;
		case PACMAN_WITH_OPEN_MOUTH:
			// Full open mouth.
			draw_sprite_from_atlas(0, 0, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y);
			break;
		case PACMAN_WITH_CLOSED_MOUTH:
			// Close mouth.
			draw_sprite_from_atlas(32, 0, 16, 16, gs.pacman.pos.x, gs.pacman.pos.y);
			break;
		}
		
		
		
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
