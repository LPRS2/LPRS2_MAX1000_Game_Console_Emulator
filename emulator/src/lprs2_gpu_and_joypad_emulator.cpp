
///////////////////////////////////////////////////////////////////////////////

#include "system.h"
#include "config.h"

#include <SFML/Graphics.hpp>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <stdexcept>
#include <cstring>
#include <cmath>

#include <cstddef>

#include <iostream>
#include <cstdio>

using namespace std;
using namespace sf;

///////////////////////////////////////////////////////////////////////////////
// Helpers.

#define DEBUG(var) do{cout << #var << " = " << var << endl; }while(false)

class semaphore {
public:
	semaphore (int c = 0)
		: count(c) {
	}

	void notify() {
		unique_lock<mutex> lock(m);
		count++;
		cv.notify_one();
	}

	void wait() {
		unique_lock<mutex> lock(m);
		
		while(count == 0){ // Handle spurious wake-ups.
			cv.wait(lock);
		}
		
		count--;
	}

private:
	mutex m;
	condition_variable cv;
	int count;
};


///////////////////////////////////////////////////////////////////////////////
// Memory mappings.

volatile void* __lprs_gpu_base;
volatile void* __lprs_joypad_base;

/*
	Device: 10M16SAU169C8G
	549 Kb = 61x M9K
	M9K: 1024x9b
	Simple Dual-Port mode.
	All adresses relative:
	- common:
		0x00000000 - mode
		0x00000004 - unpacked_0_packed_1
		0x00000008 - vsync
		[0x00001000, 0x00001040) - palette
		0x00002000 - hud_color
	- 1b index mode:
		640x480 = 9600x32b of 15616x32b
		[0x00400000, 0x0052c000) - unpacked
		[0x00600000, 0x00609600) - packed
	- 4b index mode:
		320x240 (640x480 half) = 9600x32b of 15616x32b
		[0x00800000, ) - unpacked
		[0x00a00000, ) - packed
	- RGB333 i.e. 9b color mode:
		160x120 (640x480 quater) = 19200x9b of 62464x9b
		[0x00c00000, ) - unpacked
*/

struct lprs2_gpu_mem_map {
	// common
	unsigned mode                : 2;
	unsigned                     : 30;
	unsigned unpacked_0_packed_1 : 1;
	unsigned                     : 31;
	unsigned vsync               : 1;
	unsigned                     : 31;
	uint8_t __res0[0x00001000-0x0000000c];
	uint32_t palette[16];
	uint8_t __res1[0x00002000-0x00001040];
	uint32_t hud_color; // Only on emulator.
	// 1b index mode
	uint8_t __res2[0x00400000-0x00002004];
	uint32_t unpack_idx1[307200];
	uint8_t __res3[0x00600000-0x0052c000];
	uint32_t pack_idx1[9600];
	// 4b index mode
	uint8_t __res4[0x00800000-0x00609600];
	uint32_t unpack_idx4[76800];
	uint8_t __res5[0x00a00000-0x0084b000];
	uint32_t pack_idx4[9600];
	// RGB333 mode
	uint8_t __res6[0x00c00000-0x00a09600];
	uint32_t unpack_rgb333[19200];
	uint8_t __res7[0x01000000-0x00c12c00];
};


union lprs2_joypad_mem_map {
	uint32_t r;
	struct {
		unsigned a        : 1;
		unsigned b        : 1;
		unsigned z        : 1;
		unsigned start    : 1;
		unsigned up0      : 1;
		unsigned down0    : 1;
		unsigned left0    : 1;
		unsigned right0   : 1;
		unsigned          : 2;
		unsigned l        : 1;
		unsigned r        : 1;
		unsigned up1      : 1;
		unsigned down1    : 1;
		unsigned left1    : 1;
		unsigned right1   : 1;
		unsigned analog_x : 8;
		unsigned analog_y : 8;
	} s;
};


///////////////////////////////////////////////////////////////////////////////
// Emulator.

class LPRS2_GPU_and_Joypad_Emulator {
public:
	volatile lprs2_gpu_mem_map gpu_mem_map;
	volatile lprs2_joypad_mem_map joypad_mem_map;
	RenderWindow* window;
	mutex window_mutex;
	thread* main_thread;
	uint32_t rgba8888[SCREEN_H][SCREEN_W];
#if TWO_THREADS
	thread* draw_thread;
	semaphore rgba8888_drawn; // Wait by main, notify by draw.
	semaphore rgba8888_filled; // Wait by draw, notify by main.
#endif
	Texture* texture;
	Sprite sprite;
	uint64_t frame;
	Clock clock;
#if SHOW_HEAD_UP_DISPLAY
	Font font;
	Text text;
	uint32_t fps;
	uint32_t Hz;
	char buf[30];
#endif
	
