/* ===================== BREAKOUT ===================== */

#define BK_W 38
#define BK_H 22
#define BK_PADDLE_W 6
#define BK_BRICK_ROWS 5
#define BK_BRICK_COLS 8

static int bk_paddle_x;
static int bk_bx, bk_by;
static int bk_bdx, bk_bdy;
static int bk_bricks[BK_BRICK_ROWS][BK_BRICK_COLS];
static int bk_score;
static int bk_lives;
static unsigned char bk_brick_colors[BK_BRICK_ROWS];

static void bk_init(void)
{
	bk_paddle_x = BK_W / 2 - BK_PADDLE_W / 2;
	bk_bx = BK_W / 2;
	bk_by = BK_H - 3;
	bk_bdx = 1;
	bk_bdy = -1;
	bk_score = 0;
	bk_lives = 3;
	unsigned char colors[] = {
		VGA_RED, VGA_LIGHT_RED, VGA_YELLOW, VGA_LIGHT_GREEN, VGA_CYAN
	};
	for (int y = 0; y < BK_BRICK_ROWS; ++y) {
		bk_brick_colors[y] = colors[y];
		for (int x = 0; x < BK_BRICK_COLS; ++x)
			bk_bricks[y][x] = 1;
	}
}

static void bk_draw(void)
{
	for (int y = 0; y <= BK_H; ++y) {
		for (int x = 0; x <= BK_W; ++x) {
			char c = ' ';
			unsigned char attr = VGA_DEFAULT_ATTR;
			if (y == 0 || x == 0 || x == BK_W) {
				c = '#';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (y == BK_H) {
				c = '#';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (y >= 1 && y <= BK_BRICK_ROWS && x >= 1 && x <= BK_W - 1) {
				int brick_x = (x - 1) * BK_BRICK_COLS / (BK_W - 1);
				int brick_y = y - 1;
				if (brick_x >= BK_BRICK_COLS) brick_x = BK_BRICK_COLS - 1;
				if (brick_y < BK_BRICK_ROWS && bk_bricks[brick_y][brick_x]) {
					c = 0xDB;
					attr = VGA_ATTR(bk_brick_colors[brick_y], VGA_BLACK);
				}
			}
			if (y == BK_H - 2 && x >= bk_paddle_x && x < bk_paddle_x + BK_PADDLE_W) {
				c = 0xDB;
				attr = VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK);
			}
			if (x == bk_bx && y == bk_by && y < BK_H - 1) {
				c = 'O';
				attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);
			}
			console_putxy(x + 20, y + 1, c, attr);
		}
	}
	char info[32];
	char num[8];
	kstrcpy(info, "Score: ", sizeof(info));
	kitoa(bk_score, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(22, 0, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	kstrcpy(info, "Lives: ", sizeof(info));
	kitoa(bk_lives, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(40, 0, info, VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
	console_print_at(0, 0, "BREAKOUT  A/D=Move Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
}

static void game_breakout(void)
{
	console_clear();
	bk_init();
	int running = 1;
	int speed = 2;
	int frame = 0;
	while (running) {
		bk_draw();
		frame++;
		if (frame >= speed) {
			frame = 0;
			bk_bx += bk_bdx;
			bk_by += bk_bdy;
			if (bk_bx <= 1) { bk_bdx = 1; bk_bx = 1; }
			if (bk_bx >= BK_W - 1) { bk_bdx = -1; bk_bx = BK_W - 1; }
			if (bk_by <= 1) { bk_bdy = 1; bk_by = 1; }
			if (bk_by == BK_H - 3 && bk_bx >= bk_paddle_x && bk_bx < bk_paddle_x + BK_PADDLE_W) {
				bk_bdy = -1;
				int rel = bk_bx - bk_paddle_x - BK_PADDLE_W / 2;
				bk_bdx = rel < -1 ? -1 : (rel > 1 ? 1 : rel);
				if (bk_bdx == 0) bk_bdx = (rng_next() % 2) ? 1 : -1;
			}
			if (bk_by >= BK_H) {
				bk_lives--;
				if (bk_lives <= 0) { running = 0; break; }
				bk_bx = BK_W / 2;
				bk_by = BK_H - 3;
				bk_bdx = 1;
				bk_bdy = -1;
			}
			int bx_clamped = bk_bx >= BK_W ? BK_W - 1 : bk_bx;
			int by_clamped = bk_by >= BK_H ? BK_H - 1 : bk_by;
			int brick_x = (bx_clamped - 1) * BK_BRICK_COLS / (BK_W - 1);
			int brick_y = by_clamped - 1;
			if (brick_x >= BK_BRICK_COLS) brick_x = BK_BRICK_COLS - 1;
			if (brick_y >= 0 && brick_y < BK_BRICK_ROWS && brick_x >= 0 && brick_x < BK_BRICK_COLS) {
				if (bk_bricks[brick_y][brick_x]) {
					bk_bricks[brick_y][brick_x] = 0;
					bk_bdy = -bk_bdy;
					bk_score += 10 * (BK_BRICK_ROWS - brick_y);
					int remaining = 0;
					for (int y = 0; y < BK_BRICK_ROWS; ++y)
						for (int x = 0; x < BK_BRICK_COLS; ++x)
							if (bk_bricks[y][x]) remaining++;
					if (remaining == 0) { running = 0; break; }
				}
			}
		}
		wait_frame(1);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_A && bk_paddle_x > 1) bk_paddle_x--;
			if (sc == SC_D && bk_paddle_x < BK_W - BK_PADDLE_W) bk_paddle_x++;
		}
	}
	const char *msg = (bk_lives > 0) ? "YOU WIN! All bricks cleared!" : "GAME OVER! No lives left.";
	unsigned char mattr = (bk_lives > 0) ? VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK) : VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
	bk_draw();
	console_print_at(22, 12, msg, mattr);
	console_print_at(22, 13, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

