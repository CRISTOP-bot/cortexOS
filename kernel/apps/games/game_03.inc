/* ===================== PONG ===================== */

#define PONG_W 40
#define PONG_H 20
#define PADDLE_H 4

static int pong_py, pong_ay;
static int pong_bx, pong_by;
static int pong_bdx, pong_bdy;
static int pong_ps, pong_as;

static void pong_reset(void)
{
	pong_py = PONG_H / 2 - PADDLE_H / 2;
	pong_ay = PONG_H / 2 - PADDLE_H / 2;
	pong_bx = PONG_W / 2;
	pong_by = PONG_H / 2;
	pong_bdx = 1;
	pong_bdy = 1;
}

static void pong_draw(void)
{
	for (int y = 0; y <= PONG_H; ++y) {
		for (int x = 0; x <= PONG_W; ++x) {
			char c = ' ';
			unsigned char attr = VGA_DEFAULT_ATTR;
			if (x == 0 || x == PONG_W) {
				c = '|';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (y == 0 || y == PONG_H) {
				c = '-';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (x == 1 && y >= pong_py && y < pong_py + PADDLE_H) {
				c = 0xDB;
				attr = VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK);
			} else if (x == PONG_W - 1 && y >= pong_ay && y < pong_ay + PADDLE_H) {
				c = 0xDB;
				attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
			} else if (x == pong_bx && y == pong_by) {
				c = 'O';
				attr = VGA_ATTR(VGA_WHITE, VGA_BLACK);
			}
			console_putxy(x + 20, y + 2, c, attr);
		}
	}
	char score_l[32], score_r[32];
	char num1[8], num2[8];
	kitoa(pong_ps, num1, sizeof(num1));
	kitoa(pong_as, num2, sizeof(num2));
	kstrcpy(score_l, "Player: ", sizeof(score_l));
	kstrcat(score_l, num1, sizeof(score_l));
	kstrcpy(score_r, "CPU: ", sizeof(score_r));
	kstrcat(score_r, num2, sizeof(score_r));
	console_print_at(22, 1, score_l, VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK));
	console_print_at(52, 1, score_r, VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
	console_print_at(0, 0, "PONG  W/S=Move Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
}

static void game_pong(void)
{
	console_clear();
	pong_ps = 0;
	pong_as = 0;
	pong_reset();
	int running = 1;
	while (running) {
		pong_draw();
		wait_frame(4);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_W && pong_py > 1) pong_py--;
			if (sc == SC_S && pong_py < PONG_H - PADDLE_H - 1) pong_py++;
		}
		if (pong_ay + PADDLE_H / 2 < pong_by)
			pong_ay++;
		else if (pong_ay + PADDLE_H / 2 > pong_by)
			pong_ay--;
		if (pong_ay < 1) pong_ay = 1;
		if (pong_ay > PONG_H - PADDLE_H - 1) pong_ay = PONG_H - PADDLE_H - 1;
		pong_bx += pong_bdx;
		pong_by += pong_bdy;
		if (pong_by <= 1 || pong_by >= PONG_H - 1)
			pong_bdy = -pong_bdy;
		if (pong_bx == 2 && pong_by >= pong_py && pong_by < pong_py + PADDLE_H) {
			pong_bdx = 1;
			pong_bdy = (pong_by - pong_py - PADDLE_H / 2);
		}
		if (pong_bx == PONG_W - 2 && pong_by >= pong_ay && pong_by < pong_ay + PADDLE_H) {
			pong_bdx = -1;
			pong_bdy = (pong_by - pong_ay - PADDLE_H / 2);
		}
		if (pong_bx <= 0) {
			pong_as++;
			pong_reset();
		}
		if (pong_bx >= PONG_W) {
			pong_ps++;
			pong_reset();
		}
		if (pong_ps >= 10 || pong_as >= 10)
			running = 0;
	}
	pong_draw();
	const char *winner = (pong_ps >= 10) ? "PLAYER WINS!" : "CPU WINS!";
	console_print_at(30, 12, winner, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print_at(28, 13, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

