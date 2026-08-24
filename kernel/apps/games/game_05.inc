/* ===================== TIC TAC TOE ===================== */

static int ttt_board[3][3];
static int ttt_turn;

static int ttt_check(void)
{
	for (int i = 0; i < 3; ++i) {
		if (ttt_board[i][0] && ttt_board[i][0] == ttt_board[i][1] && ttt_board[i][0] == ttt_board[i][2])
			return ttt_board[i][0];
		if (ttt_board[0][i] && ttt_board[0][i] == ttt_board[1][i] && ttt_board[0][i] == ttt_board[2][i])
			return ttt_board[0][i];
	}
	if (ttt_board[0][0] && ttt_board[0][0] == ttt_board[1][1] && ttt_board[0][0] == ttt_board[2][2])
		return ttt_board[0][0];
	if (ttt_board[0][2] && ttt_board[0][2] == ttt_board[1][1] && ttt_board[0][2] == ttt_board[2][0])
		return ttt_board[0][2];
	return 0;
}

static int ttt_minimax(int depth, int is_max)
{
	int winner = ttt_check();
	if (winner == 2) return 10 - depth;
	if (winner == 1) return depth - 10;
	int full = 1;
	for (int y = 0; y < 3; ++y)
		for (int x = 0; x < 3; ++x)
			if (!ttt_board[y][x]) full = 0;
	if (full) return 0;
	if (is_max) {
		int best = -11;
		for (int y = 0; y < 3; ++y)
			for (int x = 0; x < 3; ++x) {
				if (!ttt_board[y][x]) {
					ttt_board[y][x] = 2;
					int v = ttt_minimax(depth + 1, 0);
					if (v > best) best = v;
					ttt_board[y][x] = 0;
				}
			}
		return best;
	} else {
		int best = 11;
		for (int y = 0; y < 3; ++y)
			for (int x = 0; x < 3; ++x) {
				if (!ttt_board[y][x]) {
					ttt_board[y][x] = 1;
					int v = ttt_minimax(depth + 1, 1);
					if (v < best) best = v;
					ttt_board[y][x] = 0;
				}
			}
		return best;
	}
}

static void ttt_cpu_move(void)
{
	int best_score = -11;
	int best_x = -1, best_y = -1;
	for (int y = 0; y < 3; ++y)
		for (int x = 0; x < 3; ++x) {
			if (!ttt_board[y][x]) {
				ttt_board[y][x] = 2;
				int score = ttt_minimax(0, 0);
				ttt_board[y][x] = 0;
				if (score > best_score) {
					best_score = score;
					best_x = x;
					best_y = y;
				}
			}
		}
	if (best_x >= 0)
		ttt_board[best_y][best_x] = 2;
}

static void ttt_draw(void)
{
	console_print_at(0, 0, "TIC TAC TOE  Numbers=Play Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_at(10, 2, "You=X  CPU=O", VGA_ATTR(VGA_WHITE, VGA_BLACK));
	for (int y = 0; y < 3; ++y) {
		int by = y * 2 + 4;
		console_print_at(14, by,   "   |   |   ", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
		console_print_at(14, by + 1, "---+---+---", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
		for (int x = 0; x < 3; ++x) {
			int bx = x * 4 + 14;
			char c = ' ';
			unsigned char attr = VGA_DEFAULT_ATTR;
			int cell = y * 3 + x + 1;
			char num[2] = {'0' + cell, '\0'};
			if (ttt_board[y][x] == 1) {
				c = 'X';
				attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK);
			} else if (ttt_board[y][x] == 2) {
				c = 'O';
				attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
			} else {
				console_print_at(bx + 1, by, num, VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
				continue;
			}
			console_putxy(bx + 1, by, c, attr);
		}
	}
	console_print_at(14, 11, "1-9=Place X", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
}

static void game_tictactoe(void)
{
	console_clear();
	kmemset(ttt_board, 0, sizeof(ttt_board));
	ttt_turn = 0;
	int running = 1;
	while (running) {
		ttt_draw();
		if (ttt_check() || ttt_turn >= 9) {
			running = 0;
			break;
		}
		wait_frame(0);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			int digit = scancode_to_digit(sc);
			if (digit >= 1 && digit <= 9) {
				int y = (digit - 1) / 3;
				int x = (digit - 1) % 3;
				if (ttt_board[y][x] == 0) {
					ttt_board[y][x] = 1;
					ttt_turn++;
					if (!ttt_check() && ttt_turn < 9)
						ttt_cpu_move();
					ttt_turn++;
				}
			}
		}
	}
	ttt_draw();
	int winner = ttt_check();
	const char *msg;
	unsigned char attr;
	if (winner == 1) { msg = "YOU WIN!"; attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK); }
	else if (winner == 2) { msg = "CPU WINS!"; attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK); }
	else { msg = "DRAW!"; attr = VGA_ATTR(VGA_YELLOW, VGA_BLACK); }
	console_print_at(16, 13, msg, attr);
	console_print_at(13, 14, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

