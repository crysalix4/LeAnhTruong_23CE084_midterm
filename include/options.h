#ifndef LS_OPTIONS_H
#define LS_OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { TIME_MODIFIED, TIME_ACCESSED, TIME_CHANGED } TimeField;
typedef enum { SORT_NAME, SORT_SIZE, SORT_TIME } SortKey;
typedef enum { SIZE_BLOCKS, SIZE_KILOBYTES, SIZE_HUMAN } SizeStyle;

typedef struct {
    bool almost_all;
    bool all;
    bool directory;
    bool classify;
    bool unsorted;
    bool inode;
    bool long_format;
    bool numeric_ids;
    bool quote;
    bool recursive;
    bool reverse;
    bool blocks;
    bool terminal;
    TimeField time_field;
    SortKey sort_key;
    SizeStyle size_style;
    uintmax_t block_size;
} Options;

/* Returns the index of the first operand, or -1 on an invalid option. */
int options_parse(int argc, char **argv, Options *options);
void options_usage(const char *program);

#endif
