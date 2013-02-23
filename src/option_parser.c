#include <getopt.h>
#include <string.h>
#include <math.h>

#include "dbg.h"
#include "option_parser.h"



/* 
 * Long command-line option and their short equivalents
 */
static const struct option longOpts[] = {
        {"help", no_argument, NULL, 'h'},
        {"n-iterations", required_argument, NULL, 'n'},
        {"output-prefix", required_argument, NULL, 'o'},
        {"n-steps", required_argument, NULL, 's'},
        {NULL, 0, 0, 0} /* To prevent crash in case of illegal option */
};

static const char * shortOpts = "hn:o:s:";





static void print_help()
{
        puts("Usage: prog-name [args]\n");

        puts("Available arguments:");
        puts("\t-h --help                 print this message and exit");
        puts("\t-n --n-iterations n       number of iterations");
        puts("\t-o --output-prefix pref   Prefix to the output files. They'll be pref1.dat, pref2.dat, ...");
        puts("\t-s --n-steps num          Number of steps in the discretization");
}






int parse_options(struct prog_options *pProgOptions, int argc, char **argv)
{
        /* Initialize options with default values */
        pProgOptions->nIterations = 1000000;
        pProgOptions->outFilePrefix = "output";
        pProgOptions->nSteps = 1000;

        int ret;
        int opt = getopt_long(argc, argv, shortOpts, longOpts, NULL);
        while (opt != -1) {
                switch (opt) {
                        case 'n':
                                ret = sscanf(optarg, "%u", &pProgOptions->nIterations);
                                check (ret >= 0, "Failed to parse the number of iterations");
                                break;
                        case 'o':
                                pProgOptions->outFilePrefix = malloc(strlen(optarg) + 1);
                                check_mem(pProgOptions->outFilePrefix);
                                strcpy(pProgOptions->outFilePrefix, optarg);
                                check (ret >= 0, "Failed to copy outfile prefix");
                                break;
                        case 's':
                                ret = sscanf(optarg, "%u", &pProgOptions->nSteps);
                                check (ret >= 0, "Failed to parse the number of steps");
                                break;
                        case 'h':
                                print_help();
                                exit(0);
                                break;
                        case 0: /* Not a short option */
                                log_err("Unknown argument %s\n", argv[optind]);
                                exit(1);
                                break;
                        default:
                                log_err("Unknown argument %s\n", argv[optind]);
                                exit(1);
                                break;
                }
                opt = getopt_long(argc, argv, shortOpts, longOpts, NULL);
        }


        debug("Command-line args: nIterations: %u, output prefix: %s",
               pProgOptions->nIterations, pProgOptions->outFilePrefix);
        return 0;
}
