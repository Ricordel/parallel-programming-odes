#ifndef __SOLVE_ODE_H__
#define __SOLVE_ODE_H__

#include <stdint.h> // NOTE: needs a C99-compliant compiler


/**
 * Type for a mathematical function R -> R, will be used for both r and f
 * (as in the test of the assignment)
 */
typedef double(*function_t)(double);


/**
 * Struct to represent an ODE of the form
 *      u'' + r.u = f
 * and the parameters for its resolution on (0, 1) (granularity of the mesh)
 */
struct ode {
        function_t r;
        function_t f;
        uint32_t nPoints;
        double step;
        uint32_t nIterations;
};



/**
 * A struct that contains the details for a node: its rank and the number
 * of nodes
 */
struct parallel_context {
        uint32_t rank;
        uint32_t nProcs;
        uint32_t firstIndex;
        uint32_t nElemsAtNode;
        double *curVals;
        double *nextVals;
};






/**
 * This type represents a color, either red or black, for red/black communication.
 */
enum color { RED, BLACK };



#endif /* end of include guard: __SOLVE_ODE_H__ */
