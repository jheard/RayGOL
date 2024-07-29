#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"

#define CELL_COLOR CLITERAL(Color){124,175,227,255}
#define GHOST_COLOR CLITERAL(Color){124, 175, 227, 127}
#define BACK_COLOR CLITERAL(Color){57,126,37,255}
#define LINE_COLOR CLITERAL(Color){65,107,70,255}

#define CELLS 200
#define CELL_COUNT (CELLS * CELLS)
#define CELL_DIM 20
#define BOARD_DIM (CELLS * CELL_DIM)
typedef char board[CELLS][CELLS];

#define MAX_STATES 10000
typedef board state_buf[MAX_STATES];
typedef struct {
	size_t current_state;
	state_buf states;
} state_buffer;

#define tmod(a,b,c) ((a) + (b) + (c)) % (c)
#define state_mod(a,b) tmod(a,b,MAX_STATES)
#define cell_mod(a,b) tmod(a,b,CELLS)

#define currBoard sbuffer->states[sbuffer->current_state]

#define MAX_GLYPH_SIZE 40
enum eglyph_t { ERASE, SINGLE_DOT, GLIDER, 
				LWSS, MWSS, HWSS, 
				PUSH_ALONG, MSIDECAR, EATER, 
				GLIDER_GUN, GLYPH_COUNT};
typedef enum glyph_flip_t {GLYPH_FLIP_N, GLYPH_FLIP_H, GLYPH_FLIP_V, GLYPH_FLIP_B, GLYPH_FLIP_T} glyph_flip;
typedef int glyph_t[MAX_GLYPH_SIZE][MAX_GLYPH_SIZE];
typedef struct {
	size_t w;
	size_t h;
	glyph_t b;
	glyph_flip f;
} golGlyph;
golGlyph glyphs[GLYPH_COUNT];

#define make_glyph(g,nw,nh) {\
	glyphs[g].w=(nw);\
	glyphs[g].h=(nh);\
}

#define set_alive(g,x,y) (glyphs[(g)].b[(x)][(y)] = 1)

int num_neighbors(board b, int x, int y)
{
	int count = 0;
	for (int cy = -1; cy <= 1; cy++)
		for (int cx = -1; cx <= 1; cx++)
		{
			if (cx == 0 && cy == 0)
				continue;
			count += b[cell_mod(x,cx)][cell_mod(y,cy)];
		}
	return count;
}

void update_board(board b, board new_b)
{
	memset(new_b, 0, sizeof(board));
	for (int i = 0; i < CELLS; i++)
		for (int j = 0; j < CELLS; j++)
		{
			int neighbors = num_neighbors(b, i, j);
			if (!b[i][j] && neighbors == 3)
				new_b[i][j] = 1;
			else if (b[i][j] && 2 <= neighbors && neighbors <= 3)
				new_b[i][j] = 1;
		}	
}

void randomize_board(board b)
{
	int r = rand();
	size_t x = 0;
	size_t y = 0;
	size_t i = 0;
	while (x < CELLS)
	{
		y = 0;
		while (y < CELLS)
		{
			b[x][y++] = (r >> i++) & 0x1;
			if (i > 16)
			{
				r = rand();
				i = 0;
			}
		}
		x++;
	}
}

void render_board(board b)
{
	for (int i = 0; i < CELLS; i++)
	{
		for (int j = 0; j < CELLS; j++)
		{
			if (b[i][j])
				DrawRectangle(i * CELL_DIM, j * CELL_DIM, CELL_DIM, CELL_DIM, CELL_COLOR);
		}
	}
	for (int i = 0; i <= BOARD_DIM; i += CELL_DIM)
	{
		DrawLine(i, 0, i, BOARD_DIM, LINE_COLOR);
		DrawLine(0, i, BOARD_DIM, i, LINE_COLOR);
	}
}

size_t count_alive(board b)
{
	size_t count = 0;
	for(int i = 0; i < CELLS; i++)
		for (int j = 0; j < CELLS; j++)
		{
			if (b[i][j])
				count++;
		}
	return count;
}

void advance_state(state_buffer* sbuffer)
{
	size_t next_state = state_mod(sbuffer->current_state,1);
	update_board(currBoard, sbuffer->states[next_state]);
	sbuffer->current_state = next_state;
}

void prev_state(state_buffer* sbuffer)
{
	sbuffer->current_state = state_mod(sbuffer->current_state, -1);
}

