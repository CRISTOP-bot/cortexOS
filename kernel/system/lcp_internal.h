#ifndef LCP_INTERNAL_H
#define LCP_INTERNAL_H
#include "lcp.h"
#include <stddef.h>
#include <stdbool.h>
#define MAX_PACKAGES 32
#define MAX_DEPENDENCIES 8
#define MAX_FILES 8
struct lcp_package {
 const char *name; const char *version; const char *description;
 const char *arch; const char *maintainer; const char *license;
 const char *repo; const char *package_path;
 const char *dependencies[MAX_DEPENDENCIES]; size_t dependency_count;
 const char *files[MAX_FILES]; size_t file_count; unsigned int size; bool installed;
};
typedef struct lcp_package lcp_package_t;
extern lcp_package_t packages[MAX_PACKAGES];
extern size_t package_count;
const char *lcp_trim(const char *s);
lcp_package_t *lcp_find_package(const char *name);
void lcp_print_help(void);
void lcp_print_package_info(const lcp_package_t *pkg);
void lcp_search(const char *term);
void lcp_list_available(void);
void lcp_list_installed(void);
bool lcp_install_package(lcp_package_t *pkg, bool no_deps, int *chain, size_t chain_len);
void lcp_remove_package(lcp_package_t *pkg, bool purge);
void lcp_upgrade_package(lcp_package_t *pkg);
void lcp_show_files(const lcp_package_t *pkg);
void lcp_show_dependencies(const lcp_package_t *pkg);
void lcp_verify_package(const lcp_package_t *pkg);
#endif
