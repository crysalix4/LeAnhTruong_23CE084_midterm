#include "listing.h"
#include "entry.h"
#include "format.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Frame {
    char *path;
    EntryList entries;
    size_t next_child;
    dev_t device;
    ino_t inode;
    struct Frame *parent;
} Frame;

typedef struct {
    int status;
    bool printed;
    const Options *options;
} Listing;

static bool dot_directory(const char *name)
{
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static bool visible(const char *name, const Options *options)
{
    if (name[0] != '.' || options->all)
        return true;
    return options->almost_all && !dot_directory(name);
}

static void listing_error(Listing *listing, const char *path, int error)
{
    report_errno(path, error);
    listing->status = 1;
}

/* Snapshot and close each directory before descending. Deep trees therefore
 * consume neither the C call stack nor one open descriptor per ancestor. */
static Frame *read_directory(const char *path, Frame *parent, Listing *listing)
{
    DIR *directory;
    struct dirent *item;
    struct stat metadata;
    Frame *frame;
    Frame *ancestor;
    int descriptor;
    int flags = O_RDONLY | O_DIRECTORY;
    const Options *options = listing->options;
    bool need_metadata = options->long_format || options->blocks ||
                         options->classify || options->recursive ||
                         (!options->unsorted && options->sort_key != SORT_NAME);

    /* Command-line directory symlinks are allowed under the default policy.
     * Recursive children must still be directories, even if replaced while
     * the listing runs. O_NOFOLLOW closes that symlink race. */
    if (parent != NULL)
        flags |= O_NOFOLLOW;
    descriptor = open(path, flags);
    if (descriptor < 0) {
        listing_error(listing, path, errno);
        return NULL;
    }
    if (fstat(descriptor, &metadata) < 0) {
        int error = errno;
        close(descriptor);
        listing_error(listing, path, error);
        return NULL;
    }
    for (ancestor = parent; ancestor != NULL; ancestor = ancestor->parent) {
        if (ancestor->device == metadata.st_dev && ancestor->inode == metadata.st_ino) {
            fprintf(stderr, "%s: %s: directory cycle detected\n", program_name, path);
            listing->status = 1;
            close(descriptor);
            return NULL;
        }
    }
    directory = fdopendir(descriptor);
    if (directory == NULL) {
        int error = errno;
        close(descriptor);
        listing_error(listing, path, error);
        return NULL;
    }
    frame = xmalloc(sizeof(*frame));
    memset(frame, 0, sizeof(*frame));
    frame->path = xstrdup(path);
    frame->device = metadata.st_dev;
    frame->inode = metadata.st_ino;
    frame->parent = parent;

    for (;;) {
        char *child_path;
        /* readdir's EOF and error both return NULL; reset errno each time. */
        errno = 0;
        item = readdir(directory);
        if (item == NULL) {
            if (errno != 0)
                listing_error(listing, path, errno);
            break;
        }
        if (!visible(item->d_name, listing->options))
            continue;
        child_path = path_join(path, item->d_name);
        /* Names and inode numbers are already available from readdir. A
         * readable directory need not grant search permission: plain listings
         * must still work there without requiring a stat of every child. */
        memset(&metadata, 0, sizeof(metadata));
        metadata.st_ino = item->d_ino;
        if (need_metadata &&
            fstatat(descriptor, item->d_name, &metadata, AT_SYMLINK_NOFOLLOW) < 0)
            listing_error(listing, child_path, errno);
        else
            entries_append(&frame->entries, item->d_name, child_path, &metadata);
        free(child_path);
    }
    if (closedir(directory) < 0)
        listing_error(listing, path, errno);
    entries_sort(&frame->entries, listing->options);
    return frame;
}

static void print_directory(Frame *frame, bool header, Listing *listing)
{
    if (header) {
        if (listing->printed)
            putchar('\n');
        format_name(frame->path, listing->options);
        puts(":");
        listing->printed = true;
    }
    listing->status |= format_entries(&frame->entries, listing->options, true);
    if (frame->entries.count || listing->options->long_format ||
        (listing->options->blocks && listing->options->terminal))
        listing->printed = true;
}

static void walk_directory(const char *path, bool header, Listing *listing)
{
    Frame *frame = read_directory(path, NULL, listing);
    if (frame == NULL)
        return;
    print_directory(frame, header, listing);

    while (frame != NULL) {
        Frame *child = NULL;
        if (listing->options->recursive) {
            while (frame->next_child < frame->entries.count) {
                const Entry *entry = &frame->entries.items[frame->next_child++];
                if (!S_ISDIR(entry->metadata.st_mode) || dot_directory(entry->name))
                    continue;
                child = read_directory(entry->path, frame, listing);
                if (child != NULL)
                    break;
            }
        }
        if (child != NULL) {
            frame = child;
            print_directory(frame, true, listing);
        } else {
            Frame *parent = frame->parent;
            entries_free(&frame->entries);
            free(frame->path);
            free(frame);
            frame = parent;
        }
    }
}

int list_paths(int count, char **paths, const Options *options)
{
    EntryList files = {0};
    EntryList directories = {0};
    Listing listing = {0, false, options};
    char *default_paths[] = {"."};
    int index;
    size_t directory_index;

    if (count == 0) {
        paths = default_paths;
        count = 1;
    }
    for (index = 0; index < count; ++index) {
        struct stat metadata;
        if (lstat(paths[index], &metadata) < 0) {
            listing_error(&listing, paths[index], errno);
            continue;
        }
        /* The PDF does not spell out the default link policy. Follow an
         * operand link to a directory for a plain listing. Preserve link
         * metadata for -d, -l/-n and -F, and all directory entries. */
        if (S_ISLNK(metadata.st_mode) && !options->directory &&
            !options->long_format && !options->classify) {
            struct stat target;
            if (stat(paths[index], &target) == 0 && S_ISDIR(target.st_mode))
                metadata = target;
        }
        if (!options->directory && S_ISDIR(metadata.st_mode))
            entries_append(&directories, paths[index], paths[index], &metadata);
        else
            entries_append(&files, paths[index], paths[index], &metadata);
    }
    entries_sort(&files, options);
    entries_sort(&directories, options);
    if (files.count != 0) {
        listing.status |= format_entries(&files, options, false);
        listing.printed = true;
    }
    for (directory_index = 0; directory_index < directories.count; ++directory_index)
        walk_directory(directories.items[directory_index].path,
                       count > 1 || options->recursive, &listing);
    entries_free(&files);
    entries_free(&directories);
    return listing.status;
}