	void close_window() {
		{
			unique_lock<mutex> lock(window_mutex);
			window->close();
		}
		exit(0);
	}
	bool is_window_open() {
		unique_lock<mutex> lock(window_mutex);
		return window->isOpen();
	}
	
	void update_joypad() {
		Event event;
		{
			unique_lock<mutex> lock(window_mutex);
			if(!window->pollEvent(event)){
				return;
			}
		}
		
		if(event.type == Event::Closed){
			close_window();
			return;
		}
		if(
			event.type == Event::KeyPressed &&
			event.key.code == Keyboard::Escape
		){
			close_window();
			return;
		}
		if(
			event.type == Event::KeyPressed ||
			event.type == Event::KeyReleased
		){
			int s = event.type == Event::KeyPressed;
			
			switch(event.key.code){
			case Keyboard::Left:
				joypad_mem_map.s.left0 = s;
				joypad_mem_map.s.left1 = s;
				break;
			case Keyboard::Right:
				joypad_mem_map.s.right0 = s;
				joypad_mem_map.s.right1 = s;
				break;
			case Keyboard::Up:
				joypad_mem_map.s.up0 = s;
				joypad_mem_map.s.up1 = s;
				break;
			case Keyboard::Down:
				joypad_mem_map.s.down0 = s;
				joypad_mem_map.s.down1 = s;
				break;
			case Keyboard::A:
				joypad_mem_map.s.a = s;
				break;
			case Keyboard::B:
				joypad_mem_map.s.b = 1;
				break;
			case Keyboard::Z:
				joypad_mem_map.s.z = 1;
				break;
			case Keyboard::S:
				joypad_mem_map.s.start = 1;
				break;
			case Keyboard::L:
				joypad_mem_map.s.l = 1;
				break;
			case Keyboard::R:
				joypad_mem_map.s.r = 1;
				break;
			default:
				break;
			}
		}
	}
	
	void print_clock(const char* msg) {
		if(frame < 0){
			cout
				<< msg
				<< ": "
				<< clock.getElapsedTime().asMicroseconds()
				<< " us"
				<< endl;
		}
	}
	
	void wait_until_us(int us) {
		while(1){
			update_joypad();
			
			//print_clock("wait");
			if(clock.getElapsedTime() > microseconds(us)){
				break;
			}
		}
	}
	
	void draw_setup() {
		// Setup for drawing.
		{
			unique_lock<mutex> lock(window_mutex);
			window->setActive(true);
		}
		
		texture = new Texture;
		if(!texture->create(SCREEN_W, SCREEN_H)){
			throw runtime_error("Cannot create texture!");
		}
		sprite.setTexture(*texture, true);
#if SHOW_HEAD_UP_DISPLAY
		if(!font.loadFromFile(FONT_PATH)){
			throw runtime_error("Cannot load font!");
		}
		text.setFont(font);
		text.setCharacterSize(24); // [Pixels]
		text.setStyle(Text::Bold);
#endif
	}
	
