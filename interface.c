#include "interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define CYAN \
	(Color) { 0, 145, 143, 255 } // Cyan

#define LIGHTCYAN \
	(Color) { 8, 119, 134, 255 } // Light Cyan

#define LLGRAY \
	(Color) { 220, 220, 220, 255 } // Light Light Gray

#define TRANSPARENT_BLACK \
	(Color) { 0, 0, 0, 200 } // Transparent Black

typedef enum play_mode
{
	NONE,
	AUTO,
	MANUAL
} play_mode;

char *int_to_ascii(int num);
char *remove_spaces(char *str);

// Interface Globals
Texture *textures;
bool first_time = true;
bool first_end = true;
Sound clap;
bool started = false;
State *cur_state = NULL;

// Text Box globals
int framesCounter = 0;
char *puzzle_str = NULL;
int letter_count = 0;
int OLD_MAX = -1;

// Size Box globals
int framsCounterSize = 0;
char size_str[] = {0, 0, 0};
int size_count = 0;

// Mode Globals
play_mode cur_mode = NONE;

// Solver Globals
bool solver_running = false;
int auto_play = 2;

void interface_init()
{
	// Αρχικοποίηση του παραθύρου
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Puzzle");
	SetTargetFPS(60);

	textures = malloc(sizeof(Texture) * 8);

	InitAudioDevice();
	clap = LoadSound("assets/clap.wav");
}

