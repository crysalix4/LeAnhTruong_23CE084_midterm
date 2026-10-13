#include "format.h"
#include "util.h"

#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <limits.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#if defined(__linux__)
#include <sys/sysmacros.h>
#endif

/* Filenames need not be valid text. Consume invalid sequences one byte at a
 * time, but keep each printable multibyte character intact in the -q output. */
void format_name(const char *name, const Options *options)
{
    mbstate_t state = {0};
    const char *cursor = name;
    size_t remaining = strlen(name);

    if (!options->quote) {
        fputs(name, stdout);
        return;
    }
    while (remaining != 0) {
        wchar_t character;
        size_t length = mbrtowc(&character, cursor, remaining, &state);

        if (length == (size_t)-1 || length == (size_t)-2 || length == 0) {
            putchar('?');
            memset(&state, 0, sizeof(state));
            length = 1;
        } else if (iswprint((wint_t)character)) {
            fwrite(cursor, 1, length, stdout);
        } else {
            putchar('?');
        }
        cursor += length;
        remaining -= length;
    }
}

static char file_type(mode_t mode)
{
    if (S_ISREG(mode)) return '-';
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
#if defined(S_ISWHT)
    if (S_ISWHT(mode)) return 'w';
#elif defined(S_IFWHT) && defined(S_IFMT)
    if ((mode & S_IFMT) == S_IFWHT) return 'w';
#endif
    /* These legacy types are available only on systems defining them. */
#if defined(S_ISARCH1)
    if (S_ISARCH1(mode)) return 'a';
#elif defined(S_IFARCH1) && defined(S_IFMT)
    if ((mode & S_IFMT) == S_IFARCH1) return 'a';
#endif
#if defined(S_ISARCH2)
    if (S_ISARCH2(mode)) return 'A';
#elif defined(S_IFARCH2) && defined(S_IFMT)
    if ((mode & S_IFMT) == S_IFARCH2) return 'A';
#endif
    return '?';
}

