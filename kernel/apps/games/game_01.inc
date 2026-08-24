/* ===================== SNAKE ===================== */

#define SNAKE_W 40
#define SNAKE_H 20
#define SNAKE_MAX 128

static int snake_field[SNAKE_H][SNAKE_W];
static int snake_x[SNAKE_MAX], snake_y[SNAKE_MAX];
static int snake_len;
static int snake_dir;
static int snake_food_x, snake_food_y;
static int snake_score;
static int snake_speed;

enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

static void snake_place_food(void)
{
	while (1) {
		snake_food_x = rng_next() % (SNAKE_W - 2) + 1;
		snake_food_y = rng_next() % (SNAKE_H - 2) + 1;
		if (snake_field[snake_food_y][snake_food_x] == 0)
			break;
	}
}

static void snake_draw(void)
{
	for (int y = 0; y < SNAKE_H; ++y) {
		for (int x = 0; x < SNAKE_W; ++x) {
			char c;
			unsigned char attr;
			if (y == 0 || y == SNAKE_H - 1 || x == 0 || x == SNAKE_W - 1) {
				c = '#';
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			} else if (x == snake_food_x && y == snake_food_y) {
				c = '*';
				attr = VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK);
			} else if (snake_field[y][x] > 0) {
				c = '@';
				attr = VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK);
			} else {
				c = ' ';
				attr = VGA_DEFAULT_ATTR;
			}
			console_putxy(x + 1, y + 1, c, attr);
		}
	}
	char score_buf[32];
	char num[16];
	kstrcpy(score_buf, "Score: ", sizeof(score_buf));
	kitoa(snake_score, num, sizeof(num));
	kstrcat(score_buf, num, sizeof(score_buf));
	console_print_at(1, 0, score_buf, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print_at(SNAKE_W + 3, 1, "WASD=Move Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
}

static void snake_init(void)
{
	snake_len = 4;
	snake_dir = DIR_RIGHT;
	snake_score = 0;
	snake_speed = 8;
	kmemset(snake_field, 0, sizeof(snake_field));
	for (int i = 0; i < snake_len; ++i) {
		snake_x[i] = SNAKE_W / 2 - i;
		snake_y[i] = SNAKE_H / 2;
		snake_field[snake_y[i]][snake_x[i]] = 1;
	}
	snake_place_food();
}

static void game_snake(void)
{
	console_clear();
	rng_seed();
	snake_init();
	int alive = 1;
	while (alive) {
		snake_draw();
		wait_frame(snake_speed);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_W && snake_dir != DIR_DOWN)  snake_dir = DIR_UP;
			if (sc == SC_S && snake_dir != DIR_UP)    snake_dir = DIR_DOWN;
			if (sc == SC_A && snake_dir != DIR_RIGHT) snake_dir = DIR_LEFT;
			if (sc == SC_D && snake_dir != DIR_LEFT)  snake_dir = DIR_RIGHT;
			if (sc == SC_P) snake_speed = (snake_speed > 3) ? snake_speed - 2 : 2;
		}
		int nx = snake_x[0], ny = snake_y[0];
		if (snake_dir == DIR_UP)    ny--;
		if (snake_dir == DIR_DOWN)  ny++;
		if (snake_dir == DIR_LEFT)  nx--;
		if (snake_dir == DIR_RIGHT) nx++;
		if (nx <= 0 || nx >= SNAKE_W - 1 || ny <= 0 || ny >= SNAKE_H - 1) {
			alive = 0;
			break;
		}
		if (snake_field[ny][nx] > 0) {
			alive = 0;
			break;
		}
		if (nx == snake_food_x && ny == snake_food_y) {
			snake_score += 10;
			if (snake_len < SNAKE_MAX) {
				snake_x[snake_len] = snake_x[snake_len - 1];
				snake_y[snake_len] = snake_y[snake_len - 1];
				snake_len++;
			}
			snake_place_food();
			if (snake_speed > 3) snake_speed--;
		}
		for (int i = snake_len - 1; i > 0; --i) {
			snake_x[i] = snake_x[i - 1];
			snake_y[i] = snake_y[i - 1];
		}
		snake_x[0] = nx;
		snake_y[0] = ny;
		kmemset(snake_field, 0, sizeof(snake_field));
		for (int i = 0; i < snake_len; ++i)
			snake_field[snake_y[i]][snake_x[i]] = 1;
	}
	snake_draw();
	char msg[64];
	char num[16];
	kstrcpy(msg, "GAME OVER! Score: ", sizeof(msg));
	kitoa(snake_score, num, sizeof(num));
	kstrcat(msg, num, sizeof(msg));
	console_print_at(10, SNAKE_H / 2 + 1, msg, VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK));
	console_print_at(10, SNAKE_H / 2 + 2, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	keyboard_read_char();
}

