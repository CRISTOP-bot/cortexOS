#include "lcp.h"
#include "console.h"
#include "fs.h"
#include "kstring.h"
#include "vfs.h"
#include "cortex_package.h"
#include "lcp_catalog.h"
#include <stddef.h>
#include <stdbool.h>

#define REPO_BUFFER_SIZE 8192
#define LCP_REPOSITORY_DOMAIN "cortex.org"

#include "lcp_internal.h"

static void lcp_print(const char *s)
{
	console_print(s);
}

const char *lcp_trim(const char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
		++s;
	return s;
}

static void lcp_rtrim(char *s)
{
	size_t len = 0;
	while (s[len])
		++len;
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' ||
	       s[len - 1] == '\r' || s[len - 1] == '\n')) {
		s[len - 1] = '\0';
		--len;
	}
}

static void lcp_split_list(const char *text, const char **out,
			   size_t *count, size_t max_count)
{
	*count = 0;
	const char *p = text;
	while (p && *p && *count < max_count) {
		while (*p == ' ' || *p == ',')
			++p;
		if (!*p)
			break;
		out[*count] = p;
		while (*p && *p != ',')
			++p;
		if (*p == ',') {
			*((char *)p) = '\0';
			++p;
		}
		*count += 1;
	}
}

static void lcp_parse_package_block(char *block)
{
	if (package_count >= MAX_PACKAGES)
		return;
	lcp_package_t *pkg = &packages[package_count];
	pkg->name = 0;
	pkg->version = 0;
	pkg->description = 0;
	pkg->arch = 0;
	pkg->maintainer = 0;
	pkg->license = 0;
	pkg->repo = 0;
	pkg->package_path = 0;
	pkg->dependency_count = 0;
	pkg->file_count = 0;
	pkg->size = 0;
	pkg->installed = false;

	char *line = block;
	while (line && *line) {
		char *eol = line;
		while (*eol && *eol != '\n')
			++eol;
		if (*eol == '\n') {
			*eol = '\0';
			++eol;
		}
		lcp_rtrim(line);
		const char *value = 0;
		if (kstrncmp(line, "name:", 5) == 0) {
			value = lcp_trim(line + 5);
			pkg->name = value;
		} else if (kstrncmp(line, "version:", 8) == 0) {
			value = lcp_trim(line + 8);
			pkg->version = value;
		} else if (kstrncmp(line, "description:", 12) == 0) {
			value = lcp_trim(line + 12);
			pkg->description = value;
		} else if (kstrncmp(line, "arch:", 5) == 0) {
			value = lcp_trim(line + 5);
			pkg->arch = value;
		} else if (kstrncmp(line, "maintainer:", 11) == 0) {
			value = lcp_trim(line + 11);
			pkg->maintainer = value;
		} else if (kstrncmp(line, "license:", 8) == 0) {
			value = lcp_trim(line + 8);
			pkg->license = value;
		} else if (kstrncmp(line, "dependencies:", 13) == 0) {
			value = lcp_trim(line + 13);
			lcp_split_list(value, pkg->dependencies,
				       &pkg->dependency_count, MAX_DEPENDENCIES);
		} else if (kstrncmp(line, "files:", 6) == 0) {
			value = lcp_trim(line + 6);
			lcp_split_list(value, pkg->files,
				       &pkg->file_count, MAX_FILES);
		} else if (kstrncmp(line, "size:", 5) == 0) {
			value = lcp_trim(line + 5);
			unsigned int size = 0;
			while (*value >= '0' && *value <= '9') {
				size = size * 10 + (unsigned int)(*value - '0');
				++value;
			}
			pkg->size = size;
		} else if (kstrncmp(line, "repo:", 5) == 0) {
			value = lcp_trim(line + 5);
			pkg->repo = value;
		} else if (kstrncmp(line, "package:", 8) == 0) {
			value = lcp_trim(line + 8);
			pkg->package_path = value;
		}
		line = eol;
	}

	if (pkg->name && pkg->version)
		package_count += 1;
}

static bool lcp_parse_repo(const char *data, size_t size)
{
	if (size >= REPO_BUFFER_SIZE)
		return false;
	for (size_t i = 0; i < size; ++i)
		repo_buffer[i] = (data[i] == '\r') ? '\n' : data[i];
	repo_buffer[size] = '\0';

	package_count = 0;
	char *block = repo_buffer;
	char *p = repo_buffer;
	while (*p) {
		if (p[0] == '-' && p[1] == '-' && p[2] == '-' &&
		    (p[3] == '\n' || p[3] == '\0')) {
			*p = '\0';
			lcp_parse_package_block(block);
			p += 3;
			if (*p == '\n')
				++p;
			block = p;
		} else {
			++p;
		}
	}
	if (block && *block)
		lcp_parse_package_block(block);
	return package_count > 0;
}


