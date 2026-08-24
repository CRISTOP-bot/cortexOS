/* ===================== MEMORY (CARD MATCH) ===================== */

#define MEM_GRID 4
#define MEM_PAIRS 8

static int mem_cards[MEM_GRID][MEM_GRID];
static int mem_revealed[MEM_GRID][MEM_GRID];
static int mem_matched[MEM_GRID][MEM_GRID];
static int mem_cx, mem_cy;
static int mem_flip1_x, mem_flip1_y;
static int mem_flip2_x, mem_flip2_y;
static int mem_flipped;
static int mem_pairs_found;
static int mem_moves;

static void mem_init(void)
{
	int vals[MEM_GRID * MEM_GRID];
	for (int i = 0; i < MEM_PAIRS; ++i) {
		vals[i * 2] = i + 1;
		vals[i * 2 + 1] = i + 1;
	}
	for (int i = MEM_GRID * MEM_GRID - 1; i > 0; --i) {
		int j = rng_next() % (i + 1);
		int tmp = vals[i];
		vals[i] = vals[j];
		vals[j] = tmp;
	}
	for (int y = 0; y < MEM_GRID; ++y)
		for (int x = 0; x < MEM_GRID; ++x)
			mem_cards[y][x] = vals[y * MEM_GRID + x];
	kmemset(mem_revealed, 0, sizeof(mem_revealed));
	kmemset(mem_matched, 0, sizeof(mem_matched));
	mem_cx = 0;
	mem_cy = 0;
	mem_flip1_x = mem_flip1_y = -1;
	mem_flip2_x = mem_flip2_y = -1;
	mem_flipped = 0;
	mem_pairs_found = 0;
	mem_moves = 0;
}

static const char *mem_symbols[] = {
	"", "*", "#", "@", "$", "%", "&", "~", "+"
};

static void mem_draw(void)
{
	console_print_at(0, 0, "MEMORY  Arrows=Move Space=Flip Q=Quit", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	char info[32];
	char num[8];
	kstrcpy(info, "Pairs: ", sizeof(info));
	kitoa(mem_pairs_found, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	kstrcat(info, "/", sizeof(info));
	kitoa(MEM_PAIRS, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(0, 1, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	kstrcpy(info, "Moves: ", sizeof(info));
	kitoa(mem_moves, num, sizeof(num));
	kstrcat(info, num, sizeof(info));
	console_print_at(25, 1, info, VGA_ATTR(VGA_CYAN, VGA_BLACK));
	for (int y = 0; y < MEM_GRID; ++y) {
		for (int x = 0; x < MEM_GRID; ++x) {
			int px = x * 6 + 5;
			int py = y * 2 + 4;
			unsigned char attr;
			char cell[4];
			if (mem_matched[y][x] || mem_revealed[y][x]) {
				int val = mem_cards[y][x];
				kstrcpy(cell, mem_symbols[val], sizeof(cell));
				attr = mem_matched[y][x] ? VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK) : VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK);
			} else {
				kstrcpy(cell, "?", sizeof(cell));
				attr = VGA_ATTR(VGA_DARK_GREY, VGA_BLACK);
			}
			if (x == mem_cx && y == mem_cy && !mem_matched[y][x])
				attr = (attr & 0x0F) | 0x80;
			console_print_at(px, py, "+----+", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
			char row[8];
			kstrcpy(row, "| ", sizeof(row));
			kstrcat(row, cell, sizeof(row));
			int clen = kstrlen(cell);
			for (int p = clen; p < 2; ++p) kstrcat(row, " ", sizeof(row));
			while (kstrlen(row) < 5) kstrcat(row, " ", sizeof(row));
			kstrcat(row, "|", sizeof(row));
			console_print_at(px, py + 1, row, attr);
			console_print_at(px, py + 2, "+----+", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
		}
	}
}

static void game_memory(void)
{
	console_clear();
	rng_seed();
	mem_init();
	int running = 1;
	int wait_frames = 0;
	while (running) {
		mem_draw();
		if (wait_frames > 0) {
			wait_frame(8);
			wait_frames--;
			if (wait_frames == 0) {
				if (mem_flip1_x >= 0 && mem_flip2_x >= 0) {
					if (mem_cards[mem_flip1_y][mem_flip1_x] == mem_cards[mem_flip2_y][mem_flip2_x]) {
						mem_matched[mem_flip1_y][mem_flip1_x] = 1;
						mem_matched[mem_flip2_y][mem_flip2_x] = 1;
						mem_pairs_found++;
					}
					mem_revealed[mem_flip1_y][mem_flip1_x] = 0;
					mem_revealed[mem_flip2_y][mem_flip2_x] = 0;
					mem_flip1_x = mem_flip1_y = -1;
					mem_flip2_x = mem_flip2_y = -1;
				}
				continue;
			}
			continue;
		}
		if (mem_pairs_found == MEM_PAIRS) {
			mem_draw();
			console_print_at(12, 13, "YOU WIN! All pairs matched!", VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK));
			char info[32];
			char num[8];
			kstrcpy(info, "Completed in ", sizeof(info));
			kitoa(mem_moves, num, sizeof(num));
			kstrcat(info, num, sizeof(info));
			kstrcat(info, " moves", sizeof(info));
			console_print_at(12, 14, info, VGA_ATTR(VGA_YELLOW, VGA_BLACK));
			console_print_at(12, 15, "Press any key...", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
			keyboard_read_char();
			break;
		}
		wait_frame(0);
		if (keyboard_data_available()) {
			int sc = keyboard_read_scancode();
			if (sc == SC_Q || sc == SC_ESCAPE) break;
			if (sc == SC_LEFT  && mem_cx > 0) mem_cx--;
			if (sc == SC_RIGHT && mem_cx < MEM_GRID - 1) mem_cx++;
			if (sc == SC_UP    && mem_cy > 0) mem_cy--;
			if (sc == SC_DOWN  && mem_cy < MEM_GRID - 1) mem_cy++;
			if (sc == SC_SPACE) {
				if (mem_revealed[mem_cy][mem_cx] || mem_matched[mem_cy][mem_cx])
					continue;
				if (mem_flip1_x < 0) {
					mem_flip1_x = mem_cx;
					mem_flip1_y = mem_cy;
					mem_revealed[mem_cy][mem_cx] = 1;
				} else if (mem_flip2_x < 0 && (mem_cx != mem_flip1_x || mem_cy != mem_flip1_y)) {
					mem_flip2_x = mem_cx;
					mem_flip2_y = mem_cy;
					mem_revealed[mem_cy][mem_cx] = 1;
					mem_moves++;
					wait_frames = 2;
				}
			}
		}
	}
}

