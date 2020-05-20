
///////////////////////////////////////////////////////////////////////////////
// Headers.

#include <stdint.h>
#include "system.h"
#include <stdio.h>
#include <string.h>

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



///////////////////////////////////////////////////////////////////////////////
// Game data structures.

typedef struct {
	uint8_t digits[4]; // digits[0] is fastest.
} game_state_t;




uint32_t* red__p[16] = {
	red_0__p, red_1__p, red_2__p, red_3__p,
	red_4__p, red_5__p, red_6__p, red_7__p,
	red_8__p, red_9__p, red_a__p, red_b__p,
	red_c__p, red_d__p, red_e__p, red_f__p
};
uint32_t* green__p[16] = {
	green_0__p, green_1__p, green_2__p, green_3__p,
	green_4__p, green_5__p, green_6__p, green_7__p,
	green_8__p, green_9__p, green_a__p, green_b__p,
	green_c__p, green_d__p, green_e__p, green_f__p
};


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
	uint32_t* src_p,
	uint16_t src_w,
	uint16_t src_h,
	uint16_t dst_x,
	uint16_t dst_y
) {
	
	
	uint16_t dst_x8 = shift_div_with_round_down(dst_x, 3);
	uint16_t src_w8 = shift_div_with_round_up(src_w, 3);
	
	
	
	for(uint16_t y = 0; y < src_h; y++){
		for(uint16_t x8 = 0; x8 < src_w8; x8++){
			uint32_t src_idx = y*src_w8 + x8;
			uint32_t pixels = src_p[src_idx];
			uint32_t dst_idx =
				(dst_y+y)*SCREEN_IDX4_W8 +
				(dst_x8+x8);
			pack_idx4_p32[dst_idx] = pixels;
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
	memset(&gs, 0, sizeof(gs));
	
	
	while(1){
		
		
		/////////////////////////////////////
		// Poll controls.
		
		
		/////////////////////////////////////
		// Gameplay.
		
		// Stopwatch for 60Hz (no using floating point).
		
		// 100th of the seconds.
		switch(gs.digits[0]){
		case 0: // 0.000...
			gs.digits[0] = 2;
			break;
		case 2: // 0.016...
			gs.digits[0] = 3;
			break;
		case 3: // 0.033...
			gs.digits[0] = 5;
			break;
		case 5: // 0.050...
			gs.digits[0] = 7;
			break;
		case 7: // 0.066...
			gs.digits[0] = 8;
			break;
		case 8: // 0.083...
			gs.digits[0] = 0;
			
			// 10th of the seconds.
			gs.digits[1]++;
			if(gs.digits[1] == 10){
				gs.digits[1] = 0;
				
				// seconds.
				gs.digits[2]++;
				if(gs.digits[2] == 10){
					gs.digits[2] = 0;
					
					// 10s of seconds.
					gs.digits[3]++;
					if(gs.digits[3] == 10){
						gs.digits[3] = 0;
					}
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
		for(uint16_t r1 = 0; r1 < SCREEN_IDX4_H; r1++){
			for(uint16_t c8 = 0; c8 < SCREEN_IDX4_W8; c8++){
				pack_idx4_p32[r1*SCREEN_IDX4_W8 + c8] = 0x00000000;
			}
		}
		
		
		
		
		// Draw digits of stopwatch.
		draw_sprite(
			red__p  [gs.digits[3]], 32, 64, 32+(3-3)*40, 32
		);
		draw_sprite(
			red__p  [gs.digits[2]], 32, 64, 32+(3-2)*40, 32
		);
		draw_sprite(
			green__p[gs.digits[1]], 32, 64, 32+(3-1)*40, 32
		);
		draw_sprite(
			green__p[gs.digits[0]], 32, 64, 32+(3-0)*40, 32
		);
		
		
		
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
