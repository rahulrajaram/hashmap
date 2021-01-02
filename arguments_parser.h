#ifndef _ARGUMENTS_PARSER_H
#define _ARGUMENTS_PARSER_H

#include <argp.h>

struct arguments {
    int max_items;
    int max_slots;
    int verbose;
    int debug;
};

struct arguments parse_arguments(int argc, char *argv[]);
#endif
