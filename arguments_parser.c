/*
 * See: http://www.gnu.org/software/libc/manual/html_node/Argp-Example-3.html
 */
#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>

#include "arguments_parser.h"


const char *argp_program_version = "Hashmap 0.0.1";
const char *argp_program_bug_address = "<rahulrajaram2005@gmail.com>";
static char doc[] = "A generic hashmap implementation in C.";
static char args_doc[] = "[FILENAME]...";
static struct argp_option options[] = {
    {
        "max-items", 'i',
        "COUNT", 0,
        "Max number of key-value pairs in the hashmapl. Defaults to 100000"
    },
    {
        "max-slots", 's',
        "COUNT", 0,
        "Max number of key-value pairs that resolve to a slot. Defaults to 10"
    },
    {
        0
    }
};



static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'i':
        arguments->max_items = arg ? atoi(arg) : 100000;
        break;
    case 's':
        arguments->max_slots = arg ? atoi(arg) : 10;
        break;
    case ARGP_KEY_ARG:
        return 0;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

struct arguments parse_arguments(int argc, char *argv[])
{
    struct arguments arguments;

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    printf(
        "%d\n%d\n",
            arguments.max_items,
            arguments.max_slots
    );

    return arguments;
}
