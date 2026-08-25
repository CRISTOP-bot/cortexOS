#include "shell.h"
#include "cortex_package.h"
#include "virtualbox.h"
#include "console.h"
#include "keyboard.h"
#include "vfs.h"
#include "lcp.h"
#include "openrc.h"
#include "boot.h"
#include "gui.h"
#include "asm.h"
#include "calc_app.h"
#include "mouse.h"
#include "kstring.h"
#include "timer.h"
#include "fs.h"
#include "pci.h"
#include "games.h"
#include "pmm.h"
#include "vmm.h"
#include "apps.h"
#include "process.h"
#include "serial.h"
#include "persist.h"
#include "net.h"
#include "installer.h"
#include "rust_ffi.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t sys_mem_lower;
extern uint32_t sys_mem_upper;

const char *parse_token(const char *s, char *out, int maxlen)
{
	s = kskip_spaces(s);
	int pos = 0;
	while (*s && *s != ' ' && *s != '\t' && pos + 1 < maxlen)
		out[pos++] = *s++;
	out[pos] = '\0';
	return s;
}

void copy_rest(const char *s, char *out, int maxlen)
{
	s = kskip_spaces(s);
	int pos = 0;
	while (*s && pos + 1 < maxlen)
		out[pos++] = *s++;
	out[pos] = '\0';
}

int parse_int(const char *s)
{
	int value = 0;
	int sign = 1;
	if (*s == '-') {
		sign = -1;
		++s;
	}
	while (*s >= '0' && *s <= '9') {
		int digit = *s - '0';
		if (value > 214748364)
			return sign > 0 ? 2147483647 : (-2147483647 - 1);
		if (value == 214748364 && digit > 7)
			return sign > 0 ? 2147483647 : (-2147483647 - 1);
		value = value * 10 + digit;
		++s;
	}
	return value * sign;
}

bool grep_file(const char *path, const char *pattern)
{
	const void *data = vfs_read(path);
	size_t size = vfs_get_size(path);
	if (!data || size == 0)
		return false;
	const char *text = (const char *)data;
	int line = 1;
	const char *p = text;
	while ((size_t)(p - text) < size) {
		const char *line_start = p;
		while ((size_t)(p - text) < size && *p != '\n')
			++p;
		int line_len = (int)(p - line_start);
		size_t pattern_len = kstrlen(pattern);
		for (int i = 0; i + (int)pattern_len <= line_len; ++i) {
			bool ok = true;
			for (size_t j = 0; j < pattern_len; ++j) {
				if (line_start[i + j] != pattern[j]) {
					ok = false;
					break;
				}
			}
			if (ok) {
				char num[16];
				kitoa(line, num, sizeof(num));
				console_print(num);
				console_print(": ");
				for (int k = 0; k < line_len; ++k)
					console_putchar(line_start[k]);
				console_print("\n");
				break;
			}
		}
		if ((size_t)(p - text) < size && *p == '\n')
			++p;
		++line;
	}
	return true;
}

void handle_asm_command(const char *args)
{
	char op[16];
	char first[32];
	char second[32];
	const char *p = args;
	p = parse_token(p, op, sizeof(op));
	p = parse_token(p, first, sizeof(first));
	p = parse_token(p, second, sizeof(second));
	if (op[0] == '\0' || first[0] == '\0' || second[0] == '\0') {
		console_print("Usage: asm add|sub|mul|div <a> <b>\n");
		return;
	}
	int a = parse_int(first);
	int b = parse_int(second);
	int result = 0;
	if (kstreq(op, "add"))
		result = add_asm(a, b);
	else if (kstreq(op, "sub"))
		result = sub_asm(a, b);
	else if (kstreq(op, "mul"))
		result = mul_asm(a, b);
	else if (kstreq(op, "div"))
		result = div_asm(a, b);
	else {
		console_print("Unknown asm op. Use add, sub, mul, div\n");
		return;
	}
	char out[32];
	kitoa(result, out, sizeof(out));
	console_print("= ");
	console_print(out);
	console_print("\n");
}