	void draw_draw() {
		texture->update((const Uint8*)rgba8888);
#if SHOW_HEAD_UP_DISPLAY
		uint32_t c = gpu_mem_map.hud_color;
		Color text_color(c & 0xff, c>>8 & 0xff, c>>16 & 0xff);
#if (SFML_VERSION_MAJOR<<16 | SFML_VERSION_MINOR) >= 0x00020005
		text.setFillColor(text_color);
		text.setOutlineColor(Color::Transparent);
#else
		text.setColor(text_color);
#endif
		snprintf(buf, 30, "FPS: %4d   Hz: %4d", fps, Hz);
		text.setString(buf);
#endif
		{
			unique_lock<mutex> lock(window_mutex);
			window->draw(sprite);
#if SHOW_HEAD_UP_DISPLAY
			window->draw(text);
#endif
			window->display();
		}
	}
	
#if TWO_THREADS
	void draw() {
		// Draw (OpenGL) thread.
		
		draw_setup();
		
		// Initially it is drawn, not to make deadlock.
		rgba8888_drawn.notify();
		
		
		while(is_window_open()){
			
			// Wait for main thread to fill up the buffer.
			rgba8888_filled.wait();
			
			draw_draw();
			
			// Notify main thread that drawing is done.
			rgba8888_drawn.notify();
		}
	}
#endif
	
	
	void main() {
		window = new RenderWindow(
			VideoMode(SCREEN_W, SCREEN_H),
			"LPRS2 GPU Emulator"
		);
		
#if TWO_THREADS
		// Window thread.
		// Controls must be polled in this thread.
		window->setActive(false);
		// Create drawing thread.
		draw_thread = new thread([&]{ draw(); });
#else
		draw_setup();
#endif
		
		frame = 0;
		
		
		while(is_window_open()){
			
			// VSync timing.
			clock.restart();
			// Sync area.
			gpu_mem_map.vsync = 1;
			print_clock("vsync start");
			wait_until_us(VSYNC_US);
			print_clock("vsync end");
			// Visible area.
			gpu_mem_map.vsync = 0;
			
			//cout << "frame " << frame << endl;
			
			// Need more time for drawing.
			wait_until_us(EXTENDED_VSYNC_PAUSE_US);
			
#if TWO_THREADS
			print_clock("wait drawn");
			// Wait if draw thread is still drawing.
			// Should rarely wait here, if draw thread is fast enough.
			rgba8888_drawn.wait();
#endif
			
			print_clock("could copy");
			
			// Copy buffers.
			switch (gpu_mem_map.mode) {
			case 0: // Color bar mode.
				for(int color = 0; color < 8; color++){
					uint32_t R = (color>>0 & 0x1)*0xff;
					uint32_t G = (color>>1 & 0x1)*0xff;
					uint32_t B = (color>>2 & 0x1)*0xff;
					uint32_t RGBA8888 = 0xff000000 | B<<16 | G<<8 | R;
					int cc = color*SCREEN_W/8;
					for(int r = 0; r < SCREEN_H; r++){
						for(int c = 0; c < SCREEN_W/8; c++){
							rgba8888[r][cc+c] = RGBA8888;
						}
					}
				}
				break;
			case 1: // IDX1 mode.
				if(gpu_mem_map.unpacked_0_packed_1 == 0){
					// Unpacked.
					for(int r = 0; r < SCREEN_H; r++){
						for(int c = 0; c < SCREEN_W; c++){
							int idx = r*SCREEN_W + c;
							int i = gpu_mem_map.unpack_idx1[idx];
							int m = i & 1;
							uint32_t RGB = gpu_mem_map.palette[m];
							uint32_t RGBA = 0xff000000 | RGB;
							rgba8888[r][c] = RGBA;
						}
					}
				}else{
					// Packed.
					for(int r = 0; r < SCREEN_H; r++){
						for(int cw = 0; cw < SCREEN_W/32; cw++){
							int idx = r*(SCREEN_W/32) + cw;
							uint32_t w = gpu_mem_map.pack_idx1[idx];
							for(int sc = 0; sc < 32; sc++){
								int m = w & 1;
								uint32_t RGB = gpu_mem_map.palette[m];
								uint32_t RGBA = 0xff000000 | RGB;
								int c = cw*32 + sc;
								rgba8888[r][c] = RGBA;
								w >>= 1;
							}
						}
					}
				}
				break;
			case 2: // IDX4 mode
				if(gpu_mem_map.unpacked_0_packed_1 == 0){
					// Unpacked.
					for(int r2 = 0; r2 < SCREEN_H/2; r2++){
						for(int c2 = 0; c2 < SCREEN_W/2; c2++){
							int idx = r2*(SCREEN_W/2) + c2;
							int i = gpu_mem_map.unpack_idx4[idx];
							int m = i & 0xf;
							uint32_t RGB = gpu_mem_map.palette[m];
							uint32_t RGBA = 0xff000000 | RGB;
							for(int r1 = r2*2; r1 < (r2+1)*2; r1++){
								for(int c1 = c2*2; c1 < (c2+1)*2; c1++){
									rgba8888[r1][c1] = RGBA;
								}
							}
						}
					}
				}else{
					// Packed.
					for(int r2 = 0; r2 < SCREEN_H/2; r2++){
						for(int cw2 = 0; cw2 < SCREEN_W/2/8; cw2++){
							int idx = r2*(SCREEN_W/2/8) + cw2;
							uint32_t w = gpu_mem_map.pack_idx4[idx];
							for(int sc2 = 0; sc2 < 8; sc2++){
								int m = w & 0xf;
								uint32_t RGB = gpu_mem_map.palette[m];
								uint32_t RGBA = 0xff000000 | RGB;
								int c2 = cw2*8 + sc2;
								for(int r1 = r2*2; r1 < (r2+1)*2; r1++){
									for(int c1 = c2*2; c1 < (c2+1)*2; c1++){
										rgba8888[r1][c1] = RGBA;
									}
								}
								w >>= 4;
							}
						}
					}
				}
				break;
			case 3: // RGB333 mode
				if(gpu_mem_map.unpacked_0_packed_1 == 0){
					// Unpacked.
					for(int r4 = 0; r4 < SCREEN_H/4; r4++){
						for(int c4 = 0; c4 < SCREEN_W/4; c4++){
							int idx = r4*(SCREEN_W/4) + c4;
							uint32_t RGB333 = gpu_mem_map.unpack_rgb333[idx];
							uint32_t R3 = RGB333>>0*3 & 07;
							uint32_t G3 = RGB333>>1*3 & 07;
							uint32_t B3 = RGB333>>2*3 & 07;
							uint32_t R8 = R3<<5;
							uint32_t G8 = G3<<5;
							uint32_t B8 = B3<<5;
							uint32_t RGBA8888 = 
								0xff000000
								| B8<<16
								| G8<<8
								| R8<<0;
							for(int r1 = r4*4; r1 < (r4+1)*4; r1++){
								for(int c1 = c4*4; c1 < (c4+1)*4; c1++){
									rgba8888[r1][c1] = RGBA8888;
								}
							}
						}
					}
				}
				break;
			}
			
			print_clock("after copy");
			
#if TWO_THREADS
			// Notify draw thread that it could start drawing.
			rgba8888_filled.notify();
			print_clock("after notify");
#else
			draw_draw();
#endif
#if SHOW_HEAD_UP_DISPLAY
			fps = round(1000000.0/clock.getElapsedTime().asMicroseconds());
#endif
			
			// Wait end of frame.
			wait_until_us(FRAME_US);
			
#if SHOW_HEAD_UP_DISPLAY
			Hz = round(1000000.0/clock.getElapsedTime().asMicroseconds());
#endif
			frame++;
		}
		
	}
	

public:
	
