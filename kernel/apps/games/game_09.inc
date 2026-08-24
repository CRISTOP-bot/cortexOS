/* ===================== MENU ===================== */

void games_menu(void)
{
	console_clear();
	int running = 1;
	while (running) {
		console_clear();
		console_print_at(0, 0,  "========================================", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 1,  "   _   _            _   _  ____         ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 2,  "  | \\ | | ___  _ __| | | |/ ___|       ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 3,  "  |  \\| |/ _ \\| '__| | | | |  _        ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 4,  "  | |\\  | (_) | |  | |_| | |_| |       ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 5,  "  |_| \\_|\\___/|_|   \\___/ \\____|       ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 6,  "========================================", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		console_print_at(0, 7,  "              G A M E S", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 9,  "  1. Snake", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 10, "  2. Tetris", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 11, "  3. Pong", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 12, "  4. 2048", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 13, "  5. Tic-Tac-Toe", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 14, "  6. Minesweeper", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 15, "  7. Breakout", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 16, "  8. Memory", VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print_at(0, 18, "  Q. Back to shell", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
		console_print_at(0, 20, "Select: ", VGA_DEFAULT_ATTR);
		while (1) {
			if (keyboard_data_available()) {
				int sc = keyboard_read_scancode();
				if (sc == SC_Q || sc == SC_ESCAPE) { running = 0; break; }
				if (sc == SC_1) { game_snake(); break; }
				if (sc == SC_2) { game_tetris(); break; }
				if (sc == SC_3) { game_pong(); break; }
				if (sc == SC_4) { game_2048(); break; }
				if (sc == SC_5) { game_tictactoe(); break; }
				if (sc == SC_6) { game_minesweeper(); break; }
				if (sc == SC_7) { game_breakout(); break; }
				if (sc == SC_8) { game_memory(); break; }
			}
		}
	}
	console_clear();
}

void game_launch_snake(void) { game_snake(); console_clear(); }
void game_launch_tetris(void) { game_tetris(); console_clear(); }
void game_launch_pong(void) { game_pong(); console_clear(); }
void game_launch_2048(void) { game_2048(); console_clear(); }
void game_launch_ttt(void) { game_tictactoe(); console_clear(); }
void game_launch_minesweeper(void) { game_minesweeper(); console_clear(); }
void game_launch_breakout(void) { game_breakout(); console_clear(); }
void game_launch_memory(void) { game_memory(); console_clear(); }