void shell_echo(const char *args)
{
	char target[128];
	target[0] = '\0';
	char value[192];
	const char *p = args;
	int vi = 0;
	while (*p && *p != '>' && vi + 1 < (int)sizeof(value))
		value[vi++] = *p++;
	value[vi] = '\0';
	if (*p == '>') {
		++p;
		if (*p == '>')
			++p;
		p = kskip_spaces(p);
		int ti = 0;
		while (*p && *p != ' ' && ti + 1 < (int)sizeof(target))
			target[ti++] = *p++;
		target[ti] = '\0';
	}
	if (target[0])
		vfs_write(target, value, kstrlen(value));
	else {
		console_print(value);
		console_print("\n");
	}
}

void show_mouse_state(void)
{
	struct mouse_state ms;
	mouse_get_state(&ms);
	console_print("Mouse: x=");
	char buf[16];
	kitoa(ms.x, buf, sizeof(buf));
	console_print(buf);
	console_print(" y=");
	kitoa(ms.y, buf, sizeof(buf));
	console_print(buf);
	console_print(" buttons=");
	kitoa(ms.buttons, buf, sizeof(buf));
	console_print(buf);
	console_print("\n");
}

void print_prompt(void)
{
	console_print_color("cortexos", VGA_ATTR(VGA_GREEN, VGA_BLACK));
	console_print_color("@", VGA_DEFAULT_ATTR);
	console_print_color("cortexos", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color(":", VGA_DEFAULT_ATTR);
	console_print_color(vfs_pwd(), VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK));
	console_print_color("$ ", VGA_ATTR(VGA_WHITE, VGA_BLACK));
}

