#include "cortexos_platform.h"

static struct cortexos_doom_framebuffer *doom_framebuffer;
static cortexos_doom_clock_fn doom_clock;
static cortexos_doom_read_fn doom_read;
static void *doom_services_opaque;

static enum cortexos_doom_action action_for_scancode(unsigned short scancode)
{
	switch (scancode) {
	case CORTEXOS_DOOM_SC_W:
	case CORTEXOS_DOOM_SC_UP: return CORTEXOS_DOOM_ACTION_FORWARD;
	case CORTEXOS_DOOM_SC_S:
	case CORTEXOS_DOOM_SC_DOWN: return CORTEXOS_DOOM_ACTION_BACKWARD;
	case CORTEXOS_DOOM_SC_LEFT: return CORTEXOS_DOOM_ACTION_TURN_LEFT;
	case CORTEXOS_DOOM_SC_RIGHT: return CORTEXOS_DOOM_ACTION_TURN_RIGHT;
	case CORTEXOS_DOOM_SC_A: return CORTEXOS_DOOM_ACTION_STRAFE_LEFT;
	case CORTEXOS_DOOM_SC_D: return CORTEXOS_DOOM_ACTION_STRAFE_RIGHT;
	case CORTEXOS_DOOM_SC_LCTRL:
	case CORTEXOS_DOOM_SC_SPACE: return CORTEXOS_DOOM_ACTION_FIRE;
	case CORTEXOS_DOOM_SC_E:
	case CORTEXOS_DOOM_SC_ENTER: return CORTEXOS_DOOM_ACTION_USE;
	case CORTEXOS_DOOM_SC_LSHIFT: return CORTEXOS_DOOM_ACTION_RUN;
	case CORTEXOS_DOOM_SC_1: return CORTEXOS_DOOM_ACTION_WEAPON_1;
	case CORTEXOS_DOOM_SC_2: return CORTEXOS_DOOM_ACTION_WEAPON_2;
	case CORTEXOS_DOOM_SC_3: return CORTEXOS_DOOM_ACTION_WEAPON_3;
	case CORTEXOS_DOOM_SC_4: return CORTEXOS_DOOM_ACTION_WEAPON_4;
	case CORTEXOS_DOOM_SC_5: return CORTEXOS_DOOM_ACTION_WEAPON_5;
	case CORTEXOS_DOOM_SC_6: return CORTEXOS_DOOM_ACTION_WEAPON_6;
	case CORTEXOS_DOOM_SC_7: return CORTEXOS_DOOM_ACTION_WEAPON_7;
	case CORTEXOS_DOOM_SC_ESCAPE: return CORTEXOS_DOOM_ACTION_MENU;
	case CORTEXOS_DOOM_SC_P: return CORTEXOS_DOOM_ACTION_PAUSE;
	default: return CORTEXOS_DOOM_ACTION_NONE;
	}
}

int cortexos_doom_map_scancode(unsigned short scancode, unsigned char pressed,
                               struct cortexos_doom_input *input)
{
	enum cortexos_doom_action action;

	if (!input)
		return 0;
	action = action_for_scancode(scancode);
	if (action == CORTEXOS_DOOM_ACTION_NONE)
		return 0;
	input->scancode = scancode;
	input->pressed = pressed ? 1 : 0;
	input->action = action;
	return 1;
}

void cortexos_doom_framebuffer_attach(struct cortexos_doom_framebuffer *fb)
{
	doom_framebuffer = fb;
}

void cortexos_doom_present(const unsigned char *source, unsigned int width,
                           unsigned int height)
{
	unsigned int y;

	if (!source || !doom_framebuffer || !doom_framebuffer->pixels ||
	    !width || !height || !doom_framebuffer->width ||
	    !doom_framebuffer->height || doom_framebuffer->pitch < doom_framebuffer->width)
		return;

	for (y = 0; y < doom_framebuffer->height; ++y) {
		unsigned int x;
		unsigned int source_y = (y * height) / doom_framebuffer->height;
		unsigned char *row = doom_framebuffer->pixels +
			y * doom_framebuffer->pitch;
		for (x = 0; x < doom_framebuffer->width; ++x) {
			unsigned int source_x = (x * width) / doom_framebuffer->width;
			row[x] = source[source_y * width + source_x];
		}
	}
}

void cortexos_doom_services_install(cortexos_doom_clock_fn clock,
                                    cortexos_doom_read_fn read, void *opaque)
{
	doom_clock = clock;
	doom_read = read;
	doom_services_opaque = opaque;
}

unsigned long cortexos_doom_ticks(void)
{
	return doom_clock ? doom_clock(doom_services_opaque) : 0;
}

int cortexos_doom_read_file(const char *path, void *buffer, unsigned long size,
                            unsigned long offset)
{
	return doom_read ? doom_read(path, buffer, size, offset, doom_services_opaque) : -1;
}