void init_glyphs()
{
	make_glyph(ERASE, 1, 1);
	glyphs[ERASE].b[0][0] = 0;

	make_glyph(SINGLE_DOT, 1, 1);
	set_alive(SINGLE_DOT,0,0);

	make_glyph(GLIDER, 3, 3);
	/*  It's a glider   */set_alive(GLIDER,1,0);
						                          set_alive(GLIDER,2,1);
    	set_alive(GLIDER,0,2);set_alive(GLIDER,1,2);set_alive(GLIDER,2,2);

	make_glyph(LWSS, 5, 4);
	                    set_alive(LWSS,1,0);		            		set_alive(LWSS,4,0);
	set_alive(LWSS,0,1);
	set_alive(LWSS,0,2);		            		            		set_alive(LWSS,4,2);
	set_alive(LWSS,0,3);set_alive(LWSS,1,3);set_alive(LWSS,2,3);set_alive(LWSS,3,3);

	make_glyph(MWSS, 6, 5);
	                                                                  set_alive(MWSS, 3, 0);
						  set_alive(MWSS, 1, 1);		                              set_alive(MWSS, 5, 1);
	set_alive(MWSS, 0, 2);
	set_alive(MWSS, 0, 3);                                                                                        set_alive(MWSS, 5, 3);
	set_alive(MWSS, 0, 4);set_alive(MWSS, 1, 4);set_alive(MWSS, 2, 4);set_alive(MWSS, 3, 4);set_alive(MWSS, 4, 4);

	make_glyph(HWSS, 7, 5);
	set_alive(HWSS, 3, 0);	set_alive(HWSS, 4, 0);	set_alive(HWSS, 1, 1);
	set_alive(HWSS, 6, 1);	set_alive(HWSS, 0, 2);	set_alive(HWSS, 0, 3);
	set_alive(HWSS, 6, 3);	set_alive(HWSS, 0, 4);	set_alive(HWSS, 1, 4);
	set_alive(HWSS, 2, 4);	set_alive(HWSS, 3, 4);	set_alive(HWSS, 4, 4);
	set_alive(HWSS, 5, 4);

	make_glyph(PUSH_ALONG, 9, 7);
	set_alive(PUSH_ALONG, 4, 0);set_alive(PUSH_ALONG, 5, 0);set_alive(PUSH_ALONG, 1, 1);
	set_alive(PUSH_ALONG, 2, 1);set_alive(PUSH_ALONG, 4, 1);set_alive(PUSH_ALONG, 5, 1);
	set_alive(PUSH_ALONG, 6, 1);set_alive(PUSH_ALONG, 0, 2);set_alive(PUSH_ALONG, 1, 2);
	set_alive(PUSH_ALONG, 4, 2);set_alive(PUSH_ALONG, 5, 2);set_alive(PUSH_ALONG, 1, 3);
	set_alive(PUSH_ALONG, 2, 3);set_alive(PUSH_ALONG, 5, 3);set_alive(PUSH_ALONG, 6, 3);
	set_alive(PUSH_ALONG, 2, 4);set_alive(PUSH_ALONG, 3, 4);set_alive(PUSH_ALONG, 5, 4);
	set_alive(PUSH_ALONG, 6, 4);set_alive(PUSH_ALONG, 7, 4);set_alive(PUSH_ALONG, 5, 5);
	set_alive(PUSH_ALONG, 7, 5);set_alive(PUSH_ALONG, 6, 6);set_alive(PUSH_ALONG, 7, 6);
	set_alive(PUSH_ALONG, 8, 6);

	make_glyph(MSIDECAR, 8, 7);
	set_alive(MSIDECAR, 5, 0); set_alive(MSIDECAR, 6, 0); set_alive(MSIDECAR, 7, 0);
	set_alive(MSIDECAR, 3, 1); set_alive(MSIDECAR, 4, 1); set_alive(MSIDECAR, 6, 1);
	set_alive(MSIDECAR, 7, 1); set_alive(MSIDECAR, 2, 2); set_alive(MSIDECAR, 7, 2);
	set_alive(MSIDECAR, 1, 3); set_alive(MSIDECAR, 2, 3); set_alive(MSIDECAR, 4, 3);
	set_alive(MSIDECAR, 6, 3); set_alive(MSIDECAR, 0, 4); set_alive(MSIDECAR, 1, 4);
	set_alive(MSIDECAR, 4, 4); set_alive(MSIDECAR, 6, 4); set_alive(MSIDECAR, 1, 5);
	set_alive(MSIDECAR, 2, 5); set_alive(MSIDECAR, 4, 6); set_alive(MSIDECAR, 5, 6);

	make_glyph(EATER, 4, 4);
	set_alive(EATER, 0, 0); set_alive(EATER, 1, 0); set_alive(EATER, 0, 1);
	set_alive(EATER, 2, 1); set_alive(EATER, 2, 2); set_alive(EATER, 2, 3);
	set_alive(EATER, 3, 3);

	make_glyph(GLIDER_GUN, 36, 9);
	set_alive(GLIDER_GUN, 24, 0);	set_alive(GLIDER_GUN, 22, 1);	set_alive(GLIDER_GUN, 24, 1);
	set_alive(GLIDER_GUN, 12, 2);	set_alive(GLIDER_GUN, 13, 2);	set_alive(GLIDER_GUN, 20, 2);
	set_alive(GLIDER_GUN, 21, 2);	set_alive(GLIDER_GUN, 34, 2);	set_alive(GLIDER_GUN, 35, 2);
	set_alive(GLIDER_GUN, 11, 3);	set_alive(GLIDER_GUN, 15, 3);	set_alive(GLIDER_GUN, 20, 3);
	set_alive(GLIDER_GUN, 21, 3);	set_alive(GLIDER_GUN, 34, 3);	set_alive(GLIDER_GUN, 35, 3);
	set_alive(GLIDER_GUN,  0, 4);	set_alive(GLIDER_GUN,  1, 4);	set_alive(GLIDER_GUN, 10, 4);
	set_alive(GLIDER_GUN, 16, 4);	set_alive(GLIDER_GUN, 20, 4);	set_alive(GLIDER_GUN, 21, 4);
	set_alive(GLIDER_GUN,  0, 5);	set_alive(GLIDER_GUN,  1, 5);	set_alive(GLIDER_GUN, 10, 5);
	set_alive(GLIDER_GUN, 14, 5);	set_alive(GLIDER_GUN, 16, 5);	set_alive(GLIDER_GUN, 17, 5);
	set_alive(GLIDER_GUN, 22, 5);	set_alive(GLIDER_GUN, 24, 5);	set_alive(GLIDER_GUN, 10, 6);
	set_alive(GLIDER_GUN, 16, 6);	set_alive(GLIDER_GUN, 24, 6);	set_alive(GLIDER_GUN, 11, 7);
	set_alive(GLIDER_GUN, 15, 7);	set_alive(GLIDER_GUN, 12, 8);	set_alive(GLIDER_GUN, 13, 8);
}