void print_help(void)
{
	console_print_color("\n-- Filesystem --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  ls [-l|-a]   List directory contents\n");
	console_print("  tree         Display directory tree\n");
	console_print("  pwd          Print working directory\n");
	console_print("  cd           Change directory\n");
	console_print("  mkdir        Create directory\n");
	console_print("  rmdir        Remove directory\n");
	console_print("  touch        Create empty file\n");
	console_print("  rm           Remove file\n");
	console_print("  cp           Copy file\n");
	console_print("  mv           Move/rename file\n");
	console_print("  cat          Display file contents\n");
	console_print("  grep         Search in file\n");
	console_print("  echo         Print text or write to file\n");
	console_print("  stat         Display file information\n");
	console_print("  df           Show filesystem usage\n");
	console_print("  chmod        Change file permissions\n");
	console_print("  hexdump      Hex dump of file contents\n");
	console_print("  wc           Word/line/char count\n");
	console_print("  head         Print first N lines\n");
	console_print("  tail         Print last N lines\n");
	console_print_color("\n-- System --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  uname        Display system information\n");
	console_print("  whoami       Display current user\n");
	console_print("  sudo         Execute as root\n");
	console_print("  meminfo      Display memory information\n");
	console_print("  fastfetch    Show system information\n");
	console_print("  lspci        List PCI devices\n");
	console_print("  clear        Clear screen\n");
	console_print("  reboot       Reboot the system\n");
	console_print("  panic        Trigger kernel panic\n");
	console_print_color("\n-- Services --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  openrc       Service manager (OpenRC)\n");
	console_print("  bootctl      Boot manager\n");
	console_print("  lcp          Package manager\n");
	console_print_color("\n-- Misc --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  asm          Arithmetic via assembly\n");
	console_print("  calc         Expression calculator\n");
	console_print("  games        Game menu (all games)\n");
	console_print("  snake        Launch Snake game\n");
	console_print("  tetris       Launch Tetris game\n");
	console_print("  pong         Launch Pong game\n");
	console_print("  2048         Launch 2048 puzzle game\n");
	console_print("  ttt          Launch Tic-Tac-Toe\n");
	console_print("  minesweeper  Launch Minesweeper\n");
	console_print("  breakout     Launch Breakout\n");
	console_print("  memory       Launch Memory card game\n");
	console_print_color("\n-- Applications --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  nano <file>     Text editor\n");
	console_print("  hexview <file>  Hex viewer\n");
	console_print("  fm              File manager\n");
	console_print("  htop            System monitor\n");
	console_print("  calc-tui        Interactive calculator\n");
	console_print("  kblayout     Change keyboard layout\n");
	console_print("  mouse        Show mouse state\n");
	console_print("  gui          Show GUI (experimental)\n");
	console_print_color("\n-- Process Management --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  ps             List running processes\n");
	console_print("  proc create    Create new process\n");
	console_print("  proc list      List all processes\n");
	console_print("  proc kill      Kill process by PID\n");
	console_print("  yield          Yield CPU to scheduler\n");
	console_print("  uptime         Show system uptime\n");
	console_print_color("\n-- Rust --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  rust           Show Rust subcommands\n");
	console_print("  rust info      Rust kernel module info\n");
	console_print("  rust fib <n>   Fibonacci (Rust)\n");
	console_print("  rust fact <n>  Factorial (Rust)\n");
	console_print("  rust prime <n> Primality test (Rust)\n");
	console_print("  rust gcd <a> <b> GCD (Rust)\n");
	console_print("  rust add <a> <b> Addition (Rust)\n");
	console_print("  rust div <a> <b> Division (Rust)\n");
	console_print("  rust strlen <s>  String length (Rust)\n");
	console_print("  rust toupper <s> Uppercase (Rust)\n");
	console_print("  rust tolower <s> Lowercase (Rust)\n");
	console_print_color("\n-- Networking --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  net            Show network interfaces\n");
	console_print("  net ping       Ping loopback\n");
	console_print("  net setip      Set interface IP\n");
	console_print_color("\n-- Storage --\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	console_print("  install        Install CortexOS to a hard disk\n");
	console_print("  serial send    Send data via serial\n");
	console_print("  serial recv    Receive serial data\n");
	console_print("  serial status  Show serial port status\n");
	console_print("  persist save   Save file persistently\n");
	console_print("  persist load   Load persistent file\n");
	console_print("  persist list   List persistent files\n");
	console_print("  persist delete Delete persistent file\n");
	console_print("\n");
}

const char *layout_name(int id)
{
	switch (id) {
	case KB_LAYOUT_US: return "us (English)";
	case KB_LAYOUT_ES: return "es (Spanish)";
	case KB_LAYOUT_DE: return "de (German)";
	default:           return "unknown";
	}
}

static void print_cpu_vendor(char *out)
{
	unsigned int res[4];
	out[0] = '\0';
	cpuid(0, res);
	if (res[0] == 0) return;
	out[0] = (res[1] >> 0) & 0xFF;
	out[1] = (res[1] >> 8) & 0xFF;
	out[2] = (res[1] >> 16) & 0xFF;
	out[3] = (res[1] >> 24) & 0xFF;
	out[4] = (res[3] >> 0) & 0xFF;
	out[5] = (res[3] >> 8) & 0xFF;
	out[6] = (res[3] >> 16) & 0xFF;
	out[7] = (res[3] >> 24) & 0xFF;
	out[8] = (res[2] >> 0) & 0xFF;
	out[9] = (res[2] >> 8) & 0xFF;
	out[10] = (res[2] >> 16) & 0xFF;
	out[11] = (res[2] >> 24) & 0xFF;
	out[12] = '\0';
}

void fastfetch(void)
{
	char buf[32];
	char cpu_vendor[16];

	print_cpu_vendor(cpu_vendor);

	unsigned char attr_label = VGA_ATTR(VGA_CYAN, VGA_BLACK);
	unsigned char attr_val  = VGA_ATTR(VGA_WHITE, VGA_BLACK);
	unsigned char attr_sep  = VGA_DEFAULT_ATTR;

	console_print_color("\n", attr_sep);
	console_print_color("   _   _            _   _  ____   ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("OS:      ", attr_label); console_print_color("CortexOS v3 x86_64\n", attr_val);
	console_print_color("  | \\ | | ___  _ __| | | |/ ___| ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("Kernel:  ", attr_label); console_print_color("CortexOS v3\n", attr_val);
	console_print_color("  |  \\| |/ _ \\| '__| | | | |  _  ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("CPU:     ", attr_label);
	console_print_color(cpu_vendor[0] ? cpu_vendor : "x86_64 (no CPUID)", attr_val);
	console_print("\n");
	console_print_color("  | |\\  | (_) | |  | |_| | |_| | ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("Uptime:  ", attr_label);
	unsigned long ticks = timer_get_ticks();
	unsigned long secs = ticks / 100;
	kitoa(secs, buf, sizeof(buf));
	console_print_color(buf, attr_val);
	console_print_color("s\n", attr_val);
	console_print_color("  |_| \\_|\\___/|_|   \\___/ \\____| ", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	console_print_color("Memory:  ", attr_label);
	if (sys_mem_upper) {
		kitoa((sys_mem_upper + sys_mem_lower) / 1024, buf, sizeof(buf));
		console_print_color(buf, attr_val);
		console_print_color(" MB\n", attr_val);
	} else {
		console_print_color("not detected\n", VGA_ATTR(VGA_DARK_GREY, VGA_BLACK));
	}
	console_print_color("                                  ", attr_label); console_print_color("Shell:   ", attr_label); console_print_color("CortexOS Shell\n", attr_val);
	console_print_color("                                  ", attr_label); console_print_color("Term:    ", attr_label); console_print_color("VGA 80x25\n", attr_val);
	console_print_color("                                  ", attr_label); console_print_color("User:    ", attr_label); console_print_color("cortexos\n", attr_val);
	console_print_color("                                  ", attr_label); console_print_color("Layout:  ", attr_label);
	console_print_color(layout_name(keyboard_get_layout()), attr_val);
	console_print("\n");
	unsigned long nfiles = fs_file_count();
	kitoa(nfiles, buf, sizeof(buf));
	console_print_color("                                  ", attr_label); console_print_color("Files:   ", attr_label);
	console_print_color(buf, attr_val);
	console_print_color(" in rootfs\n", attr_val);
	console_print("\n");
}

void ls_long(const char *path)
{
	const char *target = path && path[0] ? path : vfs_pwd();
	const char *names[32];
	int count = vfs_get_children(target, names, 32);
	if (count == 0) {
		return;
	}
	for (int i = 0; i < count; ++i) {
		char full[128];
		full[0] = '\0';
		kstrcpy(full, target, sizeof(full));
		if (!kstreq(target, "/"))
			kstrcat(full, "/", sizeof(full));
		kstrcat(full, names[i], sizeof(full));

		if (vfs_is_dir(full)) {
			console_print_color("d", VGA_ATTR(VGA_CYAN, VGA_BLACK));
		} else {
			console_print_color("-", VGA_DEFAULT_ATTR);
		}
		console_print("rwxrwxrwx  1 cortexos cortexos  ");

		size_t sz = vfs_get_size(full);
		char num[16];
		kitoa((long)sz, num, sizeof(num));
		int nlen = kstrlen(num);
		for (int p = nlen; p < 8; ++p)
			console_print(" ");
		console_print(num);

		console_print("  ");
		if (vfs_is_dir(full))
			console_print_color(names[i], VGA_ATTR(VGA_CYAN, VGA_BLACK));
		else
			console_print_color(names[i], VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print("\n");
	}
}

void ls_short(const char *path)
{
	const char *target = path && path[0] ? path : vfs_pwd();
	const char *names[32];
	int count = vfs_get_children(target, names, 32);
	for (int i = 0; i < count; ++i) {
		char full[128];
		full[0] = '\0';
		kstrcpy(full, target, sizeof(full));
		if (!kstreq(target, "/"))
			kstrcat(full, "/", sizeof(full));
		kstrcat(full, names[i], sizeof(full));

		if (vfs_is_dir(full))
			console_print_color(names[i], VGA_ATTR(VGA_CYAN, VGA_BLACK));
		else
			console_print_color(names[i], VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print("  ");
	}
	if (count > 0)
		console_print("\n");
}

#define TREE_MAX_DEPTH 8

void tree_print(const char *path, const char *prefix, int is_last, int depth)
{
	if (depth >= TREE_MAX_DEPTH)
		return;
	const char *names[16];
	int count = vfs_get_children(path, names, 16);

	for (int i = 0; i < count; ++i) {
		char full[128];
		full[0] = '\0';
		kstrcpy(full, path, sizeof(full));
		if (!kstreq(path, "/"))
			kstrcat(full, "/", sizeof(full));
		kstrcat(full, names[i], sizeof(full));

		console_print(prefix);
		if (is_last)
			console_print("└── ");
		else
			console_print("├── ");

		if (vfs_is_dir(full))
			console_print_color(names[i], VGA_ATTR(VGA_CYAN, VGA_BLACK));
		else
			console_print_color(names[i], VGA_ATTR(VGA_WHITE, VGA_BLACK));
		console_print("\n");

		if (vfs_is_dir(full)) {
			char new_prefix[128];
			kstrcpy(new_prefix, prefix, sizeof(new_prefix));
			if (is_last)
				kstrcat(new_prefix, "    ", sizeof(new_prefix));
			else
				kstrcat(new_prefix, "│   ", sizeof(new_prefix));
			tree_print(full, new_prefix, i == count - 1, depth + 1);
		}
	}
}

