
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>

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


#define IDX4_0_RGB333_1 1



// Suffix 8 means that it is in units of 8 pixels.
// For example if RECT_H8 is 2,
// that means that height of rectangle is 2*8 = 16 pixels.
#define STEP8 1
#define RECT_H8 2
#define RECT_W8 4
#define SQ_A8 8




///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint16_t x;
	uint16_t y;
} point_t;

typedef enum {RECT, SQ} player_t;

typedef struct {
	// Upper left corners.
	point_t rect8;
	point_t sq8;
	
	player_t active;
} game_state_t;



///////////////////////////////////////////////////////////////////////////////
// Game code.

int main(void) {
	
	// Setup.
	#if !IDX4_0_RGB333_1
		gpu_p32[0] = 2; // IDX4 mode.
		// Use packed mode in IDX4.
		gpu_p32[1] = 1;
	#else
		gpu_p32[0] = 3; // RGB333 mode.
		// In RGB333 unpacked mode is only one that exist anyway.
		gpu_p32[1] = 0;
	#endif
	gpu_p32[0x800] = 0x00ff00ff; // Magenta for HUD.



	// Game state.
	game_state_t gs;
	gs.rect8.x = 0;
	gs.rect8.y = 0;
	
	gs.sq8.x = 2;
	gs.sq8.y = 2;
	
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
			gs.rect8.x += mov_x*STEP8;
			gs.rect8.y += mov_y*STEP8;
			if(toggle_active){
				gs.active = SQ;
			}
			break;
		case SQ:
			gs.sq8.x += mov_x*STEP8;
			gs.sq8.y += mov_y*STEP8;
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
		
		
		
		
		
#if IDX4_0_RGB333_1

		// Unpacked RGB333 mode.



		// Blue background.
		for(
			uint16_t r = 0;
			r < SCREEN_RGB333_H;
			r++
		){
			for(
				uint16_t c = 0;
				c < SCREEN_RGB333_W;
				c++
			){
				uint32_t idx = r*SCREEN_RGB333_W + c;
				unpack_rgb333_p32[idx] = 0700; // Octal format.
			}
		}




		// Red rectangle.
		for(
			uint16_t r1 = gs.rect8.y*8;
			r1 < (gs.rect8.y+RECT_H8)*8;
			r1++
		){
			for(
				uint16_t c1 = gs.rect8.x*8;
				c1 < (gs.rect8.x+RECT_W8)*8;
				c1++
			){
				uint32_t idx = r1*SCREEN_RGB333_W + c1;
				unpack_rgb333_p32[idx] = 0007;
			}
		}



		// Green square.
		for(
			uint16_t r1 = gs.sq8.y*8;
			r1 < (gs.sq8.y+SQ_A8)*8;
			r1++
		){
			for(
				uint16_t c1 = gs.sq8.x*8;
				c1 < (gs.sq8.x+SQ_A8)*8;
				c1++
			){
				uint32_t idx = r1*SCREEN_RGB333_W + c1;
				unpack_rgb333_p32[idx] = 0070;
			}
		}





#else


		// Packed IDX4 mode.
		
		palette_p32[0] = 0x00ff0000; // Blue.
		palette_p32[1] = 0x000000ff; // Red.
		palette_p32[2] = 0x0000ff00; // Green.
		
		
		
		
		// Blue background.
		for(
			uint16_t r1 = 0;
			r1 < SCREEN_IDX4_H;
			r1++
		){
			for(
				uint16_t c8 = 0;
				c8 < SCREEN_IDX4_W8;
				c8++
			){
				uint32_t idx = r1*SCREEN_IDX4_W8 + c8;
				pack_idx4_p32[idx] = 0x00000000;
			}
		}
		
		
		
		
		
		// Red rectangle.
		for(
			uint16_t r1 = gs.rect8.y*8;
			r1 < (gs.rect8.y+RECT_H8)*8;
			r1++
		){
			for(
				uint16_t c8 = gs.rect8.x;
				c8 < gs.rect8.x+RECT_W8;
				c8++
			){
				uint32_t idx = r1*SCREEN_IDX4_W8 + c8;
				pack_idx4_p32[idx] = 0x11111111;
			}
		}
		
		
		
		// Green square.
		for(
			uint16_t r1 = gs.sq8.y*8;
			r1 < (gs.sq8.y+SQ_A8)*8;
			r1++
		){
			for(
				uint16_t c8 = gs.sq8.x;
				c8 < gs.sq8.x+SQ_A8;
				c8++
		){
				uint32_t idx = r1*SCREEN_IDX4_W8 + c8;
				pack_idx4_p32[idx] = 0x22222222;
			}
		}
		
		
		
		// For testing.
#if 0
		gpu_p32[1] = 0;
		// Unpacked.
		for(uint16_t r = 0; r < SCREEN_IDX4_H; r++){
			for(uint16_t c = 0; c < SCREEN_IDX4_W; c++){
				uint32_t idx = r*SCREEN_IDX4_W + c;
				unpack_idx4_p32[idx] = 0;
			}
		}
#endif

#endif
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
