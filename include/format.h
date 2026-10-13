#ifndef LS_FORMAT_H
#define LS_FORMAT_H

#include "entry.h"
#include "options.h"

/* Print one entry per line; return 1 on a metadata/readlink/output error. */
int format_entries(const EntryList *list, const Options *options,
                   bool directory_contents);
void format_name(const char *name, const Options *options);

#endif
