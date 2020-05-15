
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// HW stuff.

#define WAIT_UNITL_0(x) while(x != 0){}
#define WAIT_UNITL_1(x) while(x != 1){}

#define SCREEN_W 640
#define SCREEN_H 480

#define gpu_p32 ((volatile uint32_t*)LPRS2_GPU_BASE)
#define palette_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x1000))
#define unpack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x400000))
#define pack_idx1_p32 ((volatile uint32_t*)(LPRS2_GPU_BASE+0x600000))
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
	uint32_t m[SCREEN_H][SCREEN_W];
} bf_unpack_idx1;
#define unpack_idx1 (*((volatile bf_unpack_idx1*)unpack_idx1_p32))



///////////////////////////////////////////////////////////////////////////////
// Game config.

#define STEP 32
#define RECT_H 64
#define RECT_W 128
#define SQ_A 256

#define UNPACKED_0_PACKED_1 0



///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef enum {RECT, SQ} player_t;

typedef struct {
	// Upper left corners.
	point_t rect;
	point_t sq;
	
	player_t active;
} game_state_t;



///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	gpu_p32[0] = 1; // 1b index mode.
	gpu_p32[1] = UNPACKED_0_PACKED_1;
	palette_p32[0] = 0x00ff0000; // Blue for background.
	palette_p32[1] = 0x000000ff; // Red for players.
	gpu_p32[0x800] = 0x0000ff00; // Green for HUD.


	// Game state.
	game_state_t gs;
	gs.rect.x = 0;
	gs.rect.y = 0;
	
	gs.sq.x = 128;
	gs.sq.y = 128;
	
	gs.active = RECT;

	
	
	while(1){
		
		
		/////////////////////////////////////
		// Poll controls.
		int mov_x = 0;
		int mov_y = 0;
		if(joypad.down){
			mov_y = +1;
		}
		if(joypad.up){
			mov_y = -1;
		}
		if(joypad.right){
			mov_x = +1;
		}
		if(joypad.left){
			mov_x = -1;
		}
		//TODO Have bug here. Hold right button and play with A button.
		int toggle_active = joypad.a;
		
		
		
		
		
		
		/////////////////////////////////////
		// Gameplay.
		
		switch(gs.active){
		case RECT:
			//TODO Limit not to go through wall. Same for all players.
			gs.rect.x += mov_x*STEP;
			gs.rect.y += mov_y*STEP;
			if(toggle_active){
				gs.active = SQ;
			}
			break;
		case SQ:
			gs.sq.x += mov_x*STEP;
			gs.sq.y += mov_y*STEP;
			if(toggle_active){
				gs.active = RECT;
			}
			break;
		}
		
		
		
		/////////////////////////////////////
		// Drawing.
		
		
		// Detecting rising edge of VSync.
		WAIT_UNITL_0(gpu_p32[2]);
		WAIT_UNITL_1(gpu_p32[2]);
		// Draw in buffer while it is in VSync.
		
		
		
#if !UNPACKED_0_PACKED_1
		// Unpacked.
		
		// Clear to blue.
		for(int r = 0; r < SCREEN_H; r++){
			for(int c = 0; c < SCREEN_W; c++){
				unpack_idx1_p32[r*SCREEN_W + c] = 0;
			}
		}
		
		
		
		// Red rectangle.
		// Use array with 2D indexing.
		for(int r = gs.rect.y; r < gs.rect.y+RECT_H; r++){
			for(int c = gs.rect.x; c < gs.rect.x+RECT_W; c++){
				unpack_idx1_p32[r*SCREEN_W + c] = 1;
			}
		}
		
		
		
		// Red sqaure.
		// Use struct with 2D matrix.
		for(int r = gs.sq.y; r < gs.sq.y+SQ_A; r++){
			for(int c = gs.sq.x; c < gs.sq.x+SQ_A; c++){
				unpack_idx1.m[r][c] = 1;
			}
		}
		
		
		
#else
		// Packed.


		//TODO This is just test. Implement same as for unpacked.
		for(int r = 0; r < 32; r++){
			for(int c = 0; c < 1; c++){
				pack_idx1_p32[r*(SCREEN_W/32) + c] = 0xffffffff;
			}
		}
		
		
#endif
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