void interface_draw_frame(Graphics *gr_state_ptr, bool play, bool *in_menu)
{
	Graphics gr_state = *gr_state_ptr;

	if (*in_menu)
	{
		if (started == true && gr_state->move_list == NULL)
		{
			if (atoi(size_str) < 2 || atoi(size_str) > 5)
			{
				printf("Invalid size.\n");
				started = false;
				return;
			}

			int N = atoi(size_str);

			free_gra_state(gr_state);

			*gr_state_ptr = create_gra_state(N);
			gr_state = *gr_state_ptr;

			cur_state = malloc(sizeof(State));
			cur_state->board = malloc(sizeof(int *) * N);
			for (int i = 0; i < N; i++)
				cur_state->board[i] = malloc(sizeof(int) * N);
			cur_state->representation = NULL;
			cur_state->moves = 0;
			cur_state->parent = NULL;
			cur_state->size = N;

			int col = 0, row = 0;
			int i = 0, j;
			char buff[100];
			int cur;
			while (puzzle_str[i] != '\0')
			{
				j = 0;
				buff[j] = '\0';

				while (puzzle_str[i] != ' ' && puzzle_str[i] != '\0')
				{
					buff[j] = puzzle_str[i];
					buff[j + 1] = '\0';
					j++;
					i++;
				}
				if (puzzle_str[i] == ' ')
					i++;

				cur = atoi(buff);

				if (cur > N * N - 1 || cur < 0)
				{
					printf("Invalid puzzle.\n");
					started = false;
					destroyfunc(cur_state);
					gr_state_ptr = create_dumy_state();
					letter_count = 0;
					puzzle_str[0] = '\0';
					return;
				}

				cur_state->board[row][col] = cur;

				col++;
				if (col == N)
				{
					col = 0;
					row++;
				}
			}

			if (!IsSolveable(cur_state))
			{
				printf("Puzzle is not solvable\n");
				started = false;
				destroyfunc(cur_state);
				gr_state_ptr = create_dumy_state();
				letter_count = 0;
				puzzle_str[0] = '\0';
				return;
			}

			if (isGoal(*cur_state) == 1)
			{
				printf("This puzzle is already solved\n");
				started = false;
				destroyfunc(cur_state);
				gr_state_ptr = create_dumy_state();
				return;
			}

			if(cur_mode == NONE)
			{
				printf("No mode selected\n");
				started = false;
				destroyfunc(cur_state);
				gr_state_ptr = create_dumy_state();
				return;
			}

			if(cur_mode == AUTO)
			{
				gr_state->move_list = NULL;

				thread_data* arg = malloc(sizeof(thread_data));
				arg->input = cur_state;
				ListPtr* temp = &gr_state->move_list;
				arg->result = temp;
				//arg->menu = in_menu;

				pthread_t thread_id;

				pthread_create(&thread_id, NULL, solve_new, arg);

				//solve_new(arg);

				//free(arg);

				solver_running = true;
				started = false;
			}

			if(cur_mode == MANUAL)
			{
				*in_menu = false;
			}
		}

		int MAX_CHAR = -1;

		// Update the string
		if (gr_state->board_size != -1)
		{
			MAX_CHAR = gr_state->board_size * gr_state->board_size;

			int temp = MAX_CHAR;
			if (temp > 9)
			{
				temp -= 9;
				temp *= 2;
				temp += 8;
			}
			MAX_CHAR--;
			MAX_CHAR += temp;
		}

		if (MAX_CHAR != OLD_MAX && MAX_CHAR > 1)
		{
			OLD_MAX = MAX_CHAR;

			if (puzzle_str != NULL)
				free(puzzle_str);

			puzzle_str = malloc(sizeof(char) * (MAX_CHAR + 1));
			puzzle_str[0] = '\0';
			letter_count = 0;
		}

		int min = 300;

		if (MAX_CHAR != -1)
		{
			char *max_string;
			int temp = 0;

			max_string = malloc(sizeof(char) * MAX_CHAR + 1);
			for (int i = 0; i < MAX_CHAR; i++)
			{
				if (i < 9 + 9) // 9 chars and 9 spaces
				{
					if (i % 2 == 0)
						max_string[i] = '0';
					else
						max_string[i] = ' ';
				}
				else
				{
					if ((i - 20) % 3 == 0)
						max_string[i] = ' ';
					else if (i < 19 + 9) // 19 chars and 9 spaces
					{
						if (temp % 2 == 0)
							max_string[i] = '1';
						else
							max_string[i] = '0';

						temp++;
					}
					else
						max_string[i] = '0';
				}
			}

			max_string[MAX_CHAR] = '\0';

			if (min < MeasureText(max_string, 35))
				min = MeasureText(max_string, 35);

			int max = SCREEN_WIDTH - 50;
			if (min > max)
				min = max;

			free(max_string);
		}

		// Rectangle positions
		Rectangle textBox = (Rectangle){SCREEN_WIDTH / 2 - min / 2, 370, min, 50};
		Rectangle sizeBox = (Rectangle){SCREEN_WIDTH / 2 + (MeasureText("Puzzle Size", 30) + 50 + 10) / 2 - 50, 240, 50, 50};
		Rectangle enterBox = (Rectangle){SCREEN_WIDTH / 2 - 110, 700, 220, 80};
		Rectangle autoBox = (Rectangle){SCREEN_WIDTH / 2 - (200 + 200 + 100) / 2, 500, 200, 140};
		Rectangle manBox = (Rectangle){SCREEN_WIDTH / 2 + (200 + 200 + 100) / 2 - 200, 500, 200, 140};

		// Text box triggers
		bool mouseOnText = false;
		bool mouseOnSize = false;

		// Button triggers
		bool mouseOnAuto = false;
		bool mouseOnMan = false;

		//
		if(solver_running == false)
		{
			// Text Boxes Update

			// Update the mouse position
			if (CheckCollisionPointRec(GetMousePosition(), textBox))
				mouseOnText = true;
			else
				mouseOnText = false;

			if (CheckCollisionPointRec(GetMousePosition(), sizeBox))
				mouseOnSize = true;
			else
				mouseOnSize = false;

			if (mouseOnText)
			{
				// Set the window's cursor to the I-Beam
				SetMouseCursor(MOUSE_CURSOR_IBEAM);

				// Update the puzzle string
				int key = GetCharPressed();

				// Check if more characters have been pressed on the same frame
				while (key > 0)
				{
					// NOTE: Only allow keys in range [48..57] (0..9)
					if ((((key >= '0') && (key <= '9')) || (key == ' ')) && (letter_count < MAX_CHAR))
					{
						puzzle_str[letter_count] = (char)key;
						puzzle_str[letter_count + 1] = '\0'; // Add null terminator at the end of the string.
						letter_count++;
					}

					key = GetCharPressed(); // Check next character in the queue
				}

				if (IsKeyPressed(KEY_BACKSPACE))
				{
					letter_count--;
					if (letter_count < 0)
						letter_count = 0;
					puzzle_str[letter_count] = '\0';
				}
			}
			else if (mouseOnSize)
			{
				// Set the window's cursor to the I-Beam
				SetMouseCursor(MOUSE_CURSOR_IBEAM);

				// Update the puzzle string
				int key = GetCharPressed();

				// Check if more characters have been pressed on the same frame
				while (key > 0)
				{
					// NOTE: Only allow keys in range [48..57] (0..9)
					if ((key >= 48) && (key <= 57) && (size_count < 2))
					{
						size_str[size_count] = (char)key;
						size_str[size_count + 1] = '\0'; // Add null terminator at the end of the string.
						size_count++;
					}

					key = GetCharPressed(); // Check next character in the queue
				}

				if (IsKeyPressed(KEY_BACKSPACE))
				{
					size_count--;
					if (size_count < 0)
						size_count = 0;
					size_str[size_count] = '\0';
				}

				if (size_str[0] != '\0')
					gr_state->board_size = atoi(size_str);
				else
					gr_state->board_size = 0;
			}
			else
				SetMouseCursor(MOUSE_CURSOR_DEFAULT);

			if (mouseOnText)
				framesCounter++;
			else
				framesCounter = 0;

			if (mouseOnSize)
				framsCounterSize++;
			else
				framsCounterSize = 0;

			// Button Updates

			if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), autoBox))
				cur_mode = AUTO;
			else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), manBox))
				cur_mode = MANUAL;

			if (CheckCollisionPointRec(GetMousePosition(), autoBox))
				mouseOnAuto = true;
			else
				mouseOnAuto = false;

			if (CheckCollisionPointRec(GetMousePosition(), manBox))
				mouseOnMan = true;
			else
				mouseOnMan = false;

			if ((CheckCollisionPointRec(GetMousePosition(), enterBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ENTER))
			{
				started = true;
			}
		}

		// Draw the text box
		BeginDrawing();

		ClearBackground(RAYWHITE);

		char *shown_str = puzzle_str;

		int guard = -10;

		if(letter_count < MAX_CHAR)
			guard -= MeasureText("_", 35);

		while (MeasureText(shown_str, 35) > textBox.width + guard)
		{
			shown_str++;
		}

		if (mouseOnText)
		{
			DrawRectangleRec(textBox, CYAN);
			DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, BLACK);
			DrawText(shown_str, (int)textBox.x + 5, (int)textBox.y + 8, 35, RAYWHITE);

			DrawRectangleRec(sizeBox, LLGRAY);
			DrawRectangleLines((int)sizeBox.x, (int)sizeBox.y, (int)sizeBox.width, (int)sizeBox.height, DARKGRAY);
			DrawText(size_str, (int)sizeBox.x + 5, (int)sizeBox.y + 8, 40, CYAN);
		}
		else if (mouseOnSize)
		{
			DrawRectangleRec(sizeBox, CYAN);
			DrawRectangleLines((int)sizeBox.x, (int)sizeBox.y, (int)sizeBox.width, (int)sizeBox.height, BLACK);
			DrawText(size_str, (int)sizeBox.x + 5, (int)sizeBox.y + 8, 40, RAYWHITE);

			DrawRectangleRec(textBox, LLGRAY);
			DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, DARKGRAY);
			DrawText(shown_str, (int)textBox.x + 5, (int)textBox.y + 8, 35, CYAN);
		}
		else
		{
			DrawRectangleRec(textBox, LLGRAY);
			DrawRectangleLines((int)textBox.x, (int)textBox.y, (int)textBox.width, (int)textBox.height, DARKGRAY);
			DrawText(shown_str, (int)textBox.x + 5, (int)textBox.y + 8, 35, CYAN);

			DrawRectangleRec(sizeBox, LLGRAY);
			DrawRectangleLines((int)sizeBox.x, (int)sizeBox.y, (int)sizeBox.width, (int)sizeBox.height, DARKGRAY);
			DrawText(size_str, (int)sizeBox.x + 5, (int)sizeBox.y + 8, 40, CYAN);
		}

		if (MAX_CHAR == -1)
			MAX_CHAR = 0;

		if (letter_count != MAX_CHAR)
			DrawText(TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_CHAR), SCREEN_WIDTH / 2 - MeasureText(TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_CHAR), 20) / 2, textBox.y + textBox.height + 10, 20, DARKGRAY);
		else
			DrawText(TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_CHAR), SCREEN_WIDTH / 2 - MeasureText(TextFormat("INPUT CHARS: %i/%i", letter_count, MAX_CHAR), 20) / 2, textBox.y + textBox.height + 10, 20, MAROON);
		// TODO
		// DrawText(TextFormat("PRESS ENTER TO START"), 315, 275, 20, DARKGRAY);
		// DrawText(TextFormat("PRESS ESC TO EXIT"), 315, 300, 20, DARKGRAY);
		// DrawText(TextFormat("PRESS A TO TOGGLE AUTOSOVLE"), 315, 325, 20, DARKGRAY);
		DrawText(TextFormat("N-Piece Puzzle"), SCREEN_WIDTH / 2 - MeasureText("N-Piece Puzzle", 70) / 2, 25, 70, CYAN);
		DrawText(TextFormat("Puzzle Size"), SCREEN_WIDTH / 2 - (MeasureText("Puzzle Size", 30) + 50 + 10) / 2, sizeBox.y + 15, 30, DARKGRAY);
		DrawText(TextFormat("Current Puzzle"), SCREEN_WIDTH / 2 - MeasureText("Current Puzzle", 30) / 2, textBox.y - 30, 30, DARKGRAY);
		DrawText(TextFormat("Made by GeorgeRG"), SCREEN_WIDTH / 2 - MeasureText("Made by GeorgeRG", 25) / 2, SCREEN_HEIGHT - 35, 25, CYAN);

		if (mouseOnText)
		{
			if (letter_count < MAX_CHAR)
			{
				// Draw blinking underscore char
				if (((framesCounter / 20) % 2) == 0)
					DrawText("_", (int)textBox.x + 8 + MeasureText(shown_str, 35), (int)textBox.y + 12, 35, RAYWHITE);
			}
		}

		if (mouseOnSize)
		{
			if (size_count < 2)
			{
				// Draw blinking underscore char
				if (((framsCounterSize / 20) % 2) == 0)
					DrawText("_", (int)sizeBox.x + 8 + MeasureText(size_str, 40), (int)sizeBox.y + 12, 40, RAYWHITE);
			}
		}

		// Draw the buttons

		if (cur_mode == AUTO)
		{
			DrawRectangleRec(autoBox, CYAN);
			DrawText("AUTO", (int)autoBox.x + ((int)autoBox.width) / 2 - MeasureText("AUTO", 40) / 2, (int)autoBox.y + ((int)autoBox.height) / 2 - 20, 40, RAYWHITE);

			DrawRectangleRec(manBox, LLGRAY);
			DrawText("MANUAL", (int)manBox.x + ((int)manBox.width) / 2 - MeasureText("MANUAL", 40) / 2, (int)manBox.y + ((int)manBox.height) / 2 - 20, 40, CYAN);
		}
		else if (cur_mode == MANUAL)
		{
			DrawRectangleRec(autoBox, LLGRAY);
			DrawText("AUTO", (int)autoBox.x + ((int)autoBox.width) / 2 - MeasureText("AUTO", 40) / 2, (int)autoBox.y + ((int)autoBox.height) / 2 - 20, 40, CYAN);

			DrawRectangleRec(manBox, CYAN);
			DrawText("MANUAL", (int)manBox.x + ((int)manBox.width) / 2 - MeasureText("MANUAL", 40) / 2, (int)manBox.y + ((int)manBox.height) / 2 - 20, 40, RAYWHITE);
		}
		else
		{
			DrawRectangleRec(autoBox, LLGRAY);
			DrawText("AUTO", (int)autoBox.x + ((int)autoBox.width) / 2 - MeasureText("AUTO", 40) / 2, (int)autoBox.y + ((int)autoBox.height) / 2 - 20, 40, CYAN);

			DrawRectangleRec(manBox, LLGRAY);
			DrawText("MANUAL", (int)manBox.x + ((int)manBox.width) / 2 - MeasureText("MANUAL", 40) / 2, (int)manBox.y + ((int)manBox.height) / 2 - 20, 40, CYAN);
		}

		if (mouseOnAuto)
		{
			DrawRectangleLines((int)autoBox.x, (int)autoBox.y, (int)autoBox.width, (int)autoBox.height, BLACK);

			DrawRectangleLines((int)manBox.x, (int)manBox.y, (int)manBox.width, (int)manBox.height, DARKGRAY);
		}
		else if (mouseOnMan)
		{
			DrawRectangleLines((int)autoBox.x, (int)autoBox.y, (int)autoBox.width, (int)autoBox.height, DARKGRAY);

			DrawRectangleLines((int)manBox.x, (int)manBox.y, (int)manBox.width, (int)manBox.height, BLACK);
		}
		else
		{
			DrawRectangleLines((int)autoBox.x, (int)autoBox.y, (int)autoBox.width, (int)autoBox.height, DARKGRAY);

			DrawRectangleLines((int)manBox.x, (int)manBox.y, (int)manBox.width, (int)manBox.height, DARKGRAY);
		}

		DrawFPS(10, 10);

		if (CheckCollisionPointRec(GetMousePosition(), enterBox))
			DrawRectangleRec(enterBox, LIGHTCYAN);
		else
			DrawRectangleRec(enterBox, CYAN);

		DrawRectangleLines((int)enterBox.x, (int)enterBox.y, (int)enterBox.width, (int)enterBox.height, DARKGRAY);
		DrawText("START", (int)enterBox.x + ((int)enterBox.width) / 2 - MeasureText("START", 40) / 2, (int)enterBox.y + ((int)enterBox.height) / 2 - 20, 40, RAYWHITE);

		DrawText("Press ENTER to start", SCREEN_WIDTH / 2 - MeasureText("Press ENTER to start", 20) / 2, (int)enterBox.y + (int)enterBox.height + 10, 20, DARKGRAY);

		if(solver_running)
		{
			Rectangle rec = (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()};
			DrawRectangleRec(rec, TRANSPARENT_BLACK);

			DrawText("Solving...", SCREEN_WIDTH / 2 - MeasureText("Solving...", 30) / 2, SCREEN_HEIGHT / 2 - 30, 30, RAYWHITE);
		
			if(gr_state->move_list != NULL)
			{
				solver_running = false;
				*in_menu = false;
			}
		}

		EndDrawing();
	}
	else if(cur_mode == AUTO)
	{
		State *prev = ListGetNth(gr_state->move_list, 1);

		BeginDrawing();

		// Καθαρισμός, θα τα σχεδιάσουμε όλα από την αρχή
		ClearBackground(CYAN);

		if ((play == false && gr_state->trans.in_transition == false) || isGoal(*prev) || first_time == true)
		{
			for (int i = 0; i < gr_state->board_size; i++)
				for (int j = 0; j < gr_state->board_size; j++)
					if (prev->board[i][j] != 0)
					{
						DrawRectangle(gr_state->positions[i][j].x + 10, gr_state->positions[i][j].y + 10, gr_state->edge - 20, gr_state->edge - 20, LLGRAY);
						char *buf = int_to_ascii(((State*)ListGetNth(gr_state->move_list, 1))->board[i][j]);
						DrawText(
							buf,
							gr_state->positions[i][j].x + gr_state->edge / 2 - MeasureText(buf, (int)(gr_state->edge / 2)) / 2,
							gr_state->positions[i][j].y + gr_state->edge / 2 - 50,
							(int)(gr_state->edge / 2), CYAN);
						free(buf);
					}

			if (isGoal(*prev))
			{
				DrawText(
					"PRESS [ESC] TO EXIT",
					GetScreenWidth() / 2 - MeasureText("PRESS [ESC] TO EXIT", 40) / 2,
					GetScreenHeight() / 2 - 70, 40, RED);

				if (first_end)
				{
					PlaySound(clap);
					first_end = false;
				}
			}
			else if (first_time)
			{
				DrawText(
					"PRESS [ENTER] TO START",
					GetScreenWidth() / 2 - MeasureText("PRESS [ENTER] TO START", 40) / 2,
					GetScreenHeight() / 2 - 70, 40, RED);

				if (IsKeyDown(KEY_ENTER))
					first_time = false;
			}
			else
			{
				DrawText(
					"PRESS [ENTER] TO PROCEED",
					20, 20,
					20, BLUE);

				DrawText(
					"PRESS [A] TO ENABLE AUTO-PLAY",
					GetScreenWidth() - MeasureText("PRESS [A] TO ENABLE AUTO-PLAY", 20) - 20,
					20, 20, BLUE);
			}
		}
		else
		{
			State *next = ListGetNth(gr_state->move_list, 2);

			if (!gr_state->trans.in_transition)
			{
				gr_state->trans.in_transition = true;

				gr_state->trans.offset_row = prev->blank_row - next->blank_row;
				gr_state->trans.offset_col = prev->blank_col - next->blank_col;

				for (int i = 0; i < gr_state->board_size; i++)
					for (int j = 0; j < gr_state->board_size; j++)
						if (prev->board[i][j] != 0)
						{
							DrawRectangle(gr_state->positions[i][j].x + 10, gr_state->positions[i][j].y + 10, gr_state->edge - 20, gr_state->edge - 20, LLGRAY);
							char *buf = int_to_ascii(((State*)ListGetNth(gr_state->move_list, 1))->board[i][j]);
							DrawText(
								buf,
								gr_state->positions[i][j].x + gr_state->edge / 2 - MeasureText(buf, (int)(gr_state->edge / 2)) / 2,
								gr_state->positions[i][j].y + gr_state->edge / 2 - 50,
								(int)(gr_state->edge / 2), CYAN);
							free(buf);
						}
			}
			else
			{
				float off_row = gr_state->trans.offset_row * (gr_state->edge / ANIMATION_SPEED);
				float off_col = gr_state->trans.offset_col * (gr_state->edge / ANIMATION_SPEED);
				gr_state->positions[next->blank_row][next->blank_col].x += off_col;
				gr_state->positions[next->blank_row][next->blank_col].y += off_row;

				for (int i = 0; i < gr_state->board_size; i++)
					for (int j = 0; j < gr_state->board_size; j++)
						if (((State*)ListGetNth(gr_state->move_list, 1))->board[i][j] != 0)
						{
							DrawRectangle(gr_state->positions[i][j].x + 10, gr_state->positions[i][j].y + 10, gr_state->edge - 20, gr_state->edge - 20, LLGRAY);
							char *buf = int_to_ascii(((State*)ListGetNth(gr_state->move_list, 1))->board[i][j]);
							DrawText(
								buf,
								gr_state->positions[i][j].x + gr_state->edge / 2 - MeasureText(buf, (int)(gr_state->edge / 2)) / 2,
								gr_state->positions[i][j].y + gr_state->edge / 2 - 50,
								(int)(gr_state->edge / 2), CYAN);
							free(buf);
						}

				point blank_next = gr_state->positions[next->blank_row][next->blank_col];
				point blank_prev = gr_state->positions[prev->blank_row][prev->blank_col];

				if (blank_next.x == blank_prev.x && blank_next.y == blank_prev.y)
				{
					destroyfunc(ListRemove_nth(gr_state->move_list, 1));
					gr_state->trans.in_transition = false;
					off_row = gr_state->trans.offset_row * gr_state->edge;
					off_col = gr_state->trans.offset_col * gr_state->edge;
					gr_state->positions[next->blank_row][next->blank_col].x -= off_col;
					gr_state->positions[next->blank_row][next->blank_col].y -= off_row;
				}
			}
		}

		DrawRectangle(0,PUZZLE_HEIGHT,SCREEN_WIDTH,SCREEN_HEIGHT-PUZZLE_HEIGHT,LLGRAY);

		EndDrawing();
	}
	else if(cur_mode == MANUAL)
	{
		//Updating 
		State* drawn_state = cur_state;
		
		// Drawing move
		BeginDrawing();

		ClearBackground(CYAN);

		if(gr_state->trans.in_transition == false || isGoal(*drawn_state))
		{
			for (int i = 0; i < gr_state->board_size; i++)
				for (int j = 0; j < gr_state->board_size; j++)
					if (drawn_state->board[i][j] != 0)
					{
						DrawRectangle(gr_state->positions[i][j].x + 10, gr_state->positions[i][j].y + 10, gr_state->edge - 20, gr_state->edge - 20, LLGRAY);
						// DrawTexture(textures[ListGetNth(gr_state->move_list,1)->board[i][j]-1],gr_state->positions[i][j].x,gr_state->positions[i][j].y,WHITE);
						char *buf = int_to_ascii(drawn_state->board[i][j]);
						DrawText(
							buf,
							gr_state->positions[i][j].x + gr_state->edge / 2 - MeasureText(buf, (int)(gr_state->edge / 2)) / 2,
							gr_state->positions[i][j].y + gr_state->edge / 2 - 50,
							(int)(gr_state->edge / 2), CYAN);
						free(buf);
					}
		}

		EndDrawing();
	}
}

