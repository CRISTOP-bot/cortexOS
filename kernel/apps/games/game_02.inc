/* ===================== TETRIS ===================== */

#define TET_W 12
#define TET_H 22
#define TET_PIECES 7

static unsigned char tetris_field[TET_H][TET_W];
static int tet_piece, tet_rot, tet_px, tet_py;
static int tet_next;
static int tet_score, tet_lines;

static const unsigned char tetrominoes[TET_PIECES][4][4] = {
	{{1,1,1,1},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
	{{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
	{{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
	{{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
	{{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
};

static int tet_size(void)
{
	if (tet_piece == 3) return 2;
	return (tet_piece == 0) ? 4 : 3;
}

static int tet_collides(int piece, int rot, int px, int py)
{
	int sz = (piece == 3) ? 2 : ((piece == 0) ? 4 : 3);
	int r = rot % 4;
	for (int i = 0; i < sz; ++i)
		for (int j = 0; j < sz; ++j) {
			int rx = j, ry = i;
			for (int k = 0; k < r; ++k) {
				int tmp = rx;
				rx = sz - 1 - ry;
				ry = tmp;
			}
			if (!tetrominoes[piece][i][j])
				continue;
			int fx = px + rx, fy = py + ry;
			if (fx < 0 || fx >= TET_W || fy >= TET_H)
				return 1;
			if (fy >= 0 && tetris_field[fy][fx])
				return 1;
		}
	return 0;
}

static void tet_lock(void)
{
	int sz = tet_size();
	int r = tet_rot % 4;
	for (int i = 0; i < sz; ++i)
		for (int j = 0; j < sz; ++j) {
			int rx = j, ry = i;
			for (int k = 0; k < r; ++k) {
				int tmp = rx;
				rx = sz - 1 - ry;
				ry = tmp;
			}
			if (!tetrominoes[tet_piece][i][j])
				continue;
			int fx = tet_px + rx, fy = tet_py + ry;
			if (fy >= 0 && fy < TET_H && fx >= 0 && fx < TET_W)
				tetris_field[fy][fx] = 1;
		}
}

static void tet_clear_lines(void)
{
	int cleared = 0;
	for (int y = TET_H - 1; y >= 0; --y) {
		int full = 1;
		for (int x = 0; x < TET_W; ++x)
			if (!tetris_field[y][x]) { full = 0; break; }
		if (full) {
			cleared++;
			for (int yy = y; yy > 0; --yy)
				for (int x = 0; x < TET_W; ++x)
					tetris_field[yy][x] = tetris_field[yy - 1][x];
			for (int x = 0; x < TET_W; ++x)
				tetris_field[0][x] = 0;
			y++;
		}
	}
	if (cleared == 1) tet_score += 100;
	else if (cleared == 2) tet_score += 300;
	else if (cleared == 3) tet_score += 500;
	else if (cleared >= 4) tet_score += 800;
	tet_lines += cleared;
}

static void tet_spawn(void)
{
	tet_piece = tet_next;
	tet_next = rng_next() % TET_PIECES;
	tet_rot = 0;
	tet_px = TET_W / 2 - tet_size() / 2;
	tet_py = 0;
	if (tet_collides(tet_piece, tet_rot, tet_px, tet_py))
		tet_py = -1;
}

static void tetris_draw(void)
{
	for (int y = 0; y < TET_H; ++y) {
		for (int x = 0; x < TET_W; ++x) {
			char c = ' ';
			unsigned char attr = VGA_DEFAULT_ATTR;
			if (y == 0 || y == TET_H - 1 || x == 0 || x == TET_W - 1) {
				c = '#';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (tetris_field[y][x]) {
				c = 0xDB;
				attr = VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK);
			}
			console_putxy(x, y, c, attr);
		}
	}
	int sz = tet_size();
	int r = tet_rot % 4;
	for (int i = 0; i < sz; ++i)
		for (int j = 0; j < sz; ++j) {
			int rx = j, ry = i;
			for (int k = 0; k < r; ++k) {
				int tmp = rx;
				rx = sz - 1 - ry;
				ry = tmp;
			}
			if (!tetrominoes[tet_piece][i][j])
				continue;
			int dx = tet_px + rx, dy = tet_py + ry;
			if (dx > 0 && dx < TET_W && dy >= 0 && dy < TET_H)
				console_putxy(dx, dy, 0xDB, VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
		}
	char info[32];
	char num[16];
	kstrcpy(info, "Score: ", sizeof(info));
	kitoa(tet_score, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(TET_W + 2, 1, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	kstrcpy(info, "Lines: ", sizeof(info));
	kitoa(tet_lines, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(TET_W + 2, 2, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print_at(TET_W + 2, 4, "Controls:", VGA_ATTR(VGA_WHITE, VGA_BLACK));
	console_print_at(TET_W + 2, 5, "A/D=Move", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_at(TET_W + 2, 6, "W=Rotate", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_at(TET_W + 2, 7, "S=SoftDrop", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_at(TET_W + 2, 8, "Space=Drop", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	console_print_at(TET_W + 2, 9, "Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
}

static void game_tetris(void)
{
	console_clear();
	rng_seed();
	kmemset(tetris_field, 0, sizeof(tetris_field));
	tet_score = 0;
	tet_lines = 0;
	tet_next = rng_next() % TET_PIECES;
	tet_spawn();
	int running = 1;
	int drop_timer = 0;
	int drop_speed = 15;
	while (running) {
		tetris_draw();
		drop_timer++;
		if (drop_timer >= drop_speed) {
			drop_timer = 0;
			if (!tet_collides(tet_piece, tet_rot, tet_px, tet_py + 1)) {
				tet_py++;
			} else {
				tet_lock();
				tet_clear_lines();
				tet_spawn();
				if (tet_collides(tet_piece, tet_rot, tet_px, tet_py)) {
					running = 0;
				}
			}
		}
		wait_frame(2);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_A) {
				if (!tet_collides(tet_piece, tet_rot, tet_px - 1, tet_py))
					tet_px--;
			}
			if (sc == SC_D) {
				if (!tet_collides(tet_piece, tet_rot, tet_px + 1, tet_py))
					tet_px++;
			}
			if (sc == SC_W) {
				if (!tet_collides(tet_piece, tet_rot + 1, tet_px, tet_py))
					tet_rot++;
			}
			if (sc == SC_S) {
				if (!tet_collides(tet_piece, tet_rot, tet_px, tet_py + 1))
					tet_py++;
			}
			if (sc == SC_SPACE) {
				while (!tet_collides(tet_piece, tet_rot, tet_px, tet_py + 1))
					tet_py++;
				tet_lock();
				tet_clear_lines();
				tet_spawn();
				if (tet_collides(tet_piece, tet_rot, tet_px, tet_py))
					running = 0;
			}
			if (sc == SC_P)
				drop_speed = (drop_speed > 5) ? drop_speed - 2 : 3;
		}
	}
	console_print_at(TET_W / 2 - 4, TET_H / 2, "GAME OVER!", VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
	console_print_at(TET_W / 2 - 7, TET_H / 2 + 1, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

