Functionnality
==============

This program numerically solves differential equations of the form

.. math::

    u''(x) + r(x)u(x) = f(x)

on :math:`(0, 1)` with the constraints :math:`u(0) = 0, u(1) = 0` using MPI.

Compile and run
===============

To compile:
::

    $ make

To run:
::

    $ mpirun -np <num threads> ./solve_ode_mpi [options]

To know the options to the program, run 
::

    $ ./solve_ode_mpi --help


Plot the results
====================


Use the ``./draw_curve`` script as follows::

      $ ./draw_curve.py <files prefix> <number of files>

| For instance, running the program on 8 threads with the default
| arguments (1000000 iterations, 1000 data points) will output
| the files:
|    ``output0.dat output1.dat output2.dat output3.dat output4.dat``
|    ``output5.dat output6.dat output7.dat``
| each containing part of the data. To plot them:
|    ``$ ./draw_curve.py output 8``
