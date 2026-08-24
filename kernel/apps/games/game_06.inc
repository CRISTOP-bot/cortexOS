/* ===================== MINESWEEPER ===================== */

#define MINE_W 9
#define MINE_H 9
#define MINE_COUNT 10

static int mine_field[MINE_H][MINE_W];
static int mine_revealed[MINE_H][MINE_W];
static int mine_flagged[MINE_H][MINE_W];
static int mine_cx, mine_cy;
static int mine_flags_placed;
static int mine_game_over;
static int mine_win;

static void mine_place_mines(void)
{
	int placed = 0;
	while (placed < MINE_COUNT) {
		int x = rng_next() % MINE_W;
		int y = rng_next() % MINE_H;
		if (mine_field[y][x] != -1) {
			mine_field[y][x] = -1;
			placed++;
		}
	}
	for (int y = 0; y < MINE_H; ++y)
		for (int x = 0; x < MINE_W; ++x) {
			if (mine_field[y][x] == -1) continue;
			int count = 0;
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx) {
					int nx = x + dx, ny = y + dy;
					if (nx >= 0 && nx < MINE_W && ny >= 0 && ny < MINE_H && mine_field[ny][nx] == -1)
						count++;
				}
			mine_field[y][x] = count;
		}
}

static void mine_init(void)
{
	kmemset(mine_field, 0, sizeof(mine_field));
	kmemset(mine_revealed, 0, sizeof(mine_revealed));
	kmemset(mine_flagged, 0, sizeof(mine_flagged));
	mine_cx = MINE_W / 2;
	mine_cy = MINE_H / 2;
	mine_flags_placed = 0;
	mine_game_over = 0;
	mine_win = 0;
	mine_place_mines();
}

static void mine_reveal(int x, int y)
{
	if (x < 0 || x >= MINE_W || y < 0 || y >= MINE_H) return;
	if (mine_revealed[y][x] || mine_flagged[y][x]) return;
	mine_revealed[y][x] = 1;
	if (mine_field[y][x] == 0) {
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
				mine_reveal(x + dx, y + dy);
	}
}

static void mine_check_win(void)
{
	int unrevealed = 0;
	for (int y = 0; y < MINE_H; ++y)
		for (int x = 0; x < MINE_W; ++x)
			if (!mine_revealed[y][x]) unrevealed++;
	if (unrevealed == MINE_COUNT)
		mine_win = 1;
}

static void mine_draw(void)
{
	console_print_at(0, 0, "MINESWEEPER  Arrows=Move Space=Reveal F=Flag Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	char info[32];
	char num[8];
	kstrcpy(info, "Mines: ", sizeof(info));
	kitoa(MINE_COUNT - mine_flags_placed, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(0, 1, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	for (int y = 0; y < MINE_H; ++y) {
		for (int x = 0; x < MINE_W; ++x) {
			int px = x * 2 + 2;
			int py = y + 3;
			unsigned char attr = VGA_DEFAULT_ATTR;
			char c = ' ';
			if (mine_revealed[y][x]) {
				if (mine_field[y][x] == -1) {
					c = '*';
					attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
				} else if (mine_field[y][x] == 0) {
					c = '.';
					attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
				} else {
					c = '0' + mine_field[y][x];
					if (mine_field[y][x] <= 2) attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK);
					else if (mine_field[y][x] <= 4) attr = VGA_ATTR(VGA_YELLOW, VGA_BLACK);
					else if (mine_field[y][x] <= 6) attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
					else attr = VGA_ATTR(VGA_RED, VGA_BLACK);
				}
			} else if (mine_flagged[y][x]) {
				c = 'F';
				attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
			}
			if (x == mine_cx && y == mine_cy && !mine_game_over)
				attr = (attr & 0x0F) | ((attr & 0xF0) >> 4) | 0x80;
			console_putxy(px, py, c, attr);
			console_putxy(px + 1, py, ' ', (attr & 0xF0) >> 4);
		}
	}
}

static void game_minesweeper(void)
{
	console_clear();
	rng_seed();
	mine_init();
	int running = 1;
	while (running) {
		mine_draw();
		wait_frame(0);
		if (mine_game_over || mine_win) {
			if (mine_win)
				console_print_at(20, 14, "YOU WIN! All mines cleared!", VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
			else
				console_print_at(20, 14, "BOOM! You hit a mine!", VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
			console_print_at(20, 15, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
			keyboard_read_char();
			break;
		}
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_LEFT  && mine_cx > 0) mine_cx--;
			if (sc == SC_RIGHT && mine_cx < MINE_W - 1) mine_cx++;
			if (sc == SC_UP    && mine_cy > 0) mine_cy--;
			if (sc == SC_DOWN  && mine_cy < MINE_H - 1) mine_cy++;
			if (sc == SC_SPACE) {
				if (mine_flagged[mine_cy][mine_cx]) continue;
				if (mine_field[mine_cy][mine_cx] == -1) {
					mine_revealed[mine_cy][mine_cx] = 1;
					mine_game_over = 1;
				} else {
					mine_reveal(mine_cx, mine_cy);
					mine_check_win();
				}
			}
			if (sc == SC_F) {
				if (!mine_revealed[mine_cy][mine_cx]) {
					if (mine_flagged[mine_cy][mine_cx]) {
						mine_flagged[mine_cy][mine_cx] = 0;
						mine_flags_placed--;
					} else {
						mine_flagged[mine_cy][mine_cx] = 1;
						mine_flags_placed++;
					}
				}
			}
		}
	}
}

