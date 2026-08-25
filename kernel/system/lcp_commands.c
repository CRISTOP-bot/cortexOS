#include "lcp_internal.h"
#include "console.h"
#include "kstring.h"

int lcp_handle_command(const char *args)
{
	char token[64];
	const char *rest = lcp_next_token(args, token, sizeof(token));
	if (token[0] == '\0' || kstrcmp(token, "help") == 0) {
		lcp_print_help();
		return 0;
	}
	if (kstrcmp(token, "search") == 0) {
		if (*rest == '\0') {
			lcp_print("search requires a term.\n");
			return 0;
		}
		lcp_search(rest);
		return 0;
	}
	if (kstrcmp(token, "info") == 0) {
		if (*rest == '\0') {
			lcp_print("info requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(rest);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_print_package_info(pkg);
		return 0;
	}
	if (kstrcmp(token, "list") == 0) {
		if (kstrcmp(rest, "--available") == 0) {
			lcp_list_available();
			return 0;
		}
		if (kstrcmp(rest, "--installed") == 0) {
			lcp_list_installed();
			return 0;
		}
		lcp_list_installed();
		return 0;
	}
	if (kstrcmp(token, "install") == 0) {
		bool no_deps = false;
		const char *name = rest;
		if (kstrncmp(rest, "--no-deps", 9) == 0) {
			no_deps = true;
			name = lcp_trim(rest + 9);
		}
		if (*name == '\0') {
			lcp_print("install requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(name);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		if (pkg->installed) {
			lcp_print("Package already installed.\n");
			return 0;
		}
		int dep_chain[MAX_PACKAGES];
		lcp_install_package(pkg, no_deps, dep_chain, 0);
		return 0;
	}
	if (kstrcmp(token, "remove") == 0) {
		bool purge = false;
		const char *name = rest;
		if (kstrncmp(rest, "--purge", 7) == 0) {
			purge = true;
			name = lcp_trim(rest + 7);
		}
		if (*name == '\0') {
			lcp_print("remove requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(name);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_remove_package(pkg, purge);
		return 0;
	}
	if (kstrcmp(token, "update") == 0) {
		if (lcp_init())
			lcp_print("Repository metadata updated.\n");
		else
			lcp_print("Failed to update repository metadata.\n");
		return 0;
	}
	if (kstrcmp(token, "upgrade") == 0) {
		if (*rest == '\0') {
			for (size_t i = 0; i < package_count; ++i) {
				if (packages[i].installed)
					lcp_upgrade_package(&packages[i]);
			}
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(rest);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_upgrade_package(pkg);
		return 0;
	}
	if (kstrcmp(token, "files") == 0) {
		if (*rest == '\0') {
			lcp_print("files requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(rest);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_show_files(pkg);
		return 0;
	}
	if (kstrcmp(token, "depends") == 0) {
		if (*rest == '\0') {
			lcp_print("depends requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(rest);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_show_dependencies(pkg);
		return 0;
	}
	if (kstrcmp(token, "verify") == 0) {
		if (*rest == '\0') {
			lcp_print("verify requires a package name.\n");
			return 0;
		}
		lcp_package_t *pkg = lcp_find_package(rest);
		if (!pkg) {
			lcp_print("Package not found.\n");
			return 0;
		}
		lcp_verify_package(pkg);
		return 0;
	}
	lcp_print("Unknown lcp command. Type 'lcp help' for commands.\n");
	return 0;
}
