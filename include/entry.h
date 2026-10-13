#ifndef LS_ENTRY_H
#define LS_ENTRY_H

#include "options.h"
#include <stddef.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    char *name;
    char *path;
    struct stat metadata;
} Entry;

typedef struct {
    Entry *items;
    size_t count;
    size_t capacity;
} EntryList;

void entries_append(EntryList *list, const char *name, const char *path,
                    const struct stat *metadata);
void entries_free(EntryList *list);
void entries_sort(EntryList *list, const Options *options);
struct timespec entry_time(const Entry *entry, const Options *options);

#endif
