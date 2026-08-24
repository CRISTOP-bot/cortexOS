#include "games.h"
#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "kstring.h"
#include "asm.h"

static unsigned int rng_state;

static unsigned int rng_next(void)
{
	rng_state = rng_state * 1103515245 + 12345;
	return (rng_state >> 16) & 0x7FFF;
}

static void rng_seed(void)
{
	rng_state = (unsigned int)timer_get_ticks();
	if (rng_state == 0)
		rng_state = 0xDEADBEEF;
}

static int scancode_to_digit(int sc)
{
	switch (sc) {
	case SC_1: return 1;
	case SC_2: return 2;
	case SC_3: return 3;
	case SC_4: return 4;
	case SC_5: return 5;
	case SC_6: return 6;
	case SC_7: return 7;
	case SC_8: return 8;
	case SC_9: return 9;
	default:  return -1;
	}
}

static void wait_frame(int ticks)
{
	unsigned long end = timer_get_ticks() + ticks;
	while (timer_get_ticks() < end) {
		if (keyboard_data_available())
			break;
	}
}

#include "games/game_01.inc"
#include "games/game_02.inc"
#include "games/game_03.inc"
#include "games/game_04.inc"
#include "games/game_05.inc"
#include "games/game_06.inc"
#include "games/game_07.inc"
#include "games/game_08.inc"
#include "games/game_09.inc"
