#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* ABI smoke test for the interfaces required by OpenRC, Fastfetch and Bash.
 * It is intentionally small and only proves that the user headers/libc link
 * against the NucleOS syscall ABI. Runtime execution still needs the kernel
 * process and address-space work documented in docs/USERSPACE_PORT.md. */
int main(void)
{
	struct stat st;
	int fd;
	int status = 0;

	if (stat("/etc/os-release", &st) < 0)
		return 1;
	fd = open("/etc/os-release", O_RDONLY);
	if (fd < 0)
		return 2;
	if (fcntl(fd, F_GETFL) < 0)
		return 3;
	close(fd);
	(void)waitpid(-1, &status, WNOHANG);
	return S_ISREG(st.st_mode) ? 0 : 4;
}
