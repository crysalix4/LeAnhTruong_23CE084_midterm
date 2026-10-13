#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *program_name = "myls";

static void allocation_failure(void)
{
    fprintf(stderr, "%s: out of memory or allocation too large\n", program_name);
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t size)
{
    void *ptr = malloc(size ? size : 1);
    if (ptr == NULL)
        allocation_failure();
    return ptr;
}

void *xreallocarray(void *ptr, size_t count, size_t size)
{
    void *result;
    if (size != 0 && count > SIZE_MAX / size)
        allocation_failure();
    result = realloc(ptr, count && size ? count * size : 1);
    if (result == NULL)
        allocation_failure();
    return result;
}

char *xstrdup(const char *text)
{
    size_t length = strlen(text);
    char *copy = xmalloc(length + 1);
    memcpy(copy, text, length + 1);
    return copy;
}

char *path_join(const char *directory, const char *name)
{
    size_t left = strlen(directory);
    size_t right = strlen(name);
    size_t slash = left != 0 && directory[left - 1] != '/';
    char *path;

    if (left > SIZE_MAX - slash - 1 || right > SIZE_MAX - left - slash - 1)
        allocation_failure();
    path = xmalloc(left + slash + right + 1);
    memcpy(path, directory, left);
    if (slash)
        path[left++] = '/';
    memcpy(path + left, name, right + 1);
    return path;
}

void report_errno(const char *path, int error)
{
    fprintf(stderr, "%s: %s: %s\n", program_name, path, strerror(error));
}