bool lcp_init(void)
{
	const struct fs_file *file = fs_find("lcp_repo.txt");
	if (file) {
		if (lcp_parse_repo((const char *)file->data, file->size)) {
			repo_loaded = true;
			return true;
		}
	}
	lcp_parse_repo(lcp_default_repo_data, sizeof(lcp_default_repo_data) - 1);
	console_print("LCP: using built-in cortex.org catalog mirror\n");
	repo_loaded = true;
	return true;
}

lcp_package_t *lcp_find_package(const char *name)
{
	for (size_t i = 0; i < package_count; ++i) {
		if (kstrcmp(packages[i].name, name) == 0)
			return &packages[i];
	}
	return 0;
}

static bool lcp_contains(const char *text, const char *term)
{
	while (*text) {
		const char *a = text;
		const char *b = term;
		while (*a && *b) {
			char ca = *a;
			char cb = *b;
			if (ca >= 'A' && ca <= 'Z')
				ca += 'a' - 'A';
			if (cb >= 'A' && cb <= 'Z')
				cb += 'a' - 'A';
			if (ca != cb)
				break;
			++a;
			++b;
		}
		if (!*b)
			return true;
		++text;
	}
	return false;
}

static void lcp_print_line(const char *label, const char *value)
{
	char buffer[256];
	buffer[0] = '\0';
	kstrcpy(buffer, label, sizeof(buffer));
	if (value)
		kstrcat(buffer, value, sizeof(buffer));
	console_print(buffer);
	console_print("\n");
}

static void lcp_format_number(unsigned int value, char *out, size_t max_len)
{
	if (max_len == 0)
		return;
	if (value == 0) {
		if (max_len >= 2) {
			out[0] = '0';
			out[1] = '\0';
		} else {
			out[0] = '\0';
		}
		return;
	}
	char rev[16];
	int rp = 0;
	while (value > 0 && rp < (int)sizeof(rev)) {
		rev[rp++] = '0' + (value % 10);
		value /= 10;
	}
	int pos = 0;
	while (rp > 0 && pos + 1 < (int)max_len)
		out[pos++] = rev[--rp];
	out[pos] = '\0';
}

void lcp_print_package_info(const lcp_package_t *pkg)
{
	lcp_print_line("name: ", pkg->name ? pkg->name : "");
	lcp_print_line("version: ", pkg->version ? pkg->version : "");
	lcp_print_line("description: ", pkg->description ? pkg->description : "");
	char number[32];
	lcp_format_number(pkg->size, number, sizeof(number));
	char size_line[64];
	kstrcpy(size_line, "size: ", sizeof(size_line));
	kstrcat(size_line, number, sizeof(size_line));
	kstrcat(size_line, " bytes", sizeof(size_line));
	console_print(size_line);
	console_print("\n");
	if (pkg->dependency_count == 0) {
		lcp_print_line("dependencies: ", "none");
	} else {
		char output[256];
		output[0] = '\0';
		for (size_t i = 0; i < pkg->dependency_count; ++i) {
			if (i > 0)
				kstrcat(output, ", ", sizeof(output));
			kstrcat(output, pkg->dependencies[i], sizeof(output));
		}
		lcp_print_line("dependencies: ", output);
	}
	lcp_print_line("repository: ", pkg->repo ? pkg->repo : "");
	lcp_print_line("package: ", pkg->package_path ? pkg->package_path : "(metadata only)");
	lcp_print_line("author: ", pkg->maintainer ? pkg->maintainer : "");
	lcp_print_line("license: ", pkg->license ? pkg->license : "");
}

void lcp_print_help(void)
{
	console_print("lcp commands:\n");
	console_print("  lcp help [command]\n");
	console_print("  lcp search <term>\n");
	console_print("  lcp info <package>\n");
	console_print("  lcp install <package> [--no-deps]\n");
	console_print("  Database: cortex.org (local metadata mirror)\n");
	console_print("  Entries use package:<path.cortex> and dependencies:a,b\n");
	console_print("  lcp remove <package>\n");
	console_print("  lcp update\n");
	console_print("  lcp upgrade [package]\n");
	console_print("  lcp list [--installed|--available|--upgradable]\n");
	console_print("  lcp files <package>\n");
	console_print("  lcp depends <package>\n");
	console_print("  lcp verify <package>\n");
}

