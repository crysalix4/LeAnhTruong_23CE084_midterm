#ifndef LS_UTIL_H
#define LS_UTIL_H

#include <stddef.h>

extern const char *program_name;
void *xmalloc(size_t size);
void *xreallocarray(void *ptr, size_t count, size_t size);
char *xstrdup(const char *text);
char *path_join(const char *directory, const char *name);
void report_errno(const char *path, int error);

#endif