void stamp_glyph(state_buffer* sbuffer, golGlyph* gly,size_t x, size_t y, bool stamp)
{
	for(size_t i=0;i<gly->w;i++)
		for (size_t j = 0; j < gly->h; j++)
		{
			int gx = i;
			int gy = j;
			int cx = i;
			int cy = j;
			switch (gly->f & GLYPH_FLIP_B)
			{
			case GLYPH_FLIP_H: {
				cx = gly->w - i - 1;
				break;
			}
			case GLYPH_FLIP_V: {
				cy = gly->h - j - 1;
				break;
			}
			case GLYPH_FLIP_B: {
				cx = gly->w - i - 1; cy = gly->h - j - 1;
				break;
			}
			default: {
			}
			}
			if (gly->f & GLYPH_FLIP_T)
			{
				gx = j;
				gy = i;
			}
			if (stamp)
			{
				(currBoard)[cell_mod(x, gx)][cell_mod(y, gy)] = gly->b[cx][cy];
			}
			else if (gly->b[cx][cy]) {
				DrawRectangle(cell_mod(x, gx) * CELL_DIM, cell_mod(y, gy) * CELL_DIM, CELL_DIM, CELL_DIM, GHOST_COLOR);
			}
			
		}
}

void stamp_random(state_buffer* sbuffer, size_t glyph_dim, int x, int y)
{
	golGlyph r = {
		.w = glyph_dim,
		.h = glyph_dim,
		.b = { 0 }
	};
	for (int i = 0; i < glyph_dim;i++)
		for (int j = 0; j < glyph_dim; j++)
		{
			if (rand() & 0x1)
			{
				r.b[i][j] = 1;
			}
		}
	stamp_glyph(sbuffer, &r, x, y, true);
}

