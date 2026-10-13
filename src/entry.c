#include "entry.h"
#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void entries_append(EntryList *list, const char *name, const char *path,
                    const struct stat *metadata)
{
    Entry *entry;
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity : 16;
        if (capacity > SIZE_MAX / 2) {
            /* Route an impossible allocation through the checked allocator. */
            (void)xreallocarray(NULL, SIZE_MAX, 2);
        }
        capacity *= 2;
        list->items = xreallocarray(list->items, capacity, sizeof(*list->items));
        list->capacity = capacity;
    }
    entry = &list->items[list->count++];
    entry->name = xstrdup(name);
    entry->path = xstrdup(path);
    entry->metadata = *metadata;
}

void entries_free(EntryList *list)
{
    size_t index;
    for (index = 0; index < list->count; ++index) {
        free(list->items[index].name);
        free(list->items[index].path);
    }
    free(list->items);
    memset(list, 0, sizeof(*list));
}

struct timespec entry_time(const Entry *entry, const Options *options)
{
#if defined(__APPLE__)
    if (options->time_field == TIME_ACCESSED)
        return entry->metadata.st_atimespec;
    if (options->time_field == TIME_CHANGED)
        return entry->metadata.st_ctimespec;
    return entry->metadata.st_mtimespec;
#else
    if (options->time_field == TIME_ACCESSED)
        return entry->metadata.st_atim;
    if (options->time_field == TIME_CHANGED)
        return entry->metadata.st_ctim;
    return entry->metadata.st_mtim;
#endif
}

/* qsort has no portable context parameter. This single-threaded program sets
 * the read-only context immediately before each call. */
static const Options *sort_options;

static int compare_entries(const void *first, const void *second)
{
    const Entry *left = first;
    const Entry *right = second;
    int result = 0;

    if (sort_options->sort_key == SORT_SIZE) {
        result = (left->metadata.st_size < right->metadata.st_size) -
                 (left->metadata.st_size > right->metadata.st_size);
    } else if (sort_options->sort_key == SORT_TIME) {
        struct timespec a = entry_time(left, sort_options);
        struct timespec b = entry_time(right, sort_options);
        result = (a.tv_sec < b.tv_sec) - (a.tv_sec > b.tv_sec);
        if (result == 0)
            result = (a.tv_nsec < b.tv_nsec) - (a.tv_nsec > b.tv_nsec);
    }
    if (result == 0) {
        int names = strcmp(left->name, right->name);
        result = (names > 0) - (names < 0);
    }
    return sort_options->reverse ? -result : result;
}

void entries_sort(EntryList *list, const Options *options)
{
    if (options->unsorted || list->count < 2)
        return;
    sort_options = options;
    qsort(list->items, list->count, sizeof(*list->items), compare_entries);
}
