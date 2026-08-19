# CortexOS Doom platform boundary

This directory contains only CortexOS-owned integration code for the original
id Software Doom source. It is not the Doom engine and it does not include an
IWAD or any other game data.

`cortexos_platform.h` exposes:

- PS/2 set-1 scancode to Doom action mapping (arrows/WASD, fire, use, strafe,
  weapon keys, Escape/menu, and P/pause).
- A caller-owned 8-bit indexed framebuffer attachment and nearest-neighbor
  presentation primitive.
- Callback slots for a monotonic clock and bounded file reads.

The callback slots default to safe failure (`0` ticks and `-1` file reads) until
CortexOS services install them. This prevents a host validation build from
pretending that kernel services exist.

Validate it with:

    make doom-platform-check

A playable binary does not exist yet. See `project/Documentation/DOOM_PORT.md` for the
port audit, legal source/data separation, and blockers.