	LPRS2_GPU_and_Joypad_Emulator() {
		// For testing.
#if 0
		printf(
			"palette offset = 0x%08x\n", 
			offsetof(lprs2_gpu_mem_map, palette)
		);
		printf(
			"pack_idx1 offset = 0x%08x\n", 
			offsetof(lprs2_gpu_mem_map, pack_idx1)
		);
		printf(
			"unpack_idx1 offset = 0x%08x\n", 
			offsetof(lprs2_gpu_mem_map, unpack_idx1)
		);
#endif
		static_assert(
			offsetof(lprs2_gpu_mem_map, palette) == 0x00001000,
			"Wrong address of palette!"
		);
		static_assert(
			offsetof(lprs2_gpu_mem_map, unpack_idx1) == 0x00400000,
			"Wrong address of unpack_idx1!"
		);
		static_assert(
			offsetof(lprs2_gpu_mem_map, pack_idx1) == 0x00600000,
			"Wrong address of pack_idx1!"
		);
		static_assert(
			offsetof(lprs2_gpu_mem_map, unpack_idx4) == 0x00800000,
			"Wrong address of unpack_idx4!"
		);
		static_assert(
			offsetof(lprs2_gpu_mem_map, pack_idx4) == 0x00a00000,
			"Wrong address of pack_idx4!"
		);
		static_assert(
			offsetof(lprs2_gpu_mem_map, unpack_rgb333) == 0x00c00000,
			"Wrong address of unpack_rgb333!"
		);
		static_assert(
			sizeof(lprs2_gpu_mem_map) == 0x01000000,
			"Wrong size of lprs2_gpu_mem_map!"
		);
		
		
		memset((void*)&gpu_mem_map, 0, sizeof(gpu_mem_map));
		gpu_mem_map.hud_color = 0x00ff00ff; // Default FPS color.
		__lprs_gpu_base = reinterpret_cast<volatile void*>(&gpu_mem_map);
		memset((void*)&joypad_mem_map, 0, sizeof(joypad_mem_map));
		__lprs_joypad_base = reinterpret_cast<volatile void*>(&joypad_mem_map);
		
		memset(rgba8888, 0, sizeof(rgba8888));
		main_thread = new thread([&]{ main(); });
	}
};

static LPRS2_GPU_and_Joypad_Emulator lprs2_gpu_and_joypad_emulator;


///////////////////////////////////////////////////////////////////////////////
