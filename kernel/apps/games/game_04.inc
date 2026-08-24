/* ===================== 2048 ===================== */

static int game2048_grid[4][4];
static int game2048_score;
static int game2048_best;

static void game2048_spawn(void)
{
	int empty[16];
	int count = 0;
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x)
			if (game2048_grid[y][x] == 0)
				empty[count++] = y * 4 + x;
	if (count == 0) return;
	int idx = rng_next() % count;
	int v = (rng_next() % 10 < 9) ? 2 : 4;
	game2048_grid[empty[idx] / 4][empty[idx] % 4] = v;
}

static void game2048_draw(void)
{
	console_print_at(0, 0, "2048  Arrows=Move R=Restart Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	char score_buf[32];
	char num[16];
	kstrcpy(score_buf, "Score: ", sizeof(score_buf));
	kitoa(game2048_score, num, sizeof(num));
	kstrcat(score_buf, num, sizeof(score_buf));
	console_print_at(0, 1, score_buf, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	kstrcpy(score_buf, "Best:  ", sizeof(score_buf));
	kitoa(game2048_best, num, sizeof(num));
	kstrcat(score_buf, num, sizeof(score_buf));
	console_print_at(25, 1, score_buf, VGA_ATTR(VGA_CYAN, VGA_BLACK));
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			int bx = x * 6 + 1;
			int by = y * 2 + 3;
			console_print_at(bx, by, "+----+", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
			console_print_at(bx, by + 1, "|    |", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
			int val = game2048_grid[y][x];
			if (val > 0) {
				kitoa(val, num, sizeof(num));
				int nlen = kstrlen(num);
				int pad = (4 - nlen) / 2;
				char cell[6];
				int ci = 0;
				for (int p = 0; p < pad; ++p) cell[ci++] = ' ';
				for (int p = 0; num[p]; ++p) cell[ci++] = num[p];
				while (ci < 4) cell[ci++] = ' ';
				cell[ci] = '\0';
				unsigned char attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);
				if (val == 2)    attr = VGA_ATTR(VGA_LIGHT_GREY, VGA_BLACK);
				else if (val == 4)    attr = VGA_ATTR(VGA_YELLOW, VGA_BLACK);
				else if (val == 8)    attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
				else if (val == 16)   attr = VGA_ATTR(VGA_RED, VGA_BLACK);
				else if (val == 32)   attr = VGA_ATTR(VGA_LIGHT_MAGENTA, VGA_BLACK);
				else if (val == 64)   attr = VGA_ATTR(VGA_MAGENTA, VGA_BLACK);
				else if (val >= 128)  attr = VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK);
				console_print_at(bx + 1, by + 1, cell, attr);
			}
		}
	}
}

static int game2048_slide_row(int row[4], int *score_add)
{
	int merged[4] = {0, 0, 0, 0};
	int dst = 0;
	int moved = 0;
	for (int src = 0; src < 4; ++src) {
		if (row[src] == 0) continue;
		if (dst > 0 && row[dst - 1] == row[src] && !merged[dst - 1]) {
			row[dst - 1] *= 2;
			*score_add += row[dst - 1];
			merged[dst - 1] = 1;
			moved = 1;
		} else {
			if (dst != src) moved = 1;
			row[dst] = row[src];
			dst++;
		}
	}
	for (int i = dst; i < 4; ++i)
		row[i] = 0;
	return moved;
}

static int game2048_move(int dir)
{
	int moved = 0;
	int score_add = 0;
	if (dir == 0) {
		for (int y = 0; y < 4; ++y)
			moved |= game2048_slide_row(game2048_grid[y], &score_add);
	} else if (dir == 2) {
		for (int y = 0; y < 4; ++y) {
			int row[4] = {game2048_grid[y][3], game2048_grid[y][2],
			              game2048_grid[y][1], game2048_grid[y][0]};
			moved |= game2048_slide_row(row, &score_add);
			game2048_grid[y][0] = row[3];
			game2048_grid[y][1] = row[2];
			game2048_grid[y][2] = row[1];
			game2048_grid[y][3] = row[0];
		}
	} else if (dir == 1) {
		for (int x = 0; x < 4; ++x) {
			int row[4] = {game2048_grid[0][x], game2048_grid[1][x],
			              game2048_grid[2][x], game2048_grid[3][x]};
			moved |= game2048_slide_row(row, &score_add);
			for (int y = 0; y < 4; ++y)
				game2048_grid[y][x] = row[y];
		}
	} else if (dir == 3) {
		for (int x = 0; x < 4; ++x) {
			int row[4] = {game2048_grid[3][x], game2048_grid[2][x],
			              game2048_grid[1][x], game2048_grid[0][x]};
			moved |= game2048_slide_row(row, &score_add);
			game2048_grid[0][x] = row[3];
			game2048_grid[1][x] = row[2];
			game2048_grid[2][x] = row[1];
			game2048_grid[3][x] = row[0];
		}
	}
	game2048_score += score_add;
	return moved;
}

static int game2048_has_moves(void)
{
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x) {
			if (game2048_grid[y][x] == 0) return 1;
			if (x < 3 && game2048_grid[y][x] == game2048_grid[y][x + 1]) return 1;
			if (y < 3 && game2048_grid[y][x] == game2048_grid[y + 1][x]) return 1;
		}
	return 0;
}

static void game2048_init(void)
{
	kmemset(game2048_grid, 0, sizeof(game2048_grid));
	game2048_score = 0;
	game2048_spawn();
	game2048_spawn();
}

static void game_2048(void)
{
	console_clear();
	rng_seed();
	game2048_best = 0;
	game2048_init();
	int running = 1;
	while (running) {
		game2048_draw();
		wait_frame(0);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_R) { game2048_init(); continue; }
			int dir = -1;
			if (sc == SC_UP)    dir = 1;
			if (sc == SC_DOWN)  dir = 3;
			if (sc == SC_LEFT)  dir = 0;
			if (sc == SC_RIGHT) dir = 2;
			if (dir >= 0) {
				if (game2048_move(dir)) {
					game2048_spawn();
					if (game2048_score > game2048_best)
						game2048_best = game2048_score;
				}
				if (!game2048_has_moves())
					running = 0;
			}
		}
	}
	game2048_draw();
	console_print_at(15, 12, "GAME OVER!", VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
	console_print_at(13, 13, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

