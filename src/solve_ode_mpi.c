#include <mpi.h>
#include <stdint.h>
#include <math.h>

#include "dbg.h"
#include "solve_ode_mpi.h"
#include "option_parser.h"



/**
 * File: solve_ode_mpi.c
 * Author: Yoann Ricordel <yoann.ricordel *AT- gmail PUNKT com
 *
 * Content: Code to numerically solve ODEs of the form
 *              y'' + ry = f
 *          using the Jacobi iteration. Realised for the "parallel programming
 *          for large scale problems" course at KTH.
 */



/**
 * The function r in y" + ry = f
 *
 * in: x     Variable of the function in the ODE
 */
static double r(double x)
{
        return -exp(-x);
}


/**
 * The function f in y" + ry = f
 *
 * in: x     Variable of the function in the ODE
 */
static double f(double x)
{
        return cos(10*x);
}



/**
 * This functions performs one iteration of the Jacobi method. pCtx->curVals
 * contains the current values of the function, and the result of the iteration
 * will be computed using pCtx->nextVals as a temporary array.
 * The elements the current process is responsible for are contained in the
 * slots 1..nElemsAtNode+1. There is one more element on each side that is
 * used for the computations. These elements have been obtained from the
 * boundary conditions or neighbours during the communication step.
 */
static void jacobiStep(struct parallel_context *pCtx, const struct ode *pOde)
{
        for (uint32_t i = 1; i < pCtx->nElemsAtNode + 1; i++) {
                /* -1 in f_vals because f_vals contains only the values for the
                 * points the process is responsible for, not from the borders. */
                double num = pCtx->curVals[i-1] + pCtx->curVals[i+1]
                             - pOde->step * pOde->step * pCtx->f_vals[i-1];
                double denom = 2.0 - pOde->step * pOde->step * pCtx->r_vals[i-1];

                pCtx->nextVals[i] = num / denom;
        }

        double *tmp = pCtx->curVals;
        pCtx->curVals = pCtx->nextVals;
        pCtx->nextVals = tmp;
}



/**
 * Step of communication with the process "on the left", i.e with inferior
 * rank. Can be the boundary condition on 0 if rank is 0.
 *
 * Inout: pCtx  Pointer to the paralle context. Its curVals buffer will be udpated.
 */
static void communicate_left(struct parallel_context *pCtx)
{
        int rc;
        if (pCtx->rank == 0) {
                /* Not really a lot to do */
                pCtx->curVals[0] = 0;
                return;
        }

        /* In the common case: there is somebody on the left, send the
         * second element of the array, and receive the first one */
        rc = MPI_Sendrecv(&pCtx->curVals[1], 1, MPI_DOUBLE, pCtx->rank - 1, 0,
                          &pCtx->curVals[0], 1, MPI_DOUBLE, pCtx->rank - 1, MPI_ANY_TAG,
                          MPI_COMM_WORLD, NULL);
        check(rc == MPI_SUCCESS, "Failed to sendRecv on the left at node %u", pCtx->rank);
}



/**
 * Step of communication with the process "on the right", i.e with superior
 * rank. Can be the boundary condition on 1 if rank is maxRank.
 *
 * Inout: pCtx  Pointer to the paralle context. Its curVals buffer will be udpated.
 */
static void communicate_right(struct parallel_context *pCtx)
{
        int rc;
        if (pCtx->rank == pCtx->nProcs - 1) {
                /* Not really a lot to do */
                pCtx->curVals[pCtx->nElemsAtNode + 1] = 0;
                return;
        }

        /* In the common case: there is somebody on the right, send the
         * before last element of the array, and receive the last one */
        rc = MPI_Sendrecv(&pCtx->curVals[pCtx->nElemsAtNode], 1, MPI_DOUBLE, pCtx->rank + 1, 0,
                          &pCtx->curVals[pCtx->nElemsAtNode+1], 1, MPI_DOUBLE, pCtx->rank + 1, MPI_ANY_TAG,
                          MPI_COMM_WORLD, NULL);
        check(rc == MPI_SUCCESS, "Failed to sendRecv on the right at node %u", pCtx->rank);
}



/**
 * Communication with neighbours to get and send the latest values on the
 * borders. Implements red-black communciation to prevent deadlocks.
 *
 * Inout: pCtx  Context of the current node. Its curVals array will be updated.
 */
static void communicate_boundaries(struct parallel_context *pCtx)
{
        enum color color = pCtx->rank % 2 == 0 ? RED : BLACK;

        if (color == RED) {
                communicate_left(pCtx);
                communicate_right(pCtx);
        } else {
                communicate_right(pCtx);
                communicate_left(pCtx);
        }
}



/**
 * Compute the values of f and r for the points the process is responsible
 * for, to save computation time during Jacobi iterations.
 *
 * In: pOde     Contains the parameters of the ODE, specifically f and r
 * Inout: pCtx  Context of the process. At function return, its r_vals and
 *              f_vals arrays will be filled with relevant values of f and r.
 */
static void precompute_functions(struct parallel_context *pCtx, struct ode *pOde)
{
        for (uint32_t i = 0; i < pCtx->nElemsAtNode; i++) {
                uint32_t globalIndex = pCtx->firstIndex + i;
                double xn = pOde->step * globalIndex;
                pCtx->r_vals[i] = pOde->r(xn);
                pCtx->f_vals[i] = pOde->f(xn);
        }
}



