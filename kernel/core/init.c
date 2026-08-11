#include "init.h"
#include "console.h"
#include "process.h"
#include "vfs.h"
#include "kstring.h"

bool init_start_openrc(void)
{
	static const char *argv[] = { "/sbin/openrc-init", 0 };
	static const char *envp[] = {
		"PATH=/sbin:/bin:/usr/sbin:/usr/bin",
		"HOME=/root",
		0
	};
	int pid;

	/* Do not silently execute the in-kernel compatibility manager.  The real
	 * handoff only happens once the cross-built OpenRC ELF is installed. */
	if (!vfs_exists("/sbin/openrc-init"))
		return false;
	pid = process_spawn_exec("openrc-init", "/sbin/openrc-init", argv, envp);
	if (pid < 0) {
		console_print_color("[FAILED] Could not load /sbin/openrc-init\n",
					VGA_ATTR(VGA_RED, VGA_BLACK));
		return false;
	}
	console_print("[  OK  ] Loaded real OpenRC init as PID ");
	char pid_text[16];
	kitoa(pid, pid_text, sizeof(pid_text));
	console_print(pid_text);
	console_print("\n");
	return true;
}
