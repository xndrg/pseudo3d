// !!! WORKS ONLY ON WINDOWS COMPILED WITH MINGW32 C COMPILER
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define SMATH_IMPLEMENTATION
#include "smath.h"

#define MAP_WIDTH 16
#define MAP_HEIGHT 16
static char map[MAP_WIDTH * MAP_HEIGHT];

#define SCREEN_WIDTH 120
#define SCREEN_HEIGHT 40
static char screen[SCREEN_WIDTH * SCREEN_HEIGHT];
static HANDLE console_handle;

typedef struct {
    float x;
    float y;
    float angle;
} Player;
static Player player = {.x = 8.0f, .y = 8.0f};
#define FOV M_PI / 4.0f
#define DEPTH 16.0f

struct timeval start, end;
long double dt;

// Defined in stdio.h file, that belongs to microsoft visual c++ compiler
// Without this declaration we would get warning, because we use mingw, not cl
int sprintf_s(char*, size_t, const char*, ...);

static void map_init(void)
{
    memcpy(map + 0*MAP_WIDTH,  "################", MAP_WIDTH);
    memcpy(map + 1*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 2*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 3*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 4*MAP_WIDTH,  "#.........######", MAP_WIDTH);
    memcpy(map + 5*MAP_WIDTH,  "####...........#", MAP_WIDTH);
    memcpy(map + 6*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 7*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 8*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 9*MAP_WIDTH,  "#..............#", MAP_WIDTH);
    memcpy(map + 10*MAP_WIDTH, "#......#.......#", MAP_WIDTH);
    memcpy(map + 11*MAP_WIDTH, "#....###.......#", MAP_WIDTH);
    memcpy(map + 12*MAP_WIDTH, "#...........#..#", MAP_WIDTH);
    memcpy(map + 13*MAP_WIDTH, "#...........#..#", MAP_WIDTH);
    memcpy(map + 14*MAP_WIDTH, "#...........#..#", MAP_WIDTH);
    memcpy(map + 15*MAP_WIDTH, "################", MAP_WIDTH);
}

static void console_init(void)
{
    console_handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0,
					       NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    if (!console_handle) {
	fprintf(stderr, "ERROR: could not create console screen buffer\n");
	exit(-1);
    }
    SetConsoleActiveScreenBuffer(console_handle);
}

static void console_write(void)
{
    // We don't care about it, doesn't work if we just pass NULL into function below
    DWORD bytes_written;

    sprintf_s(screen, sizeof(screen), "x=%3.2f, y=%3.2f, angle=%3.2f",
	      player.x, player.y, player.angle);

    for (int x = 0; x < MAP_WIDTH; ++x) {
	for (int y = 0; y < MAP_WIDTH; ++y) {
	    screen[(y + 1)*SCREEN_WIDTH + x] = map[y * MAP_WIDTH + x];
	}
    }
    screen[((int) player.y + 1) * SCREEN_WIDTH + (int) player.x] = 'P';
    
    screen[SCREEN_WIDTH*SCREEN_HEIGHT - 1] = '\0';
    WriteConsoleOutputCharacter(console_handle, screen,
				SCREEN_WIDTH * SCREEN_HEIGHT,
				(COORD){0, 0}, &bytes_written);
}

static void handle_keyboard(void)
{
    if (GetAsyncKeyState((char) 'A') & 0x80000) {
        player.angle -= (0.8f) * dt;
    }
    
    if (GetAsyncKeyState((char) 'D') & 0x80000) {
        player.angle += (0.8f) * dt;
    }

    if (GetAsyncKeyState((char) 'W') & 0x80000) {
	// Check collision with the wall
	Vec2f next_loc = {
	    .x = player.x + sinf(player.angle) * 3.0f * (float)dt,
	    .y = player.y + cosf(player.angle) * 3.0f * (float)dt,
	};
	if (map[(int) next_loc.y * MAP_WIDTH + (int) next_loc.x] == '#') {
	    return;
	}
	
        player.x = next_loc.x;
        player.y = next_loc.y;
    }

    if (GetAsyncKeyState((char) 'S') & 0x80000) {
	Vec2f next_loc = {
	    .x = player.x - sinf(player.angle) * 3.0f * (float)dt,
	    .y = player.y - cosf(player.angle) * 3.0f * (float)dt,
	};
	if (map[(int) next_loc.y * MAP_WIDTH + (int) next_loc.x] == '#') {
	    return;
	}
	
        player.x = next_loc.x;
        player.y = next_loc.y;
    }
}

int main(void)
{
    console_init();
    map_init();

    gettimeofday(&start, NULL);
    gettimeofday(&end, NULL);
    
    while (true) {
	// TODO: Very weird way to calculate delta time, need to simplify
	gettimeofday(&end, NULL);
	dt = (end.tv_sec - start.tv_sec) * 1000.0;      // sec to ms
	dt += (end.tv_usec - start.tv_usec) / 1000.0;   // us to ms
	dt /= 1000;
	start = end;
	
	handle_keyboard();
	
	for (int x = 0; x < SCREEN_WIDTH; ++x) {
	    float ray_angle = (player.angle - FOV/2.0f) + ((float) x / (float) SCREEN_WIDTH) * FOV;
	    float distance_to_wall = 0;
	    // Unit vector for ray in player space
	    Vec2f eye = {.x = sinf(ray_angle), .y = cosf(ray_angle)};
	    
	    // Loop until we hit the wall
	    while (distance_to_wall < DEPTH) {
		distance_to_wall += 1.0f;
		Vec2i test = {
		    .x = (int) (player.x + eye.x*distance_to_wall),
		    .y = (int) (player.y + eye.y*distance_to_wall),
		};

		// Test if ray is out of bounds
		if (test.x < 0 || test.x >= MAP_WIDTH || test.y < 0 || test.y >= MAP_HEIGHT) {
		    distance_to_wall = DEPTH;
		    break;
		} else {
		    if (map[test.y*MAP_WIDTH + test.x] == '#') {
			break;
		    }
		}
	    }

	    int ceiling = (float) (SCREEN_HEIGHT/2.0f - SCREEN_HEIGHT/distance_to_wall);
	    int floor = SCREEN_HEIGHT - ceiling;
	    // Change symbol depend on distance to the wall
	    uint8_t wall_shade;
	    if (distance_to_wall <= DEPTH / 4.0f)     wall_shade = 219;
	    else if (distance_to_wall < DEPTH / 3.0f) wall_shade = 178;
	    else if (distance_to_wall < DEPTH / 2.0f) wall_shade = 177;
	    else if (distance_to_wall < DEPTH)        wall_shade = 176;
	    else                                      wall_shade = ' ';
		
	    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
		if (y < ceiling) {
		    screen[y*SCREEN_WIDTH + x] = ' ';
		} else if (ceiling < y && y <= floor) {
		    screen[y*SCREEN_WIDTH + x] = wall_shade;
		} else {
		    // Shade floor based on distance
		    uint8_t floor_shade;
		    float b = 1.0f - (((float) y - SCREEN_HEIGHT/2.0f) / ((float) SCREEN_HEIGHT / 2.0f));
		    if (b < 0.25)      floor_shade = '#';
		    else if (b < 0.5)  floor_shade = 'x';
		    else if (b < 0.75) floor_shade = '.';
		    else if (b < 0.9)  floor_shade = '-';
		    else               floor_shade = ' ';
		    screen[y*SCREEN_WIDTH + x] = floor_shade;
		}
	    }
	}
	
	console_write();
    }
    
    return 0;
}
