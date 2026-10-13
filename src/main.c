#include "listing.h"
#include "options.h"
#include "util.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    Options options;
    int first_operand;
    int status;
    const char *slash;

    if (argc > 0 && argv[0] != NULL) {
        slash = strrchr(argv[0], '/');
        program_name = slash ? slash + 1 : argv[0];
    }
    /* Character classification and dates follow the user's locale; pathname
     * ordering remains deterministic, byte-wise lexicographical ordering. */
    (void)setlocale(LC_ALL, "");
    tzset();
    first_operand = options_parse(argc, argv, &options);
    if (first_operand < 0) {
        options_usage(program_name);
        return EXIT_FAILURE;
    }
    status = list_paths(argc - first_operand, argv + first_operand, &options);
    if (fflush(stdout) == EOF || ferror(stdout)) {
        report_errno("standard output", errno ? errno : EIO);
        status = 1;
    }
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
