#ifndef __OPTION_PARSER_H__
#define __OPTION_PARSER_H__

#include <stdint.h>


/**** Command-line options *****/
struct prog_options {
        uint32_t nIterations;
        uint32_t nSteps;
        char *outFilePrefix;
};



/* Parses the command-line */
int parse_options(struct prog_options *pProgOptions, int argc, char **argv);



#endif /* end of include guard: __OPTION_PARSER_H__ */
