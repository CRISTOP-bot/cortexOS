# Doom port milestone

## Scope and current status

This milestone starts the CortexOS port boundary for the original id Software
Doom engine. It is intentionally an honest, non-playable integration step:

- `project/third_party/doom` is a pinned submodule containing the upstream GPLv2 source.
- `userspace/usr/doom/cortexos_platform.h` and `.c` define a CortexOS-owned platform
  boundary for PS/2 input, an indexed framebuffer copy, ticks, and file reads.
- `make doom-platform-check` builds and runs a host validation program for the
  control mapper and framebuffer primitive.
- `make doom-source` validates that the source submodule is initialized and is
  pinned to the documented upstream revision.
- `make doom-check` runs both checks.

There is **no playable Doom binary**, no Doom engine build, no IWAD in this
repository, and no claim that the current kernel can launch this port. The
platform callbacks are not yet connected to a CortexOS userspace process or to
a hardware linear framebuffer. A framebuffer copy primitive is not a video
mode implementation.

## Source and licensing

The source is obtained from the public id Software repository:

    https://github.com/id-Software/DOOM.git

The submodule is pinned to commit `a77dfb96cb91780ca334d0d4cfd86957558007e0`.
The original engine source is GPLv2; retain its notices and consult
`project/third_party/doom/LICENSE.TXT` before redistributing a combined work. CortexOS
files in this milestone remain under the repository license unless a file says
otherwise.

The Doom engine is separate from its game data. IWAD/PWAD files (for example,
`DOOM.WAD`) are copyrighted and are **not** copied, generated, committed,
staged into the rootfs, or distributed by CortexOS. A future runtime must take a
compatible IWAD supplied by the user through an explicit path or deployment
step. Do not download or add commercial game data to this repository or CI.

## Controls contract

The platform layer consumes PS/2 set-1 make/break scancodes from the existing
CortexOS keyboard driver. The `pressed` field distinguishes make (`1`) from
break (`0`). The intended Doom action mapping is:

| Action | Keys |
|---|---|
| Forward / backward | `W` / Up; `S` / Down |
| Turn | Left / Right arrows |
| Strafe | `A` / `D` |
| Fire | Left Ctrl or Space |
| Use / open | `E` or Enter |
| Run | Left Shift |
| Weapons | `1` through `7` |
| Menu | Escape |
| Pause | `P` |

This is a platform mapping contract, not yet a playable in-game binding. Mouse,
audio, networking, savegames, demo recording, configurable bindings, and
joystick/gamepad input are not implemented.

## Audit of CortexOS dependencies

| Area | Current evidence | Doom impact |
|---|---|---|
| Architecture | x86_64 GRUB path is the only complete experimental kernel; AArch64 has an independent early/PMM/EL0/SVC image | First runtime target should be x86_64; AArch64 is not ready for this userspace |
| Third-party source | Bash, OpenRC, and Fastfetch submodules; Doom now pinned separately | No Doom source is copied into CortexOS; submodule must be initialized |
| Compiler/libc/userspace | GCC-hosted freestanding kernel; small static ELF/libc smoke tests in `userspace/usr/`; no complete POSIX process runtime | Engine needs a bounded libc/ABI port, argument/env setup, heap and process launch |
| Graphics | VGA text console and experimental TUI/GUI; no stable user framebuffer ABI | Add a user-visible 8-bit framebuffer mode and ownership/lifetime rules |
| Keyboard | x86_64 PS/2 set-1 IRQ path and scancode API (`core/kernel/include/keyboard.h`) | The mapping in `userspace/usr/doom` is ready for an adapter, but event delivery is not |
| Timer | x86_64 PIT tick API; AArch64 Generic Timer exists only in early path | Add a monotonic userspace clock and 35 Hz/engine tic scheduling |
| Filesystem/rootfs | Experimental VFS/CRFS and rootfs copied to RAM; regular-file ABI is partial | Add read-only IWAD path and bounded file reads; never package IWAD data |
| Build system | Makefile builds x86_64 core/kernel/ISO, userspace smoke tests, and early ARM images | Doom source build and engine object list remain future work |

## Next blockers (in order)

1. Complete a user process/ELF launch path and the libc calls required by the
   original engine, without weakening the current ABI validation.
2. Define a stable indexed framebuffer syscall/device and connect keyboard
   events plus timer ticks to userspace.
3. Adapt the original `i_system.c`, `i_video.c`, `i_input.c` equivalent, and
   file/audio backends through this boundary; keep upstream engine logic
   separate from CortexOS glue.
4. Add an opt-in local IWAD path to the test runner. CI must test only source,
   platform, and synthetic data; it must never acquire or redistribute an IWAD.
5. Build and boot a real x86_64 userspace Doom binary in QEMU, then add a
   documented user-provided-IWAD play test. Only then may the project call it
   playable.
