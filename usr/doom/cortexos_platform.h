#ifndef CORTEXOS_DOOM_PLATFORM_H
#define CORTEXOS_DOOM_PLATFORM_H

/*
 * CortexOS platform boundary for the GPLv2 Doom engine source.
 *
 * This header deliberately has no dependency on the kernel headers.  The
 * eventual Doom port can compile the engine with this small boundary while
 * keeping hardware access in CortexOS-owned code.
 */

enum cortexos_doom_action {
	CORTEXOS_DOOM_ACTION_NONE = 0,
	CORTEXOS_DOOM_ACTION_FORWARD,
	CORTEXOS_DOOM_ACTION_BACKWARD,
	CORTEXOS_DOOM_ACTION_TURN_LEFT,
	CORTEXOS_DOOM_ACTION_TURN_RIGHT,
	CORTEXOS_DOOM_ACTION_STRAFE_LEFT,
	CORTEXOS_DOOM_ACTION_STRAFE_RIGHT,
	CORTEXOS_DOOM_ACTION_FIRE,
	CORTEXOS_DOOM_ACTION_USE,
	CORTEXOS_DOOM_ACTION_RUN,
	CORTEXOS_DOOM_ACTION_WEAPON_1,
	CORTEXOS_DOOM_ACTION_WEAPON_2,
	CORTEXOS_DOOM_ACTION_WEAPON_3,
	CORTEXOS_DOOM_ACTION_WEAPON_4,
	CORTEXOS_DOOM_ACTION_WEAPON_5,
	CORTEXOS_DOOM_ACTION_WEAPON_6,
	CORTEXOS_DOOM_ACTION_WEAPON_7,
	CORTEXOS_DOOM_ACTION_MENU,
	CORTEXOS_DOOM_ACTION_PAUSE
};

struct cortexos_doom_input {
	unsigned short scancode;
	unsigned char pressed;
	enum cortexos_doom_action action;
};

/* PS/2 set-1 scancodes used by the current CortexOS keyboard driver. */
#define CORTEXOS_DOOM_SC_ESCAPE 0x01
#define CORTEXOS_DOOM_SC_1      0x02
#define CORTEXOS_DOOM_SC_2      0x03
#define CORTEXOS_DOOM_SC_3      0x04
#define CORTEXOS_DOOM_SC_4      0x05
#define CORTEXOS_DOOM_SC_5      0x06
#define CORTEXOS_DOOM_SC_6      0x07
#define CORTEXOS_DOOM_SC_7      0x08
#define CORTEXOS_DOOM_SC_P      0x19
#define CORTEXOS_DOOM_SC_E      0x12
#define CORTEXOS_DOOM_SC_W      0x11
#define CORTEXOS_DOOM_SC_A      0x1e
#define CORTEXOS_DOOM_SC_S      0x1f
#define CORTEXOS_DOOM_SC_D      0x20
#define CORTEXOS_DOOM_SC_LCTRL  0x1d
#define CORTEXOS_DOOM_SC_LSHIFT 0x2a
#define CORTEXOS_DOOM_SC_SPACE  0x39
#define CORTEXOS_DOOM_SC_ENTER  0x1c
#define CORTEXOS_DOOM_SC_UP     0x48
#define CORTEXOS_DOOM_SC_LEFT   0x4b
#define CORTEXOS_DOOM_SC_RIGHT  0x4d
#define CORTEXOS_DOOM_SC_DOWN   0x50

/* Return non-zero when the scancode is a supported Doom control. */
int cortexos_doom_map_scancode(unsigned short scancode, unsigned char pressed,
                               struct cortexos_doom_input *input);

struct cortexos_doom_framebuffer {
	unsigned char *pixels;
	unsigned int width;
	unsigned int height;
	unsigned int pitch;
};

/* Attach a caller-owned 8-bit indexed framebuffer.  No memory is allocated. */
void cortexos_doom_framebuffer_attach(struct cortexos_doom_framebuffer *fb);

/* Copy an indexed Doom frame to the attached framebuffer with nearest-neighbor
 * scaling.  This is only a pixel-copy primitive; it does not provide a video
 * mode or claim that VGA/linear framebuffer support is complete. */
void cortexos_doom_present(const unsigned char *source, unsigned int width,
                           unsigned int height);

typedef unsigned long (*cortexos_doom_clock_fn)(void *opaque);
typedef int (*cortexos_doom_read_fn)(const char *path, void *buffer,
                                     unsigned long size, unsigned long offset,
                                     void *opaque);

/* Kernel/userspace integration callbacks.  Until installed, ticks return zero
 * and file reads return -1; this makes missing OS services explicit. */
void cortexos_doom_services_install(cortexos_doom_clock_fn clock,
                                    cortexos_doom_read_fn read, void *opaque);
unsigned long cortexos_doom_ticks(void);
int cortexos_doom_read_file(const char *path, void *buffer, unsigned long size,
                            unsigned long offset);

#endif
