# CortexOS TTY subsystem

## Scope implemented in this branch

CortexOS now has one kernel terminal device, the controlling VGA console (`tty0`).
The kernel TTY owns a termios line discipline rather than making the syscall
layer call the legacy shell line reader directly.

Implemented and testable pieces are:

- `/dev/console`, `/dev/tty0`, and `/dev/tty` open as terminal descriptors;
  descriptors obtained from them remain terminals across `dup2()` and `fork()`.
- `read(2)` on file descriptors 0, 1, and 2, or on an opened TTY, uses the
  console TTY. Canonical reads wait for a complete line; raw reads return
  translated keyboard bytes. Writes go through the console output path.
- Canonical input buffering, raw mode, echo, `ICRNL`/`INLCR`, erase, kill,
  EOF, newline handling, `OPOST|ONLCR`, and a bounded 4096-byte input buffer.
  The default erase byte is Backspace (8), matching the existing PS/2 driver.
- `TCGETS`, `TCSETS`, `TCSETSW`, `TCSETSF`, `TIOCGPGRP`, and `TIOCSPGRP`, with
  libc `tcgetattr`, `tcsetattr`, `tcgetpgrp`, and `tcsetpgrp` wrappers.
- A single controlling-console foreground process group. `setsid`, `setpgid`,
  and foreground-group changes are checked against the caller's session.
  `VINTR` (Ctrl-C) is delivered to that foreground group as `SIGINT` using the
  kernel's current signal representation.
- The old in-kernel shell still uses its existing `keyboard_readline` path, so
  this change does not alter its prompt/editing behavior. User descriptors use
  the new TTY path.

`hw/drivers/tty/tty_line.c` is independent of VGA, PS/2, and process code. The
`tty-check` host test exercises canonical editing, raw mode, echo behavior, and
Ctrl-C delivery.

## Deliberate limitations

This is a small, safe CortexOS TTY layer, not a claim of full POSIX terminal
support. There is currently one console and no PTY master/slave pair, UART TTY,
job-control stop/continue (`SIGTSTP`, `SIGTTIN`, `SIGTTOU`), terminal window
size, modem/flow-control implementation, UTF-8/wide-character editing, or
blocking wait queues. A blocking read polls the existing PS/2 keyboard driver
until input arrives. Signal handler delivery and `sigreturn` remain incomplete
elsewhere in the kernel, so Ctrl-C's runtime effect is limited by that ABI.
QEMU keyboard/input and a user ELF opening `/dev/tty` still require runtime
acceptance on a bootable CortexOS image; the host test only validates the
portable line discipline.