void interface_close()
{
	free(textures);
	CloseAudioDevice();
	CloseWindow();
}

char *int_to_ascii(int num)
{
	char *buf;
	if (num <= 9)
	{
		buf = malloc(sizeof(char) * 2);
		buf[0] = num + 48;
		buf[1] = 0;
	}
	else
	{
		buf = malloc(sizeof(char) * 3);
		buf[0] = (num / 10) + 48;
		buf[1] = (num % 10) + 48;
		buf[2] = 0;
	}
	return buf;
}

char *remove_spaces(char *str)
{
	char *temp = malloc(sizeof(char) * strlen(str));
	int i = 0;
	int j = 0;
	while (str[i] != '\0')
	{
		if (str[i] != ' ')
		{
			temp[j] = str[i];
			j++;
		}
		i++;
	}
	temp[j] = '\0';

	char *result = malloc(sizeof(char) * strlen(temp));
	strcpy(result, temp);
	free(temp);
	return result;
}

Graphics create_gra_state(int size)
{
	Graphics gr = malloc(sizeof(gra_state));

	gr->board_size = size;
	gr->trans.in_transition = false;

	// Figuring out where the blocks are going to be
	gr->edge = (float)PUZZLE_HEIGHT / (float)size;

	gr->positions = malloc(sizeof(point *) * size);
	for (int i = 0; i < size; i++)
		gr->positions[i] = malloc(sizeof(point) * size);

	point offset = {0, 0};
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			gr->positions[i][j] = offset;
			offset.x += gr->edge;
		}
		offset.x = 0;
		offset.y += gr->edge;
	}

	return gr;
}

void free_gra_state(Graphics gr)
{
	if (gr != NULL)
	{
		if (gr->move_list != NULL)
			freeList(gr->move_list);

		if (gr->positions != NULL)
		{
			for (int i = 0; i < gr->board_size; i++)
				free(gr->positions[i]);
			free(gr->positions);
		}

		free(gr);
	}
}

Graphics *create_dumy_state(void)
{
	Graphics *gr_state_ptr = malloc(sizeof(Graphics));
	*gr_state_ptr = malloc(sizeof(gra_state));

	Graphics gr_state = *gr_state_ptr;

	gr_state->board_size = -1;
	gr_state->move_list = NULL;
	gr_state->positions = NULL;

	return gr_state_ptr;
}