int WinMain()
{
	srand(GetTime());

	init_glyphs();
	size_t activeGlyph = 1;
	int screenWidth = 600;
	int screenHeight = 600;
	
	state_buffer* sbuffer = malloc(sizeof(state_buffer));
	memset(sbuffer, 0, sizeof(state_buffer));
	stamp_glyph(sbuffer, &glyphs[GLIDER], 75, 75, true);

	board saved_board = { 0 };
	bool saved = false;
	
	char title[20];
	sprintf(title, "RayGOL: %zu", sbuffer->current_state);
	InitWindow(screenWidth, screenHeight, title);
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(30);

	Camera2D cam = { 0 };
	cam.zoom = 0.5f;
	cam.offset = (Vector2){ screenWidth / 2,screenHeight / 2 };
	cam.target = (Vector2){ BOARD_DIM / 2,BOARD_DIM/ 2 };

	bool paused = false;
	while (!WindowShouldClose()) {
		if (IsWindowResized())
		{
			screenHeight = GetScreenHeight();
			screenWidth = GetScreenWidth();
		}
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
		{
			Vector2 delta = GetMouseDelta();
			delta = Vector2Scale(delta, -1.0f / cam.zoom);
			cam.target = Vector2Add(cam.target, delta);
		}
		float wheel = GetMouseWheelMove();
		if (wheel != 0)
		{
			// Get the world point that is under the mouse
			Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), cam);

			// Set the offset to where the mouse is
			cam.offset = GetMousePosition();

			// Set the target to match, so that the camera maps the world space point 
			// under the cursor to the screen space point under the cursor at any zoom
			cam.target = mouseWorldPos;

			// Zoom increment
			float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
			if (wheel < 0) scaleFactor = 1.0f / scaleFactor;
			cam.zoom = Clamp(cam.zoom * scaleFactor, 0.125f, 64.0f);
		}
		int k = GetKeyPressed();
		if (k >= KEY_ZERO && k < (KEY_ZERO+GLYPH_COUNT))
		{
			activeGlyph = k - KEY_ZERO;
		}
		else if (k == KEY_R)
		{
			memset(currBoard, 0, sizeof(board));
		} 
		else if (k ==KEY_SPACE)
		{
			paused = !paused;
		}
		else if (k == KEY_COMMA)
		{
			glyphs[activeGlyph].f ^= GLYPH_FLIP_H;
		}
		else if (k == KEY_PERIOD)
		{
			glyphs[activeGlyph].f ^= GLYPH_FLIP_V;
		}
		else if (k == KEY_SLASH)
		{
			glyphs[activeGlyph].f ^= GLYPH_FLIP_T;
		}
		else if (k == KEY_Y)
		{
			randomize_board(currBoard);
		}
		Vector2 pos = GetScreenToWorld2D(GetMousePosition(), cam);
		int cx = (int)(pos.x / CELL_DIM);
		int cy = (int)(pos.y / CELL_DIM);
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			stamp_glyph(sbuffer, &glyphs[activeGlyph], cx, cy, true);
		}
		if (k >= KEY_KP_0 && k <= KEY_KP_9) {
			stamp_random(sbuffer, k - KEY_KP_0 + 3, cx, cy);
		}
		BeginDrawing();
			ClearBackground(BACK_COLOR);
				BeginMode2D(cam);
					render_board(currBoard);
					stamp_glyph(sbuffer, &glyphs[activeGlyph], cx, cy,  false);
				EndMode2D();
			if (paused) {
				Vector2 pause_pos = CLITERAL(Vector2){ 15,20 };
				DrawRectangle(pause_pos.x,pause_pos.y, 15, 40, RED);
				DrawRectangle(pause_pos.x+20, pause_pos.y, 15, 40, RED);
				DrawText("\
Glyphs:\n\
0 - Dead\n\
1 - Alive\n\
2 - Glider\n\
3 - LWSS\n\
4 - MWSS\n\
5 - HWSS\n\
6 - Pushalong\n\
7 - M. Sidecar\n\
8 - Eater1\n\
9 - GliderGun\n\n\
R - Clear Board\n\
Y - Randomize Board\n\
, - Flip Horizontal\n\
. - Flip Vertical\n\
/ - Transpose\n\n\
While Paused:\n\
N - Forward\n\
B - Backward\n\
S - Save board\n\
L - Load Board\n\
", 15, 80, 18, RAYWHITE);
			if (IsKeyPressed(KEY_N))
				{
					advance_state(sbuffer);
				}
				else if (IsKeyPressed(KEY_B))
				{
					prev_state(sbuffer);
				}
				else if (IsKeyPressed(KEY_S))
				{
					memcpy(saved_board, currBoard, sizeof(board));
					saved = true;
				}
				else if (saved && IsKeyPressed(KEY_L))
				{
					memcpy(currBoard, saved_board, sizeof(board));
				}
			}
			else {
				advance_state(sbuffer);
			}
			char stats[30] = { 0 };
			size_t alive = count_alive(currBoard);
			snprintf(stats, 30, "Alive: %zu\nDead: %zu", alive, CELL_COUNT - alive);
			DrawText(stats, 15, screenHeight - 50, 18, RAYWHITE);
		EndDrawing();
		sprintf(title, "RayGOL: %zu", sbuffer->current_state);
		SetWindowTitle(title);
	}
	CloseWindow();
	free(sbuffer);
	return 0;
}
