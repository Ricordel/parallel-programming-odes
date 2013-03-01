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
        uint32_t nPoints; /**< Number of discretization points of [0, 1] */
        double step; /**< Interval between two points in the discritization */
        uint32_t nIterations; /**< Number of Jacobi iterations */
};



/**
 * A struct that contains computation details specific to a node.
 */
struct parallel_context {
        uint32_t rank; /**< Rank of the process */
        uint32_t nProcs; /**< Total number of processros */
        uint32_t firstIndex; /**< Index of the first element handeled by the node */
        uint32_t nElemsAtNode; /**< Number of elements the node is responsible for */
        double *r_vals; /**< Cache for the values of r on the elements the node is responsible for */
        double *f_vals; /**< Cache for the values of f on the elements the node is responsible for */
        double *curVals; /**< Buffer containing the current values for the elems the node is
                              responsible for, plus one more element on each side (needed for computations) */
        double *nextVals; /**< Temporary array containing the values currently being computed */
};



/**
 * This type represents a color, either red or black, for red/black communication.
 */
enum color { RED, BLACK };



#endif /* end of include guard: __SOLVE_ODE_H__ */