static void permission_text(mode_t mode, char text[11])
{
    static const mode_t bits[] = {
        S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
        S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH
    };
    static const char symbols[] = "rwxrwxrwx";
    size_t i;

    text[0] = file_type(mode);
    for (i = 0; i < 9; ++i)
        text[i + 1] = (mode & bits[i]) ? symbols[i] : '-';
    if (mode & S_ISUID)
        text[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        text[6] = (mode & S_IXGRP) ? 's' : 'S';
#ifdef S_ISVTX
    if (mode & S_ISVTX)
        text[9] = (mode & S_IXOTH) ? 't' : 'T';
#endif
    text[10] = '\0';
}

static char classification(mode_t mode)
{
    switch (file_type(mode)) {
    case 'd': return '/';
    case 'l': return '@';
    case 'w': return '%';
    case 's': return '=';
    case 'p': return '|';
    default:
        return (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? '*' : '\0';
    }
}

static uintmax_t allocated_blocks(const struct stat *metadata)
{
    /* st_blocks counts allocated 512-byte units, including for sparse files. */
    return metadata->st_blocks > 0 ? (uintmax_t)metadata->st_blocks : 0;
}

/* Compute ceil(blocks * 512 / unit) without forming an overflowing product.
 * Each doubling carries the remainder into the quotient. */
static int convert_blocks(uintmax_t blocks, uintmax_t unit, uintmax_t *result)
{
    uintmax_t quotient;
    uintmax_t remainder;
    unsigned int i;

    if (unit == 0)
        return 1;
    quotient = blocks / unit;
    remainder = blocks % unit;
    for (i = 0; i < 9; ++i) {
        if (quotient > UINTMAX_MAX / 2)
            return 1;
        quotient *= 2;
        if (remainder >= unit - remainder) {
            remainder -= unit - remainder;
            if (quotient == UINTMAX_MAX)
                return 1;
            ++quotient;
        } else {
            remainder *= 2;
        }
    }
    if (remainder != 0) {
        if (quotient == UINTMAX_MAX)
            return 1;
        ++quotient;
    }
    *result = quotient;
    return 0;
}

static void human_text(long double bytes, char *text, size_t capacity)
{
    static const char units[] = "BKMGTPEZY";
    long double magnitude = bytes < 0 ? -bytes : bytes;
    size_t index = 0;

    while (magnitude >= 1024 && index + 1 < sizeof(units) - 1) {
        magnitude /= 1024;
        ++index;
    }
    /* Prevent rounding from producing a misleading 1024K rather than 1.0M. */
    if (magnitude >= 1023.5L && index != 0 &&
        index + 1 < sizeof(units) - 1) {
        magnitude /= 1024;
        ++index;
    }
    if (bytes < 0)
        magnitude = -magnitude;
    if (index != 0 && magnitude > -10 && magnitude < 10)
        snprintf(text, capacity, "%.1Lf%c", magnitude, units[index]);
    else
        snprintf(text, capacity, "%.0Lf%c", magnitude, units[index]);
}

static int print_blocks(uintmax_t blocks, const Options *options,
                        const char *path)
{
    uintmax_t value;
    uintmax_t unit = options->size_style == SIZE_KILOBYTES
                   ? 1024 : options->block_size;

    if (options->size_style == SIZE_HUMAN) {
        char text[64];
        human_text((long double)blocks * 512, text, sizeof(text));
        fputs(text, stdout);
        return 0;
    }
    if (convert_blocks(blocks, unit, &value)) {
        report_errno(path, EOVERFLOW);
        putchar('?');
        return 1;
    }
    printf("%" PRIuMAX, value);
    return 0;
}

static int print_total(const EntryList *list, const Options *options)
{
    uintmax_t blocks = 0;
    long double human_bytes = 0;
    bool overflow = false;
    size_t i;

    for (i = 0; i < list->count; ++i) {
        uintmax_t count = allocated_blocks(&list->items[i].metadata);

        human_bytes += (long double)count * 512;
        if (count > UINTMAX_MAX - blocks)
            overflow = true;
        else
            blocks += count;
    }
    fputs("total ", stdout);
    if (options->size_style == SIZE_HUMAN) {
        char text[64];
        human_text(human_bytes, text, sizeof(text));
        puts(text);
        return 0;
    }
    if (overflow) {
        report_errno("directory block total", EOVERFLOW);
        puts("?");
        return 1;
    }
    {
        int result = print_blocks(blocks, options, "directory block total");
        putchar('\n');
        return result;
    }
}

/* A link can change after lstat. Grow and retry whenever readlink fills the
 * buffer instead of assuming st_size or PATH_MAX is an adequate limit. */
static char *link_target(const char *path)
{
    size_t capacity = 128;
    char *target = xmalloc(capacity + 1);

    for (;;) {
        ssize_t length = readlink(path, target, capacity);

        if (length < 0) {
            int error = errno;
            free(target);
            report_errno(path, error);
            return NULL;
        }
        if ((size_t)length < capacity) {
            target[length] = '\0';
            return target;
        }
        if (capacity > (SIZE_MAX - 1) / 2 ||
            capacity > (size_t)SSIZE_MAX / 2) {
            free(target);
            report_errno(path, EOVERFLOW);
            return NULL;
        }
        capacity *= 2;
        target = xreallocarray(target, capacity + 1, sizeof(*target));
    }
}

static void print_identity(const struct stat *metadata, bool numeric)
{
    struct passwd *owner = numeric ? NULL : getpwuid(metadata->st_uid);
    struct group *group;

    if (owner != NULL)
        printf("%s ", owner->pw_name);
    else
        printf("%" PRIuMAX " ", (uintmax_t)metadata->st_uid);
    group = numeric ? NULL : getgrgid(metadata->st_gid);
    if (group != NULL)
        printf("%s ", group->gr_name);
    else
        printf("%" PRIuMAX " ", (uintmax_t)metadata->st_gid);
}

static int print_long_prefix(const Entry *entry, const Options *options)
{
    const struct stat *metadata = &entry->metadata;
    struct timespec selected_time = entry_time(entry, options);
    struct tm local_time;
    char mode[11];
    char date[128];
    int result = 0;

    permission_text(metadata->st_mode, mode);
    printf("%s %" PRIuMAX " ", mode, (uintmax_t)metadata->st_nlink);
    print_identity(metadata, options->numeric_ids);
    if (S_ISCHR(metadata->st_mode) || S_ISBLK(metadata->st_mode)) {
        printf("%" PRIuMAX ", %" PRIuMAX " ",
               (uintmax_t)major(metadata->st_rdev),
               (uintmax_t)minor(metadata->st_rdev));
    } else if (options->size_style == SIZE_HUMAN) {
        char size[64];
        human_text((long double)metadata->st_size, size, sizeof(size));
        printf("%s ", size);
    } else {
        printf("%" PRIdMAX " ", (intmax_t)metadata->st_size);
    }
    if (localtime_r(&selected_time.tv_sec, &local_time) == NULL ||
        strftime(date, sizeof(date), "%b %e %H:%M", &local_time) == 0) {
        report_errno(entry->path, EOVERFLOW);
        strcpy(date, "??? ?? ??:??");
        result = 1;
    }
    printf("%s ", date);
    return result;
}

int format_entries(const EntryList *list, const Options *options,
                   bool directory_contents)
{
    size_t i;
    int result = 0;

    if ((directory_contents && options->long_format) ||
        (options->blocks && options->terminal))
        result |= print_total(list, options);
    for (i = 0; i < list->count; ++i) {
        const Entry *entry = &list->items[i];
        char *target = NULL;

        if (options->long_format && S_ISLNK(entry->metadata.st_mode)) {
            target = link_target(entry->path);
            if (target == NULL)
                result = 1;
        }
        if (options->inode)
            printf("%" PRIuMAX " ", (uintmax_t)entry->metadata.st_ino);
        if (options->blocks) {
            result |= print_blocks(allocated_blocks(&entry->metadata),
                                   options, entry->path);
            putchar(' ');
        }
        if (options->long_format)
            result |= print_long_prefix(entry, options);
        format_name(entry->name, options);
        if (options->classify) {
            char suffix = classification(entry->metadata.st_mode);
            if (suffix != '\0')
                putchar(suffix);
        }
        if (target != NULL) {
            fputs(" -> ", stdout);
            format_name(target, options);
            free(target);
        }
        putchar('\n');
    }
    if (ferror(stdout)) {
        report_errno("standard output", errno != 0 ? errno : EIO);
        result = 1;
    }
    return result;
}