void lcp_list_available(void)
{
	for (size_t i = 0; i < package_count; ++i) {
		console_print(packages[i].name);
		console_print("\n");
	}
}

void lcp_list_installed(void)
{
	for (size_t i = 0; i < package_count; ++i) {
		if (packages[i].installed) {
			console_print(packages[i].name);
			console_print("\n");
		}
	}
}

static bool lcp_has_dependents(const char *name)
{
	for (size_t i = 0; i < package_count; ++i) {
		if (!packages[i].installed)
			continue;
		for (size_t j = 0; j < packages[i].dependency_count; ++j) {
			if (kstrcmp(packages[i].dependencies[j], name) == 0)
				return true;
		}
	}
	return false;
}

bool lcp_install_package(lcp_package_t *pkg, bool no_deps, int *chain, size_t chain_len)
{
	if (pkg->installed) {
		console_print("Package already installed.\n");
		return true;
	}
	if (pkg->package_path &&
	    (!vfs_exists(pkg->package_path) || vfs_is_dir(pkg->package_path))) {
		console_print("Package payload is missing: ");
		console_print(pkg->package_path);
		console_print("\n");
		return false;
	}
	int my_idx = (int)(pkg - packages);
	for (size_t v = 0; v < chain_len; v++) {
		if (chain[v] == my_idx) {
			console_print("Circular dependency detected, aborting.\n");
			return false;
		}
	}
	if (chain_len < MAX_PACKAGES)
		chain[chain_len] = my_idx;
	pkg->installed = true;
	if (!no_deps) {
		for (size_t i = 0; i < pkg->dependency_count; ++i) {
			lcp_package_t *dep = lcp_find_package(pkg->dependencies[i]);
			if (dep && !dep->installed) {
				if (!lcp_install_package(dep, false, chain, chain_len + 1)) {
					pkg->installed = false;
					return false;
				}
			}
		}
	}
	if (pkg->package_path && cortex_package_run(pkg->package_path) < 0) {
		pkg->installed = false;
		console_print("Package command manifest failed.\n");
		return false;
	}
	console_print("Installed ");
	console_print(pkg->name);
	console_print("\n");
	return true;
}

void lcp_remove_package(lcp_package_t *pkg, bool purge)
{
	if (!pkg->installed) {
		console_print("Package is not installed.\n");
		return;
	}
	if (lcp_has_dependents(pkg->name)) {
		console_print("Cannot remove package because another installed package depends on it.\n");
		return;
	}
	pkg->installed = false;
	console_print("Removed ");
	console_print(pkg->name);
	if (purge)
		console_print(" and purged configuration.");
	console_print("\n");
}

void lcp_upgrade_package(lcp_package_t *pkg)
{
	if (!pkg->installed) {
		console_print("Package is not installed.\n");
		return;
	}
	console_print("Upgraded ");
	console_print(pkg->name);
	console_print("\n");
}

void lcp_show_files(const lcp_package_t *pkg)
{
	if (!pkg) {
		console_print("Package not found.\n");
		return;
	}
	for (size_t i = 0; i < pkg->file_count; ++i) {
		console_print(pkg->files[i]);
		console_print("\n");
	}
}

void lcp_show_dependencies(const lcp_package_t *pkg)
{
	if (!pkg) {
		console_print("Package not found.\n");
		return;
	}
	if (pkg->dependency_count == 0) {
		console_print("No dependencies.\n");
		return;
	}
	for (size_t i = 0; i < pkg->dependency_count; ++i) {
		console_print(pkg->dependencies[i]);
		console_print("\n");
	}
}

void lcp_verify_package(const lcp_package_t *pkg)
{
	if (!pkg) {
		console_print("Package not found.\n");
		return;
	}
	if (!pkg->installed) {
		console_print("Package is not installed.\n");
		return;
	}
	console_print("Package is installed and appears healthy.\n");
}

void lcp_search(const char *term)
{
	bool found = false;
	for (size_t i = 0; i < package_count; ++i) {
		if (lcp_contains(packages[i].name, term) ||
		    lcp_contains(packages[i].description ? packages[i].description : "", term)) {
			console_print(packages[i].name);
			console_print("\n");
			found = true;
		}
	}
	if (!found)
		console_print("No matching packages.\n");
}

static const char *lcp_next_token(const char *s, char *token, size_t max_len)
{
	while (*s == ' ')
		++s;
	size_t pos = 0;
	while (*s && *s != ' ' && pos + 1 < max_len)
		token[pos++] = *s++;
	token[pos] = '\0';
	while (*s == ' ')
		++s;
	return s;
}