/**
 * Resolution of the equation, consists in iterating Jacobi steps and
 * communication steps. We don't check for convergence but use a fixed
 * number of iterations. We also pre-compute the values of f and r to
 * save computation time during the resolution.
 *
 * In: pOde     Contains the parameters of the ODE
 * Inout: pCtx  Information of the context. At exit, will contain part of
 *              the solution to the ODE in its curVals array
 */
static void solve_equation(struct parallel_context *pCtx, struct ode *pOde)
{
        pCtx->r_vals = (double *)calloc(pCtx->nElemsAtNode, sizeof(double));
        check_mem(pCtx->r_vals);
        pCtx->f_vals = (double *)calloc(pCtx->nElemsAtNode, sizeof(double));
        check_mem(pCtx->f_vals);

        precompute_functions(pCtx, pOde);

        for (uint32_t i = 0; i < pOde->nIterations; i++) {
                if (i % 10000 == 0) {
                        printf("Iteration %u\n", i);
                }
                jacobiStep(pCtx, pOde);
                communicate_boundaries(pCtx);
        }

        free(pCtx->r_vals);
        free(pCtx->f_vals);
}



/**
 * Save the partial resutls of current node in a file, with the following format:
 *                      value1 value2 ... valueN
 *
 * In: pCtx     Context of the process, its curVals values will be saved to file
 * In: outFile  File descriptor to an open file in which the values will be written
 */
static void save_results(struct parallel_context *pCtx, FILE *outFile)
{
        int ret;
        for (uint32_t i = 1; i < pCtx->nElemsAtNode + 1; i++) {
                ret = fprintf(outFile, "%lf ", pCtx->curVals[i]);
                check (ret > 0, "Failed to write to output file");
        }
}



/**
 * Gives the number of elements at a given node.
 *
 * In: nodeNumber       Rank of the node
 * In: nbNodes          Total number of nodes
 * In: nbElems          Number of elements scattered througout all nodes
 *
 * Returns: The number of elements at the given node
 */
static inline int elems_at_node(int nodeNumber, int nbNodes, uint32_t nbElems)
{
        int d = nbElems / nbNodes;
        int m = nbElems % nbNodes;

        if (nodeNumber < m) {
                return d + 1;
        } else {
                return d;
        }
}



/**
 * Gives the index of the first element handled by a given node
 *
 * In: nodeNumber       Rank of the node
 * In: nbNodes          Total number of nodes
 * In: nbElems          Number of elements scattered througout all nodes
 *
 * Returns: Global index of the first element handled by the node.
 */
static inline int first_elem_of_node(int nodeNumber, int nbNodes, uint32_t nbElems)
{
        int d = nbElems / nbNodes;
        int m = nbElems % nbNodes;

        if (nodeNumber == 0) {
                return 0; /* Does not fit in the following formula */
        } else if (nodeNumber < m) {
                return (d + 1) * nodeNumber - 1;
        } else {
                return (d + 1) * m + d * (nodeNumber - m) - 1;
        }
}






int main(int argc, char **argv)
{
        int rc;

        /* First of all, start MPI */
        rc = MPI_Init(&argc, &argv);
        check (rc == MPI_SUCCESS, "Failed to init MPI");

        /* Get program options, and then MPI parameters */
        struct prog_options progOptions;
        parse_options(&progOptions, argc, argv);

        int32_t nProcs, rank;
        rc = MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
        check (rc == MPI_SUCCESS, "Failed to get number of processor");

        rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        check (rc == MPI_SUCCESS, "Failed to get rank");

        /* Construct the name of the output file in function of the rank,
         * and try to open it right ahead, before starting the computations */
        char *outFileName = NULL;
        outFileName = (char *) calloc(strlen(progOptions.outFilePrefix) + 10, sizeof(char));
        check_mem(outFileName);
        sprintf(outFileName, "%s%d.dat", progOptions.outFilePrefix, rank);
        FILE *outFile = fopen(outFileName, "w");
        check_null(outFile);


        /* Create and fill the local context, and the parameters of the ODE. */
        double step = (double)1 / (double)(progOptions.nSteps + 1);
        struct ode ode = { r, f, 1000, step, progOptions.nIterations };

        struct parallel_context ctx;
        ctx.rank = rank;
        ctx.nProcs = nProcs;
        ctx.firstIndex = first_elem_of_node(rank, nProcs, progOptions.nSteps);
        ctx.nElemsAtNode = elems_at_node(rank, nProcs, progOptions.nSteps);
        ctx.curVals = (double *) calloc(ctx.nElemsAtNode + 2, sizeof(double));
        check_mem(ctx.curVals);
        ctx.nextVals = (double *) calloc(ctx.nElemsAtNode + 2, sizeof(double));
        check_mem(ctx.nextVals);

        /* For debug printing only */
        print_ode(&ode);
        print_context(&ctx);

        /***** That's it, we can now solve our equation, and save the result *****/
        solve_equation(&ctx, &ode);
        save_results(&ctx, outFile);


        /***** And time for some cleanup *****/
        fclose(outFile);
        free(outFileName);
        free(ctx.curVals);
        free(ctx.nextVals);
        free(progOptions.outFilePrefix);

        rc = MPI_Finalize();
        check (rc == MPI_SUCCESS, "Failed to finalize");

        return 0;
}
