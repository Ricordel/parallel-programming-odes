#ifndef __dbg_h__
#define __dbg_h__

/*************
 * Mostly inspired from Zed Shaw's Awesome Debug Macros
 * (see http://c.learncodethehardway.org/book/ex20.html)
 * and slightly modified to suit my needs.
 *************/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>


#include "solve_ode_mpi.h"

#ifdef NDEBUG
#define debug(...)
#else
#define debug(...) fprintf(stderr, "[DEBUG] (%s:%d) ", __FILE__, __LINE__);\
                   fprintf(stderr, __VA_ARGS__); \
                   fprintf(stderr, "\n")
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define log_err(...) fprintf(stderr, "[ERROR] (%s%d: errno: %s) ", __FILE__, __LINE__, clean_errno()); \
                     fprintf(stderr, __VA_ARGS__); \
                     fprintf(stderr, "\n")

#define log_warn(...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) ", __FILE__, __LINE__, clean_errno()); \
                      fprintf(stderr, __VA_ARGS__); \
                      fprintf(stderr, "\n")

#define log_info(...) fprintf(stderr, "[INFO] (%s:%d) ", __FILE__, __LINE__);\
                      fprintf(stderr, __VA_ARGS__); \
                      fprintf(stderr, "\n")

#define check(A, ...) if(!(A)) { log_err(__VA_ARGS__); errno=0; abort(); }

#define check_mem(A) check((A), "Out of memory.")

#define check_null(A) check((A), "Got null pointer for %s\n", #A)

#define check_debug(A, ...) if (!(A)) { debug(__VA_ARGS__); errno=0; goto error; }

#define die(...) fprintf(stderr, "FATAL: ");\
                 fprintf(stderr, __VA_ARGS__);\
                 fprintf(stderr, "\n");\
                 abort();



#ifdef NDEBUG
#define print_ode(pOde)
#else
#define print_ode(pOde) \
        do { \
                printf("ODE: r <function>\n"); \
                printf("     f <function>\n"); \
                printf("     nPoints:     %u\n", (pOde)->nPoints); \
                printf("     nIterations: %u\n", (pOde)->nIterations); \
                printf("     step:        %lf\n", (pOde)->step); \
        } while (0)
#endif



#ifdef NDEBUG
#define print_context(pOde)
#else
#define print_context(pCtx) \
        do { \
                printf("ctx: rank: %u\n", (pCtx)->rank); \
                printf("     nProcs: %u\n", (pCtx)->nProcs); \
                printf("     firstIndex: %u\n", (pCtx)->firstIndex); \
                printf("     nElemsAtNode: %u\n", (pCtx)->nElemsAtNode); \
        } while (0)
                
#endif


#endif /* include guard __dbg_h__ */
