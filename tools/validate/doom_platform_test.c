#include <libc/stdio.h>
#include <libc/string.h>
#include "../../usr/doom/cortexos_platform.h"

static int expect_action(unsigned short scancode,
                         enum cortexos_doom_action expected)
{
	struct cortexos_doom_input input;
	if (!cortexos_doom_map_scancode(scancode, 1, &input) ||
	    input.action != expected || !input.pressed)
		return 0;
	if (!cortexos_doom_map_scancode(scancode, 0, &input) ||
	    input.action != expected || input.pressed)
		return 0;
	return 1;
}

static int check_framebuffer(void)
{
	unsigned char source[4] = { 1, 2, 3, 4 };
	unsigned char destination[16];
	struct cortexos_doom_framebuffer fb;
	unsigned int i;

	memset(destination, 0, sizeof(destination));
	fb.pixels = destination;
	fb.width = 4;
	fb.height = 4;
	fb.pitch = 4;
	cortexos_doom_framebuffer_attach(&fb);
	cortexos_doom_present(source, 2, 2);
	if (destination[0] != 1 || destination[1] != 1 ||
	    destination[2] != 2 || destination[3] != 2 ||
	    destination[4] != 1 || destination[5] != 1 ||
	    destination[6] != 2 || destination[7] != 2)
		return 0;
	for (i = 8; i < 12; ++i)
		if (destination[i] != (i < 10 ? 3 : 4))
			return 0;
	return destination[12] == 3 && destination[13] == 3 &&
	       destination[14] == 4 && destination[15] == 4;
}

int main(void)
{
	if (!expect_action(CORTEXOS_DOOM_SC_W, CORTEXOS_DOOM_ACTION_FORWARD) ||
	    !expect_action(CORTEXOS_DOOM_SC_UP, CORTEXOS_DOOM_ACTION_FORWARD) ||
	    !expect_action(CORTEXOS_DOOM_SC_A, CORTEXOS_DOOM_ACTION_STRAFE_LEFT) ||
	    !expect_action(CORTEXOS_DOOM_SC_RIGHT, CORTEXOS_DOOM_ACTION_TURN_RIGHT) ||
	    !expect_action(CORTEXOS_DOOM_SC_SPACE, CORTEXOS_DOOM_ACTION_FIRE) ||
	    !expect_action(CORTEXOS_DOOM_SC_ENTER, CORTEXOS_DOOM_ACTION_USE) ||
	    !expect_action(CORTEXOS_DOOM_SC_7, CORTEXOS_DOOM_ACTION_WEAPON_7) ||
	    !expect_action(CORTEXOS_DOOM_SC_ESCAPE, CORTEXOS_DOOM_ACTION_MENU) ||
	    !expect_action(CORTEXOS_DOOM_SC_P, CORTEXOS_DOOM_ACTION_PAUSE) ||
	    cortexos_doom_map_scancode(0x7f, 1, &(struct cortexos_doom_input){0}) ||
	    !check_framebuffer()) {
		fprintf(stderr, "Doom CortexOS platform checks failed\n");
		return 1;
	}
	puts("Doom CortexOS platform checks passed (controls + framebuffer)");
	return 0;
}
