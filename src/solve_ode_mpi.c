#include <mpi.h>
#include <stdint.h>
#include <math.h>

#include "dbg.h"
#include "solve_ode_mpi.h"
#include "option_parser.h"



double r(double x)
{
        return -exp(-x);
}


double f(double x)
{
        return cos(10*x);
}



/**
 * This functions performs one iteration of the Jacobi method
 * curVals and nextVals contain the xs for which the current process is
 * responsible between indexes 1 and nElemsAtNode. The index 0 contains
 * the value from the process on the left (or the boundary condition)
 * and the index nElemsAtNode + 1 contains the value from the process
 * on the right (or the boundary condition).
 */
void jacobiStep(struct parallel_context *pCtx, const struct ode *pOde)
{
        for (uint32_t i = 1; i < pCtx->nElemsAtNode + 1; i++) {
                double num = pCtx->curVals[i-1] + pCtx->curVals[i+1]
                             - pOde->step * pOde->step * pCtx->f_vals[i-1];
                double denom = 2.0 - pOde->step * pOde->step * pCtx->r_vals[i-1];

                pCtx->nextVals[i] = num / denom;
        }

        double *tmp = pCtx->curVals;
        pCtx->curVals = pCtx->nextVals;
        pCtx->nextVals = tmp;
}



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
        check(rc == MPI_SUCCESS, "Failed to sendRecv on the left at node %d", pCtx->rank);
}



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
        check(rc == MPI_SUCCESS, "Failed to sendRecv on the right at node %d", pCtx->rank);
}



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





void solve_equation(struct parallel_context *pCtx, struct ode *pOde)
{
        for (uint32_t i = 0; i < pOde->nIterations; i++) {
                if (i % 10000 == 0) {
                        printf("Iteration %u\n", i);
                }
                jacobiStep(pCtx, pOde);
                communicate_boundaries(pCtx);
        }
}



/**
 * Save the partial resutls of current node in a file, with the following format:
 *                      value1 value2 ... valueN
 */
void save_results(struct parallel_context *pCtx, FILE *outFile)
{
        int ret;
        for (uint32_t i = 1; i < pCtx->nElemsAtNode + 1; i++) {
                ret = fprintf(outFile, "%lf ", pCtx->curVals[i]);
                check (ret > 0, "Failed to write to output file");
        }
}





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



static void precompute_functions(struct parallel_context *pCtx, struct ode *pOde)
{

        pCtx->r_vals = (double *)calloc(pCtx->nElemsAtNode, sizeof(double));
        check_mem(pCtx->r_vals);
        pCtx->f_vals = (double *)calloc(pCtx->nElemsAtNode, sizeof(double));
        check_mem(pCtx->f_vals);

        for (uint32_t i = 0; i < pCtx->nElemsAtNode; i++) {
                uint32_t globalIndex = pCtx->firstIndex + i;
                double xn = pOde->step * globalIndex;
                pCtx->r_vals[i] = pOde->r(xn);
                pCtx->f_vals[i] = pOde->f(xn);
        }
}




int main(int argc, char **argv)
{
        int rc;
        /* First of all, start MPI */
        rc = MPI_Init(&argc, &argv);
        check (rc == MPI_SUCCESS, "Failed to init MPI");

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
        sprintf(outFileName, "%s%u.dat", progOptions.outFilePrefix, rank);
        FILE *outFile = fopen(outFileName, "w");


        /* Create the contexts */
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
        precompute_functions(&ctx, &ode);

        print_ode(&ode);
        print_context(&ctx);

        /***** That's it, we can now solve our equation, and save the result *****/
        solve_equation(&ctx, &ode);
        save_results(&ctx, outFile);


        /***** And clean the context ******/
        rc = MPI_Finalize();
        check (rc == MPI_SUCCESS, "Failed to finalize");

        return 0;
}
