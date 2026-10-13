#include "options.h"
#include "util.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void options_usage(const char *program)
{
    fprintf(stderr, "usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n", program);
}

/* Accept positive byte counts or binary K/M/G suffixes. A bare suffix means
 * one unit; FreeBSD login environments commonly set BLOCKSIZE=K. */
static uintmax_t environment_block_size(void)
{
    const char *text = getenv("BLOCKSIZE");
    const char *end;
    uintmax_t value;
    unsigned int exponent = 0;

    if (text == NULL || *text == '\0')
        return 512;
    if (*text >= '0' && *text <= '9') {
        char *number_end;
        errno = 0;
        value = strtoumax(text, &number_end, 10);
        if (errno == ERANGE || value == 0)
            goto invalid;
        end = number_end;
    } else {
        value = 1;
        end = text;
    }
    switch (*end) {
    case 'k': case 'K': exponent = 1; break;
    case 'm': case 'M': exponent = 2; break;
    case 'g': case 'G': exponent = 3; break;
    case '\0': break;
    default: goto invalid;
    }
    if (exponent != 0) {
        ++end;
        if (*end == 'b' || *end == 'B')
            ++end;
    }
    if (*end != '\0')
        goto invalid;
    while (exponent-- != 0) {
        if (value > UINTMAX_MAX / 1024)
            goto invalid;
        value *= 1024;
    }
    return value;

invalid:
    fprintf(stderr, "%s: invalid BLOCKSIZE '%s'; using 512 bytes\n",
            program_name, text);
    return 512;
}

int options_parse(int argc, char **argv, Options *options)
{
    int index;
    memset(options, 0, sizeof(*options));
    options->terminal = isatty(STDOUT_FILENO) != 0;
    options->quote = options->terminal;
    options->almost_all = geteuid() == 0;
    options->time_field = TIME_MODIFIED;
    options->sort_key = SORT_NAME;
    options->size_style = SIZE_BLOCKS;

    /* Parse left to right so each documented overriding pair is explicit.
     * POSIX-style parsing stops at the first operand; -- ends the options. */
    for (index = 1; index < argc; ++index) {
        const char *flag = argv[index];
        if (flag[0] != '-' || flag[1] == '\0')
            break;
        if (strcmp(flag, "--") == 0) {
            ++index;
            break;
        }
        for (++flag; *flag; ++flag) {
            switch (*flag) {
            case 'A': options->almost_all = true; break;
            case 'a': options->all = true; break;
            case 'c': options->time_field = TIME_CHANGED; break;
            case 'd': options->directory = true; options->recursive = false; break;
            case 'F': options->classify = true; break;
            case 'f': options->unsorted = true; break;
            case 'h': options->size_style = SIZE_HUMAN; break;
            case 'i': options->inode = true; break;
            case 'k': options->size_style = SIZE_KILOBYTES; break;
            case 'l': options->long_format = true; options->numeric_ids = false; break;
            case 'n': options->long_format = true; options->numeric_ids = true; break;
            case 'q': options->quote = true; break;
            case 'R': options->recursive = true; options->directory = false; break;
            case 'r': options->reverse = true; break;
            case 'S': options->sort_key = SORT_SIZE; break;
            case 's': options->blocks = true; break;
            case 't': options->sort_key = SORT_TIME; break;
            case 'u': options->time_field = TIME_ACCESSED; break;
            case 'w': options->quote = false; break;
            default:
                fprintf(stderr, "%s: unknown option -- %c\n", program_name, *flag);
                return -1;
            }
        }
    }
    options->block_size = options->size_style == SIZE_KILOBYTES ? 1024 : 512;
    if (options->size_style == SIZE_BLOCKS)
        options->block_size = environment_block_size();
    return index;
}
